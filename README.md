
# ESP32 REPEATER CONTROLLER MANUAL
# This User Manual is for FW revision: R_1.0

## KEY FEATURES

- **WiFi Connectivity**: Access Point or Client mode for remote administration via Web browser
- **Dual Signal Detection**: 
  - RSSI level measurement 
  - Logic Signal from Carrier detection or CTCSS/DCS (external) detection
- **Reliable Operation**: Comprehensive hysteresis in both voltage domain (RSSI Mode) and time domain (both modes) ensures reliable functionality even in high noise environments
- **Timeout Protection**: "Calm-Down" feature prevents immediate reactivation after timeout
- **Audio Features**:
  - Courtesy Tone with customizable settings
  - Tail End Tone with adjustable parameters
  - CW Beacon with configurable interval and message
- **Timing Control**: User-defined timeout and repeater tail intervals
- **Display**: 2x16 LCD Display for basic status monitoring
- **Manual Control**: Beacon triggering via Web Browser button or hardware button
- **Protection Features**: Anti-Kerchunking and continuous interference protection
- **Configuration Management**: Backup for user-defined settings
- **Audio Control**: Adjustable volume levels for each audio tone type
- **Diagnostics**: Serial output for debugging and monitoring, selectable from Web page


## ABOUT REPEATER MODES
The ESP32 Repeater Controller supports two primary modes of operation:

1. **RSSI Mode**: Uses the analog RSSI input to detect incoming signals. This mode provides more precise control over signal detection using adjustable thresholds.
   - RSSI HIGH threshold: Signal must exceed this level to be considered valid (default: 1400)
   - RSSI LOW threshold: Signal must fall below this level to be considered ended (default: 1100)
   - Number of readings to average: Improves stability by averaging multiple samples, each sample is taken at 2 msec. (default: 10 samples)

2. **Carrier Detect Mode**: Uses a digital input to detect incoming signals.
   - Can be configured as Active HIGH or Active LOW depending on your receiver's squelch output

For further processing of the incoming signal, the controller implements hysteresis in both modes to prevent signal flutter and false triggering.

## ABOUT HARDWARE CONNECTIONS
The ESP32 Repeater Controller uses the following pin connections:

- **PTT Output**: Fixed Default at Pin 12 - Controls the transmitter PTT line
  - Configurable as Active HIGH or Active LOW to match your transmitter
- **Carrier Detect Input**: Fixed Default at Pin 14 - Digital input for carrier detect mode
  - Configurable as Active HIGH or Active LOW to match your receiver
- **RSSI Input**: Pin 36 - Analog input for RSSI mode
- **Audio Outputs**: Configurable GPIO pins for different audio tones:
  - **Beacon Pin**: Default GPIO 25 - Audio output for CW beacon and Morse code
  - **Courtesy Pin**: Default GPIO 25 - Audio output for courtesy tones
  - **Tail Pin**: Default GPIO 25 - Audio output for tail and timeout tones
  - Each audio output can be independently configured to use GPIO 25, 26, or 27
- **TX LED**: Pin 2 - Built-in LED that indicates transmitter activity
- **I2C LCD**: Connected to default ESP32 I2C pins (SDA: 21, SCL: 22)
  - Uses standard 16x2 LCD with I2C adapter at default address 0x27 but the user can choose other addresses and can choose to isolate the LCD for headless operation.
  
  
  
## THE COMPLETE SIGNAL FLOW - SIGNAL DETECTION AND REPEATER OPERATION SEQUENCE

### Initial Signal Detection
When a signal is detected on the input (either through RSSI measurement or logic input), the repeater begins monitoring the signal.

### Anti-Kerchunking Period
The repeater waits for the "Anti-Kerchunking Time" to ensure the signal is legitimate and not just a brief noise spike or accidental key-up.
If the signal disappears during this period, the process resets and no repeater activation occurs.

### Repeater Activation
After the Anti-Kerchunking Time elapses with continuous signal presence, the repeater activates its transmitter.
The PTT (Push-To-Talk) output is activated, and the repeater begins retransmitting the input signal.

### Signal Loss Detection
When the input signal disappears, the repeater doesn't immediately deactivate.
The repeater first waits for the "Hold Time" (Typically A-K Time + 50-75 msec.) to see if the signal quickly returns.
This prevents the repeater from deactivating during very brief pauses in transmission.

### Fragment Time Processing
If the signal remains absent after the Hold Time, the repeater enters the Fragment Time phase.
During the "Fragment Time" (default: 600ms), the repeater continues to monitor for signal return.
If the signal returns during this period, the repeater continues normal operation.
This prevents the repeater from deactivating during normal speech pauses.

### Courtesy Tone Sequence
If no signal returns during the entire Fragment Time, the repeater prepares to deactivate.
The repeater will play the "Courtesy Tone" after a very brief "Courtesy Interval" to indicate the channel is clear for another transmission.

### Tail Time
After the courtesy tone, the repeater enters the "Repeater Tail Time".
The transmitter remains active during this period, allowing users to hear the courtesy tone and providing a brief window for someone to respond.

### Final Deactivation
At the end of the Tail Time, the repeater plays Tail Tone.
The transmitter is then deactivated, and the repeater returns to standby mode.
The system resets and begins monitoring for new input signals.

### Timeout Protection and "Calm-Down"
When a signal is received and the repeater starts transmitting, a Time Out Timer (Repeater TOT) starts.
If a transmission continues beyond Time Out Timer, the timeout protection activates.
The repeater plays Tail Tone and deactivates the transmitter.
The system then waits for the input signal to completely end and a mandatory pause is enforced.
This mandatory pause is defined by the "Minimum Pause After Timeout".
During this pause period, the repeater will not activate even if a new signal is detected.
This prevents the same user from immediately reactivating the repeater after a timeout.

### Beacon Operation
If no activity occurs on the repeater for the "Beacon Interval", the repeater automatically transmits its Callsign followed by Tail Info (both set by user).
The beacon is transmitted at the configured CW speed and tone frequency.
After sending the beacon, the repeater returns to standby mode and continues monitoring for input signals.


This signal flow creates a smooth and reliable repeater operation that handles normal conversation patterns while providing protection against excessive use and meeting identification requirements.

The Beacon can be activated manually from the web page of the repeater controller or by hardware momentary press switch.

###All parameters are user defined in the controller's administration webpage. 



## ABOUT WEB INTERFACE
The ESP32 provides a web interface for configuration accessible via WiFi:

1. **WiFi Connectivity**:
   - Initially starts in Access Point mode (SSID: ESP32_Repeater)
   - Can be configured to connect to an existing WiFi network
   - Supports mDNS for easy access (http://repeater.local)

2. **Main Page**:
   - Shows current WiFi status
   - Links to WiFi Setup and Repeater Settings
   - Features consistently styled buttons with equal width and centered text
   - Includes a footer with copyright information and restart link

3. **WiFi Setup Page**:
   - Configure SSID and password for connecting to existing network
   - Settings are saved to persistent storage

4. **Repeater Settings Page**:
   - Hardware I/O Settings:
     - Receive Detection Mode (RSSI or Carrier Detect)
     - PTT Output Logic (Active HIGH or LOW)
   
   - RSSI Hysteresis:
     - HIGH and LOW thresholds
     - Number of readings to average
   
   - Timing Settings:
     - Time-Out Timer (TOT)
     - Anti-Kerchunking Time
     - Hold Time
     - Fragment Time
     - Repeater Tail Time (in seconds)
     - Minimum Pause After Timeout (in seconds)
   
   - Tone Settings:
     - Courtesy and Tail End tone frequencies and durations
     - Tone delays and intervals
   
   - Beacon Settings:
     - Interval, CW speed, and tone frequency
     - Callsign and Tail Info message
     - Beacon timer resets correctly after QSOs and timeouts
   
   - Debug Settings:
     - Enable/disable various debug outputs
     - Enable/disable courtesy tones
     - Debug information displayed with custom styling on web interface
   
   - Repeater Control:
     - Lock/unlock repeater operation
   
   - Command Controls:
     - "Apply Settings" button (red) to save changes
     - "Download Settings" button (green) to export current settings as an INI file
     - All buttons have consistent width and centered text

All settings are saved to non-volatile memory, persist through power cycles and can be downloaded via Web Browser as .ini file.

### VALUE RANGES

The ESP32 Repeater Controller enforces the following value ranges for critical parameters. If a value is set outside these ranges, it will automatically be adjusted to the default value and a notification will be displayed.

1. **RSSI Threshold Values**:
   - Range: 0 to 4000

2. **Timing Parameters**:
   - Anti-Kerchunking Time (AKtime): 10 to 1000 ms
   - Hold Time: 10 to 1000 ms
   - Fragment Time: 1 to 1500 ms
   - Time-Out Timer (ToTime): 20 to 1200 seconds
   - Courtesy Interval: 1 to 250 ms
   - Repeater Tail Time: 0 to 60 seconds
   - Minimum Pause Timer: 1 to 600 seconds

3. **Beacon Settings**:
   - Beacon Interval: 0 (disabled) to 120 minutes

4. **CW and Tone Settings**:
   - CW Speed: 20 to 40 WPM
   - CW Tone: 350 to 2400 Hz
   - Courtesy Tone Frequencies: 350 to 2400 Hz
   - Courtesy Tone Durations: 1 to 1000 ms
   - Pre-Tone Delays: 1 to 1000 ms

## DEFAULT VALUES

The following default values are used when the controller is first initialized or reset to factory settings. These values are tailored for quick test.
After fine tuning them for yoour setup, the values are saved into non-volatile memory and can be downloaded for backup from the Settings web page as "ESP32_Repeater.ini" file:

1. **Hardware Settings**:
   - UseRssiMode: 0 (Logic mode)
   - CarrierActiveHigh: 0 (Active LOW)
   - PttActiveHigh: 1 (Active HIGH)

2. **Timing Parameters**:
   - ToTime: 30 seconds
   - MinimumPauseTimer: 3 seconds
   - RepeaterTailTime: 3 seconds
   - CourtesyInterval: 10 ms
   - FragmentTime: 250 ms

3. **RSSI Thresholds**:
   - RssiHthresh: 1400
   - RssiLthresh: 1100

4. **Beacon Settings**:
   - BeacInterval: 15 minutes
   - Callsign: YO3HJV
   - CWspeed: 35 WPM
   - CWtone: 900 Hz
   - BeaconVolume: 153

5. **Tone Settings**:
   - CourtesyEnable: 1 (Enabled)
   - CourtesyTone1Freq: 1300 Hz
   - CourtesyTone2Freq: 760 Hz
   - CourtesyTone1Dur: 80 ms
   - CourtesyTone2Dur: 100 ms
   - PreTime_C1: 20 ms
   - PreTime_C2: 10 ms
   - CourtesyVolume: 117
   - TailVolume: 140

6. **Pin Assignments**:
   - BeaconPin: 25
   - CourtesyPin: 25
   - TailPin: 25

7. **Other Settings**:
   - UserLockActive: 0 (Unlocked)
   - LcdI2cAddress: 0x27
   - Debug settings (all 0 - disabled)



# ABOUT WEB SERVER AND REPEATER CORE FUNCTIONS

The ESP32 Repeater Controller is designed to efficiently handle both web server functionality and core repeater operations simultaneously:

1. **Dual-Core Architecture**:
   - The ESP32's dual-core processor allows web server tasks and repeater functions to run in parallel
   - Core repeater timing functions remain precise and unaffected by web interface activity

2. **Non-Blocking Implementation**:
   - Web server handling is implemented in a non-blocking way
   - Critical timing operations use hardware timers and millis() functions that operate independently
   - The main loop handles both repeater operations and web client requests efficiently

3. **User Interface Features**:
   - Consistent button styling across all pages with fixed width and centered text
   - Color-coded buttons: red for Apply Settings, green for Download Settings, blue for Home
   - Settings can be exported as an INI file for backup or sharing
   - Real-time status updates with auto-refresh every 3 seconds

4. **Performance Considerations**:
   - Heavy web traffic may cause slight delays in web response, but will not affect repeater timing
   - The ESP32 has sufficient processing power to handle both tasks without noticeable impact
   - Critical repeater functions are prioritized over web interface operations

This architecture ensures reliable repeater operation even while actively using the web interface for monitoring or configuration.

## SERIAL DEBUGGING

The ESP32 Repeater Controller provides comprehensive serial debugging options to help with troubleshooting and monitoring system operation.

### Accessing Serial Debug Output

1. **Connection Setup**:
   - Connect your computer to the ESP32 using a USB cable
   - Use any serial terminal program (Arduino IDE Serial Monitor, PuTTY, etc.)
   - Set baud rate to 115200 bps
   - Configure for Newline (NL) or Carriage Return + Newline (CR+NL)

2. **Debug Categories**:
   - **Main Debug**: General system status, timing events, and repeater state changes
   - **RSSI Debug**: Detailed RSSI readings and threshold comparisons
   - **Beacon Debug**: Information about beacon timing and transmission

3. **Enabling Debug Output**:
   - Navigate to the Settings page in the web interface
   - Scroll to the Debug Settings section
   - Enable the desired debug categories (Main, RSSI, Beacon)
   - Click "Apply Settings" to save changes

### Debug Output Format

Serial debug messages follow a consistent format with timestamps and category prefixes:

- **Timestamp**: [milliseconds since boot]
- **Category**: [MAIN], [RSSI], or [BEACON]
- **Message**: Descriptive text of the event or status

Example output:
```
[12345] [MAIN] Signal detected, starting Anti-Kerchunking timer
[12645] [MAIN] Anti-Kerchunking time elapsed, activating repeater
[12646] [MAIN] PTT activated
```

### Using Debug Information

1. **Troubleshooting Signal Detection**:
   - Enable RSSI Debug to monitor signal levels and threshold crossings
   - Compare actual RSSI values against configured thresholds
   - Observe hysteresis behavior in noisy environments

2. **Timing Verification**:
   - Monitor Main Debug to verify correct operation of timers
   - Confirm Anti-Kerchunking, Hold Time, and Fragment Time are working as expected
   - Verify timeout protection and minimum pause enforcement

3. **Beacon Diagnostics**:
   - Enable Beacon Debug to monitor beacon scheduling and transmission
   - Verify correct CW timing and message content

### Performance Considerations

- Enabling all debug options may slightly increase system load
- For normal operation, disable debug outputs when not needed
- Serial output does not affect critical repeater timing functions


## RECOMMENDED HARDWARE

The following ESP32 development boards are recommended for use with this controller:

- [Board 1 - ESP32 Development Board](https://www.aliexpress.com/item/1005008669669631.html)
- [Board 2 - ESP32 WROOM Module](https://www.aliexpress.com/item/1005007640283735.html)
