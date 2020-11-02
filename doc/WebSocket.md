# WebSocket Communication

The Edge-o-Matic 3000 serves as a WebSocket host for streaming data over wireless networks. The NoGasm UI project makes 
use of this, but you are free to develop your own interface.

The WebSocket interface uses JSON serialization for both sending and receiving packets. The structure of a payload sent 
TO the Edge-o-Matic is a key-value object with each key corresponding to a command, and a value corresponding to the 
arguments to that command. Responses streamed from the device should be handled similarly. 

**An example request sent to the WebSocket channel:**

```json
{
    "configSet": {
        "motor_max_speed": 255
    },
    "configList": {
        "nonce": 1234
    }
}
```

**The associated response:**

```json
{
    "configList": {
        "nonce": 1234,
        "config": {…}
    }
}
```

Multiple commands can be sent in either direction, but only one of each command type can be specified. It is recommended
that you send only one command per request. To track responses to commands, you can use the “nonce” parameter, which 
some commands support. This is a numeric identifier that is returned with the associated response and can be used to 
filter duplicate responses.


## Server Commands

These commands are recognized by the WebSocket server running on the Edge-o-Matic 3000:


### `configSet`
Sets one or more configuration values.

**Arguments:**

|Argument|Type|Description|
|---|---|---|
|*|\<any>|Config keys / values|

**Example:**
```json
"configSet": { 
    "motor_max_speed": 255 
}
```


### `configList`
Requests all config values to be sent.

**Example:**
```json
"configList": null
```


### `serialCmd`	
Execute a string as a serial command.

**Arguments:**

|Argument|Type|Description|
|---|---|---|
|cmd|String|Command to execute|
|nonce|Numeric|Returned in response|

**Example:**
```json
"serialCmd": {
    "cmd": "external enable",
    "nonce": 1234
}
```


### `getWiFiStatus`
Requests the Wi-Fi status to be sent.

**Example:**
```json
"getWiFiStatus": {}
```


### `getSDStatus`
Requests SD Card status to be sent.

**Example:**
```json
"getSDStatus": {}
```


### `setMode`
Sets the current run mode.

**Arguments:**

|Argument|Type|Description|
|---|---|---|
|mode|String|`automatic`, `manual`|

**Example:**
```json
"setMode": "automatic"
```


### `setMotor`
Sets vibration speed. Will enter manual mode.

**Arguments:**

|Argument|Type|Description|
|---|---|---|
|speed|Byte|0-255 motor speed|

**Example:**
```json
"setMotor": 128
```


## Server Responses
Your application should be prepared to handle these messages streamed from the server. The actual data may change as 
this is a printed document and not live documentation. See GitHub for more up-to-date details.


### `configList`
A listing of the current configuration.

**Parameters:**

|Parameter|Type|Description|
|---|---|---|
|*|<any>|Current serialized configuration|

**Example:**
```json
"configList": {
    "motor_max_speed": 255,
    "wifi_on": false
}
```


### `serialCmd`
The response from a serial command.

**Parameters:**

|Parameter|Type|Description|
|---|---|---|
|text|String|Output text from command|
|nonce|Numeric|Same as initial request|

**Example:**
```json
"serialCmd": {
    "nonce": 1234,
    "text": "Enabled external bus\nOK\n”
}
```
 

### `wifiStatus`
The current Wi-Fi connection status.

**Parameters:**

|Parameter|Type|Description|
|---|---|---|
|ssid|String|The connected network SSID|
|ip|String|Device IP Address|
|rssi|Numeric|RSSI Value (signal strength)|

**Example:**
```json
"wifiStatus": {
    "rssi": -56,
    "ssid": "FBI Spy-Fi",
    "ip": "10.0.102.192"
}
```


### `sdStatus`
Current SD card status.

**Parameters:**

|Parameter|Type|Description|
|---|---|---|
|size|Numeric|Size of card in MB|
|type|String|Type of card inserted|

**Example:**
```json
"sdStatus": {
    "size": 127,
    "type": "MMC"
}
```
 

### `readings`
A collection of current readings and device status. This is streamed at the global update frequency, unless disabled, 
and is used for providing real-time updates to your application.

**Parameters:**

|Parameter|Type|Description|
|---|---|---|
|pressure|Numeric|Current pressure reading|
|pavg|Numeric|Rolling pressure average|
|motor|Numeric|Current vibrator speed|
|arousal|Numeric|Current arousal value|
|millis|Numeric|Millisecond timestamp|

**Example:**
```json
"readings": {
    "pressure": 1029,
    "pavg": 1028,
    "motor": 255,
    "arousal": 10,
    "millis": 198452
}
```

