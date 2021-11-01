from platformio.commands.device import DeviceMonitorFilter
import re
import subprocess

pattern = re.compile(r'[0-9a-fA-F]{2048}')
screenshot_rb = "bin\\lib\\screenshot.rb"

class Screenshot(DeviceMonitorFilter):
    NAME = "screenshot"

    def __init__(self, *args, **kwargs):
        super(Screenshot, self).__init__(*args, *kwargs)
        self._buffer = ""

    def rx(self, text):
        self._buffer += text

        if text.endswith("\n"):
            m = pattern.search(self._buffer)
            self._buffer = ""

            if m:
                print(text) # We're rearranging the output, this is probably bad.
                hexdata = m.group(0)
                return self.__do_screenshot(hexdata)
        
        return text

    def tx(self, text):
        return text

    def __do_screenshot(self, data):
        print("Saving screenshot data...\n")
        ruby = subprocess.run(["ruby", screenshot_rb, data], 
            shell=True, 
            stdout=subprocess.PIPE, 
            close_fds=True)
        return ruby.stdout.decode('utf-8')