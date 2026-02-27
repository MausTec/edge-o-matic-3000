# Relicensing Notice

Effective February 26, 2026, this project is licensed under the **MIT License**.

Prior to this date, the project was distributed under the GNU General Public
License v3.0 (GPL-3.0). Copies obtained before this date under GPL-3.0 retain
those terms. The final GPL-3.0 release is tagged as `v1.3.0-GPL`.

## Why the change

The GPL-3.0 was adopted early in this project's life without adequate
consideration for compatibility with the ESP-IDF toolchain (which includes
precompiled binary components) or for the realities of shipping commercial
firmware. As the project matured, the license became untenable:

- **Toolchain incompatibility.** ESP-IDF includes precompiled blobs for which
  source is not available, making GPL-3.0 distribution non-compliant.
- **Project focus.** Maintaining a public codebase for commercial firmware
  increasingly diverted resources from product development toward managing
  external contributions and source-level support requests outside the scope
  of the documented product interface.

The MIT License applies to all publicly available source from this point forward,
unless otherwise noted by the copyright holder. Maus-Tec LLC reserves the right
to change the license terms of future releases.

## What this means for you

- **Existing forks under GPL-3.0** remain valid under GPL-3.0 for the code
  they contain as of their fork date.
- **New code from this commit forward** is MIT-licensed.
- **Binary firmware updates** distributed via OTA or direct download are
  provided under the product Terms of Service at maus-tec.com.

Questions: info@maus-tec.com
