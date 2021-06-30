# Edge-o-Matic 3000 - An Automated Orgasm Denial Device

This code is an ESP32 rewrite of the core NoGasm project by Rhoboto: [github.com/nogasm/nogasm](https://github.com/nogasm/nogasm)

Using an inflatable butt plug to detect pressure changes indicative of pelvic floor contractions, this
software, and associated hardware, is used to detect when the user is approaching orgasm and control
stimulation accordingly. The net result: automated edging and orgasm denial.

## Web UI

Edge-o-Matic 3000, NoGasm Plus, and Edge-o-Matic AiO devices both support a Websocket connection and a Web UI. The current Web UI can be accessed
at [nogasm-ui.maustec.io](http://nogasm-ui.maustec.io). The source code is at [github.com/maustec/nogasm-ui](https://github.com/maustec/nogasm-ui).

## WebSocket API

Documentation for the WebSocket API can be found in [doc/WebSocket.md](doc/WebSocket.md).

## Configuration

If you have hardware already, please see `examples/config.json` or your own `config.json` file
shipped with your SD card. Note that this JSON document does not support comment preprocessing,
and is automatically generated. Here is a quick summary of config variables:

|Key|Type|Default|Note|
|---|----|---|---|
|`wifi_ssid`|String|""|Your WiFi SSID|
|`wifi_key`|String|""|Your WiFi Password.|
|`wifi_on`|Boolean|false|True to enable WiFi / Websocket server.|
|`bt_display_name`|String|"Edge-o-Matic 3000"|AzureFang* device name, you might wanna change this.|
|`bt_on`|Boolean|false|True to enable the AzureFang connection.|
|`led_brightness`|Byte|128|LED Ring max brightness, only for NoGasm+.|
|`websocket_port`|Int|80|Port to listen for incoming Websocket connections.|
|`use_ssl`|Boolean|false|Enable SSL server, which will eat all your RAM!|
|`hostname`|String|"eom3k"|Local hostname for your device.|
|`motor_start_speed`|Byte|10|The minimum speed the motor will start at in automatic mode.|
|`motor_max_speed`|Byte|128|Maximum speed for the motor in auto-ramp mode.|
|`motor_ramp_time_s`|Int|30|The time it takes for the motor to reach `motor_max_speed` in auto ramp mode.|
|`edge_delay`|Int|10000|Time (ms) after edge detection before resuming stimulation.|
|`minimum_on_time`|Int|1000|Time (ms) after stimulation starts before edge detection is resumed.|
|`screen_dim_seconds`|Int|10|Time, in seconds, before the screen dims. 0 to disable.|
|`screen_timeout_seconds`|Int|0|Time, in seconds, before the screen turns off. 0 to disable.|
|`pressure_smoothing`|Byte|5|Number of samples to take an average of. Higher results in lag and lower resolution!|
|`classic_serial`|Boolean|false|Output classic NoGasm values over serial for backwards compatibility.|
|`sensitivity_threshold`|Int|600|The arousal threshold for orgasm detection. Lower = sooner cutoff.|
|`update_frequency_hz`|Int|50|Update frequency for pressure readings and arousal steps. Higher = crash your serial monitor.|
|`sensor_sensitivity`|Byte|128|Analog pressure prescaling. Adjust this until the pressure is ~60-70%|
|`use_average_values`|Boolean|false|Use average values when calculating arousal. This smooths noisy data.|
|`vibration_mode`|VibrationMode|RampStop|Vibration Mode for main vibrator control.|

\* AzureFang refers to a common wireless technology that is blue and involves chewing face-rocks. However, the
   trademark holders of this technology require the name to be licensed, so we're totally just using AzureFang.
   
### Vibration Modes:

|ID|Name|Description|
|---|---|---|
|1|Ramp-Stop|Vibrator ramps up from set min speed to max speed, stopping abruptly on arousal threshold crossing.|
|2|Depletion|Vibrator speed ramps up from min to max, but is reduced as arousal approaches threshold.|
|3|Enhancement|Vibrator speed ramps up as arousal increases, holding a peak for ramp_time.|
|0|Global Sync|When set on secondary vibrators, they will follow the primary vibrator speed.|

## Hardware

Hardware builds for this project can be purchased from Maus-Tec Electronics, at [maustec.io/eom](https://maustec.io/eom).

Hardware development and assembly helps keep pizza in the freezer and a roof over the head of the maintainer.
Your support helps a small business grow into something neat, and ensures future devices like this can continue
to be produced.

The User Guide for the hardware can be downloaded at [doc/Edge-o-Matic_UserGuide.docx](doc/Edge-o-Matic_UserGuide.docx).

### That RJ45 Jack

**The RJ45 Jack IS NOT ETHERNET!** That is a balanced twisted pair extension of the I2C bus on the controller, and is available
as a convenience for future development, dongles, and accessories. It carries +5V and SDA+/-, SCL+/- signals. You must use an
I2C redriver IC or compatible module to interface with this. Additionaly, this is terminated at 100ohm assuming standard CAT5.
[We recommend SparkFun's BOB-14589](https://www.digikey.com/product-detail/en/sparkfun-electronics/BOB-14589/1568-1873-ND/9351349).

|Pin|Signal|
|---|---|
|1|`SCL-`|
|2|`SCL+`|
|3|NC|
|4|`+5V`|
|5|`GND`|
|6|NC|
|7|`SDA-`|
|8|`SDA+`|

# Contributions

Any changes to configuration values should be linted. Run `ruby bin/config_lint.rb` to check documentation, serializers
and structs for consistency.

Thanks for readin'!
