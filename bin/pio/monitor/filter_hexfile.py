from platformio.commands.device import DeviceMonitorFilter
import pathlib
import math
import re

file_start = re.compile(r'^>>>HEXFILE:(\d+);(.*)')
file_end = re.compile(r'^<<<EOF')

class Hexfile(DeviceMonitorFilter):
    NAME = "hexfile"

    def __init__(self, *args, **kwargs):
        super(Hexfile, self).__init__(*args, *kwargs)
        self._buffer = ""
        self._file_buffer = ""
        self._incoming_file = False
        self._incoming_file_bytes = 0
        self._incoming_file_name = ""

    def rx(self, text):
        for idx in range(0, len(text)):
            self._buffer += text[idx]

            if self._buffer.endswith("\n"):
                m = file_start.search(self._buffer)
                m_end = file_end.search(self._buffer)

                if m:
                    self._incoming_file = True
                    self._incoming_file_bytes = m.group(1)
                    self._incoming_file_name = m.group(2).strip()
                    self._file_buffer = ""

                elif m_end:
                    self._incoming_file = False
                    path = pathlib.Path("tmp/serial-downloads/").absolute().joinpath(self._incoming_file_name.strip('/'))
                    path.parent.mkdir(parents=True, exist_ok=True)

                    f = open(path, "w+b")
                    for n in range(0, math.floor(len(self._file_buffer) / 2)):
                        pos = n * 2
                        byte = int(self._file_buffer[pos:pos+2], 16)
                        f.write(bytes([byte]))
                    f.close()
                
                elif self._incoming_file:
                    self._file_buffer += self._buffer.strip()

                self._buffer = ""

        return text

    def tx(self, text):
        return text