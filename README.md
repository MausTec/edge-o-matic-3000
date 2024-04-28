# Edge-o-Matic 3000 - An Automated Orgasm Denial Device

Using an inflatable butt plug to detect pressure changes indicative of pelvic floor contractions, this
software, and associated hardware, is used to detect when the user is approaching orgasm and control
stimulation accordingly. The net result: automated edging and orgasm denial.

## Web UI

Edge-o-Matic 3000 devices support a WebSocket communication channel to be used on a web UI. The current Web UI can be accessed
at [link.maustec.io](http://link.maustec.io).
For control outside your Local Area Network, please consider a port-forwarding software such as Ngrok. You will have to permit insecure content
for that domain within Chrome to be able to connect your device while the page is loaded over HTTPS. This cannot be done automatically for you.
A remote bridge for Maus-Link compatible devices is in development and should be released Q4 2024. This will allow internet control of genuine
Maus-Tec products.

## WebSocket API

Documentation for the WebSocket API can be found in [doc/WebSocket.md](doc/WebSocket.md).

## Configuration

If you have hardware already, please see `examples/config.json` or your own `config.json` file
shipped with your SD card. Note that this JSON document does not support comment preprocessing,
and is automatically generated. Here is a quick summary of config variables:

|Key|Type|Default|Note|
|---|----|---|---|
|`wifi_ssid`|String|""|Your WiFi SSID.|
|`wifi_key`|String|""|Your WiFi Password.|
|`wifi_on`|Boolean|false|Enable WiFi and the Websocket server. Cannot be enabled if AzureFang is on.|
|`bt_display_name`|String|"Edge-o-Matic 3000"|AzureFang* device name, you might wanna change this.|
|`bt_on`|Boolean|false|Enable AzureFang connectivity. Cannot be enabled if WiFi is on.|
|`force_bt_coex`|Boolean|false|Force AzureFang and WiFi at the same time**.|
|`led_brightness`|Byte|128|Status LED maximum brightness.|
|`websocket_port`|Int|80|Port to listen for incoming Websocket connections.|
|`use_ssl`|Boolean|false|Enable SSL server, which will eat all your RAM!|
|`hostname`|String|"eom3k"|Local hostname for your device.|
|`motor_start_speed`|Byte|10|The minimum speed the motor will start at in automatic mode.|
|`motor_max_speed`|Byte|128|Maximum speed for the motor in auto-ramp mode.|
|`motor_ramp_time_s`|Int|30|The time it takes for the motor to reach Motor Max Speed in automatic mode.|
|`edge_delay`|Int|10000|Minimum time (ms) after edge detection before resuming stimulation.|
|`max_additional_delay`|Int|10000|Maximum time (ms) that can be added to the edge delay before resuming stimulation. A random number will be picked between 0 and this setting each cycle. 0 to disable.|
|`minimum_on_time`|Int|1000|Time (ms) after stimulation starts before edge detection is resumed.|
|`screen_dim_seconds`|Int|10|Time, in seconds, before the screen dims. 0 to disable.|
|`screen_timeout_seconds`|Int|0|Time, in seconds, before the screen turns off. 0 to disable.|
|`reverse_menu_scroll`|Boolean|false|Reverse the direction of the scroll wheel when navigating menus.|
|`pressure_smoothing`|Byte|5|Number of samples to take an average of. Higher results in lag and lower resolution!|
|`classic_serial`|Boolean|false|Output continuous stream of arousal data over serial for backwards compatibility with other software.|
|`sensitivity_threshold`|Int|600|The arousal threshold for orgasm detection. Lower values stop sooner.|
|`update_frequency_hz`|Int|50|Update frequency for pressure readings and arousal steps. Higher = crash your serial monitor.|
|`sensor_sensitivity`|Byte|128|Analog pressure prescaling. Please see instruction manual.|
|`use_average_values`|Boolean|false|Use average values when calculating arousal. This smooths noisy data.|
|`vibration_mode`|VibrationMode|RampStop|Vibration Mode for main vibrator control.|
|`use_post_orgasm`|Boolean|false|Use post-orgasm torture mode and functionality.|
|`clench_pressure_sensitivity`|Int|200|Minimum additional Arousal level to detect clench. See manual.|
|`clench_time_to_orgasm_ms`|Int|1500|Threshold variable that is milliseconds count of clench to detect orgasm. See manual.|
|`clench_detector_in_edging`|Boolean|false|Use the clench detector to adjust Arousal. See manual.|
|`auto_edging_duration_minutes`|Int|30|How long to edge before permiting an orgasm.|
|`post_orgasm_duration_seconds`|Int|10|How long to stimulate after orgasm detected.|
|`edge_menu_lock`|Boolean|false|Deny access to menu starting in the edging session.|
|`post_orgasm_menu_lock`|Boolean|false|Deny access to menu starting after orgasm detected.|
|`max_clench_duration_ms`|Int|3000|Duration the clench detector can raise arousal if clench detector turned on in edging session.|
|`clench_time_threshold_ms`|Int|900|Threshold variable that is milliseconds counts to detect the start of clench.|
|`denials_count_to_orgasm`|Int|10|How many denials before permiting an orgasm, if this mode is chosen.|
|`milk_o_matic_rest_duration_minutes`|Int|60|How long to rest before restarting an other round of Denial_count edging.|
|`random_orgasm_triggers`|Boolean|false|Randomize Edge timer and Denial count to minimum of 1/2 of their values.|
|`post_orgasm_mode`|PostOrgasmMode|Timer|Timer, Denial_count and Milk-O-Matic. Random is a choice between Timer and Milk-o-matic. See documentation for more details.|
|`max_orgasms`|Int|4|Milk-o-matic maximum orgasms before turning off.|


\* AzureFang refers to a common wireless technology that is blue and involves chewing face-rocks. However, the
   trademark holders of this technology require the name to be licensed, so we're totally just using AzureFang.

\** AzureFang and WiFi coexistance is EXPERIMENTAL and may cause system instability. If your device resets with
    both of these turned on, please turn them back off. Additionally, it might be helpful to post some info in
    discord of the last serial dump on the console when your device reset, assuming you have a console connected.
   
### Vibration Modes:

|ID|Name|Description|
|---|---|---|
|1|Ramp-Stop|Vibrator ramps up from set min speed to max speed, stopping abruptly on arousal threshold crossing.|
|2|Depletion|Vibrator speed ramps up from min to max, but is reduced as arousal approaches threshold.|
|3|Enhancement|Vibrator speed ramps up as arousal increases, holding a peak for ramp_time.|
|0|Global Sync|When set on secondary vibrators, they will follow the primary vibrator speed.|

### Post Orgasm Modes:
|ID|Name|Description|
|---|---|---|
|0|Denial_count|How many Denials before permiting an orgasm. Value set with - "Denials to permit orgasm" in the Orgasm Menu|
|1|Timer|How long to edge before permiting an orgasm. Value set with - "Edge Duration Minutes" in the Orgasm Menu|
|2|Milk_o_matic|Cycles in Denial_count mode after each orgasm. Repeats until max_orgasms reached|
|3|Random_mode|Random choice Timer or milk-o-matic before orgasm. For best result choose "Edge Duration Minutes" = 60 min and "Denials to permit orgasm" = 20. the idea is not knowing if you will have an 1 hours tease or multiple orgasm within that same 1 hour. Max_orgasms stops the session after it's reached.|

### post_orgasm_duration_seconds:
|Seconds|Description|
|---|---|
|0|Turn off stimulation at orgasm dectection - Failed Orgasm|
|5-15|Turn off stimulation after amount of seconds - Normal orgasm|
|16-4095|Turn off stimulation after amount of seconds - Post orgasm Torture|

Post-Orgasm and clench detector modules are community contributed features. While we will do our best to provide support
for these, we kindly request that you ask questions related to this in Discord under Firmware Development and not
Customer Support. Thank you for your understanding.

## Hardware

Hardware builds for this project can be purchased from Maus-Tec Electronics, at [maustec.io/eom](https://maustec.io/eom).

Hardware development and assembly helps keep pizza in the freezer and a roof over the head of the maintainer.
Your support helps a small business grow into something neat, and ensures future devices like this can continue
to be produced. Genuine hardware purchases also ensure continued maintenance and support can be provided for the project.

The User Guide for the hardware can be downloaded at [doc/Edge-o-Matic_UserGuide.docx](doc/Edge-o-Matic_UserGuide.docx).

### That RJ45 Jack

**The RJ45 Jack IS NOT ETHERNET!** This is the Maus-Bus Accessory Port, used to connect various Maus-Bus compatible devices
to build up a network of accessories and dongles to customize your play experience. Please only connect authorized Maus-Bus
accessories to this port. Unauthorized or counterfeit devices can damage your Edge-o-Matic. Do not connect it to an ethernet
jack. This is not for internet.

# Contributions

Any changes to configuration values should be linted. Run `ruby bin/config_lint.rb` to check documentation, serializers
and structs for consistency.

Thanks for readin'!



---

Maus-Bus and Maus-Link are trademarks of Maus-Tec LLC. Edge-o-Matic is a registered trademark of Maus-Tec LLC. If you are
a 3rd party hardware manufacturer looking to certify your Maus-Bus connected device for use with the Edge-o-Matic, we 
encourage you to contact info@maustec.io and our friendly customer support robot will be glad to assist!
