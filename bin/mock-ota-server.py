#!/usr/bin/env python3
"""
Mock OTA server for testing firmware updates on Edge-o-Matic 3000 devices.

Serves a firmware binary and version manifest that mimics the production
gh-release-embedded-bridge Cloud Function. Point the device's
remote_update_url config setting at this server to test OTA without
touching the real release infrastructure.

Usage:
    python3 bin/mock-ota-server.py [OPTIONS]

Options:
    --firmware PATH     Path to firmware.bin (default: .pio/build/eom3k/firmware.bin)
    --version VER       Version string to advertise (default: auto-detect from binary)
    --port PORT         Port to listen on (default: 8070)
    --bind ADDR         Address to bind (default: 0.0.0.0)

Then on the device, set remote_update_url to:
    http://<your-ip>:8070

The server handles:
    GET /version.txt    -> plain text version string
    GET /               -> JSON release manifest with asset list
    GET /firmware.bin   -> raw firmware binary (OTA image)
"""

import argparse
import json
import os
import struct
import sys
from datetime import datetime, timezone
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs

# ESP-IDF application descriptor offsets (from esp_app_format.h)
# image_header (24 bytes) + segment_header (8 bytes) = 32 byte offset
APP_DESC_OFFSET = 32
APP_DESC_MAGIC = 0xABCD5432


def read_app_desc(firmware_path):
    """Read the esp_app_desc_t from a firmware binary to extract version info."""
    with open(firmware_path, "rb") as f:
        f.seek(APP_DESC_OFFSET)
        data = f.read(256)  # esp_app_desc_t is 256 bytes

    if len(data) < 256:
        return None

    magic = struct.unpack_from("<I", data, 0)[0]
    if magic != APP_DESC_MAGIC:
        return None

    # esp_app_desc_t layout:
    #   uint32_t magic_word        (offset 0)
    #   uint32_t secure_version    (offset 4)
    #   uint32_t reserv1[2]        (offset 8)
    #   char     version[32]       (offset 16)
    #   char     project_name[32]  (offset 48)
    #   char     time[16]          (offset 80)
    #   char     date[16]          (offset 96)
    #   char     idf_ver[32]       (offset 112)
    #   uint8_t  app_elf_sha256[32](offset 144)
    #   uint32_t reserv2[20]       (offset 176)

    def read_str(offset, length):
        raw = data[offset : offset + length]
        return raw.split(b"\x00", 1)[0].decode("ascii", errors="replace")

    return {
        "version": read_str(16, 32),
        "project_name": read_str(48, 32),
        "compile_time": read_str(80, 16),
        "compile_date": read_str(96, 16),
        "idf_version": read_str(112, 32),
    }


class OTAHandler(BaseHTTPRequestHandler):
    """HTTP request handler for mock OTA server."""

    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path.rstrip("/")
        qs = parse_qs(parsed.query)

        current_ver = qs.get("current_version", ["unknown"])[0]
        serial = qs.get("device_serial", ["unknown"])[0]

        if path == "/version.txt":
            self._serve_version(current_ver, serial)
        elif path == "/firmware.bin":
            self._serve_firmware()
        elif path == "" or path == "/":
            self._serve_manifest(current_ver, serial)
        else:
            self.send_error(404)

    def _serve_version(self, current_ver, serial):
        ver = self.server.ota_version
        self.log_message(
            "Version check from %s (serial %s) -> serving %s", current_ver, serial, ver
        )
        body = ver.encode()
        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _serve_manifest(self, current_ver, serial):
        ver = self.server.ota_version
        host = self.headers.get("Host") or f"{self.server.server_address[0]}:{self.server.server_address[1]}"
        fw_url = f"http://{host}/firmware.bin"
        self.log_message(
            "Manifest request from %s (serial %s) -> version %s", current_ver, serial, ver
        )
        manifest = {
            "version": ver,
            "date": datetime.now(timezone.utc).strftime("%Y-%m-%d"),
            "assets": [
                {
                    "role": "image",
                    "url": fw_url,
                    "name": "firmware.bin",
                }
            ],
        }
        body = json.dumps(manifest, indent=2).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _serve_firmware(self):
        fw_path = self.server.firmware_path
        fw_size = os.path.getsize(fw_path)
        self.log_message("Serving firmware: %s (%d bytes)", fw_path, fw_size)
        self.send_response(200)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", str(fw_size))
        self.end_headers()
        with open(fw_path, "rb") as f:
            while True:
                chunk = f.read(8192)
                if not chunk:
                    break
                self.wfile.write(chunk)

    def log_message(self, fmt, *args):
        sys.stderr.write(f"[OTA] {fmt % args}\n")


def find_firmware(explicit_path=None):
    """Locate firmware binary, preferring explicit path, then common build outputs."""
    if explicit_path:
        if not os.path.isfile(explicit_path):
            print(f"Error: firmware not found at {explicit_path}", file=sys.stderr)
            sys.exit(1)
        return explicit_path

    candidates = [
        ".pio/build/eom3k/firmware.bin",
        ".pio/build/eom3k-debug/firmware.bin",
        "build/firmware.bin",
    ]
    for c in candidates:
        if os.path.isfile(c):
            return c

    print(
        "Error: no firmware binary found. Build first (pio run) or pass --firmware PATH.",
        file=sys.stderr,
    )
    sys.exit(1)


def get_local_ip():
    """Best-effort detection of the machine's LAN IP address."""
    import socket

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("10.255.255.255", 1))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"


def main():
    parser = argparse.ArgumentParser(description="Mock OTA server for EOM3K testing")
    parser.add_argument(
        "--firmware",
        default=None,
        help="Path to firmware.bin (default: auto-detect from build output)",
    )
    parser.add_argument(
        "--version",
        default=None,
        help="Version to advertise (default: read from firmware binary header)",
    )
    parser.add_argument("--port", type=int, default=8070, help="Port (default: 8070)")
    parser.add_argument(
        "--bind", default="0.0.0.0", help="Bind address (default: 0.0.0.0)"
    )
    args = parser.parse_args()

    firmware_path = find_firmware(args.firmware)
    print(f"Firmware: {firmware_path} ({os.path.getsize(firmware_path)} bytes)")

    # Try to read version from the binary header
    app_desc = read_app_desc(firmware_path)
    if app_desc:
        print(
            f"  Project: {app_desc['project_name']}  "
            f"Version: {app_desc['version']}  "
            f"Built: {app_desc['compile_date']} {app_desc['compile_time']}  "
            f"IDF: {app_desc['idf_version']}"
        )

    if args.version:
        version = args.version
    elif app_desc and app_desc["version"]:
        version = app_desc["version"]
    else:
        version = "99.0.0"
        print(f"  Warning: could not read version from binary, using {version}")

    local_ip = get_local_ip()

    print(f"\nServing OTA at http://{args.bind}:{args.port}")
    print(f"  Version: {version}")
    print(f"\nOn the device, set remote_update_url to:")
    print(f"  http://{local_ip}:{args.port}\n")

    server = HTTPServer((args.bind, args.port), OTAHandler)
    server.firmware_path = os.path.abspath(firmware_path)
    server.ota_version = version

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down.")
        server.server_close()


if __name__ == "__main__":
    main()
