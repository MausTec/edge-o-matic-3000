# Nobra Integration

Basic integration for Nobra devices is included with the Edge-O-Matic-3000.

### Setup
1. Turn on your Nobra device.
2. A green light indicates 'AzureFang' is enabled.
3. If the light is not green, press the outer two buttons on the control box at the same time and power cycle the device.
4. Turn the main control knob so that the Nobra device is vibrating slightly.
5. With your EOM3k, push the knob and select "Network Settings > Bluetooth Pair".
6. Select "NobraControl" to pair.
7. The EOM3k is now controlling your Nobra device.

### Configuration
The Nobra's main intensity knob essentially takes over the EOM3k's clamping of motor speed. This is why step 4 in Setup suggests to turn the intensity up until it's vibrating slightly - otherwise, it can appear the 'AzureFang' connection is not working.

Put another way, you want to configure your EOM3k's "Motor Max Speed" to be the highest it will go (255). This lets you fully adjust the intensity on the NobraControl device.

So the basic procedure after the 'AzureFang' connection is established is to enter manual mode, turn the EOM3k to max, and configure the NobraControl knobs to a suitable maximum intensity. Then you are ready for the automated modes.
