# ESP32 REPEATER CONTROLLER MANUAL Ver. d.10.2

## ABOUT REPEATER MODES
The ESP32 Repeater Controller supports two primary modes of operation:

1. **RSSI Mode**: Uses the analog RSSI input to detect incoming signals. This mode provides more precise control over signal detection using adjustable thresholds.
   - RSSI HIGH threshold: Signal must exceed this level to be considered valid (default: 70)
   - RSSI LOW threshold: Signal must fall below this level to be considered ended (default: 40)
   - Number of readings to average: Improves stability by averaging multiple samples (default: 10)

2. **Carrier Detect Mode**: Uses a digital input to detect incoming signals.
   - Can be configured as Active HIGH or Active LOW depending on your receiver's squelch output

The controller implements hysteresis in both modes to prevent signal flutter and false triggering.

## ABOUT HARDWARE CONNECTIONS
The ESP32 Repeater Controller uses the following pin connections:

- **PTT Output**: Pin 12 - Controls the transmitter PTT line
  - Configurable as Active HIGH or Active LOW to match your transmitter
- **Carrier Detect Input**: Pin 14 - Digital input for carrier detect mode
  - Configurable as Active HIGH or Active LOW to match your receiver
- **RSSI Input**: Pin 36 - Analog input for RSSI mode
- **CW/Tone Output**: Pin 25 - Audio output for courtesy tones and CW beacon
- **TX LED**: Pin 2 - Built-in LED that indicates transmitter activity
- **I2C LCD**: Connected to default ESP32 I2C pins (SDA: 21, SCL: 22)
  - Uses standard 16x2 LCD with I2C adapter at address 0x27

## ABOUT SIGNAL CONDITIONING
The controller implements several signal conditioning features to ensure reliable operation:

1. **Anti-Kerchunking (AKtime)**: Prevents brief transmissions from activating the repeater
   - Signal must be present for this duration before repeater activates (default: 300ms)

2. **Hold Time**: Maintains the repeater active during brief signal dropouts
   - Repeater remains active for this duration after signal loss (default: 150ms)

3. **Fragment Time**: Determines how long to wait for a signal to return before ending transmission
   - If no signal returns within this time, the courtesy tone plays (default: 600ms)

4. **RSSI Averaging**: In RSSI mode, multiple readings are averaged to reduce noise
   - Configurable from 3 to 100 samples (default: 10)

5. **Hysteresis**: Separate HIGH and LOW thresholds prevent signal flutter
   - Signal must cross HIGH threshold to activate and LOW threshold to deactivate

## ABOUT COURTESY TONES AND VARIOUS TIMERS
The controller provides configurable courtesy tones and timing parameters:

1. **Courtesy Tone**: Plays when a transmission ends
   - Frequency: Adjustable tone frequency (default: 1100Hz)
   - Duration: Adjustable tone length (default: 200ms)
   - PreTime: Delay before playing tone (default: 150ms)
   - Interval: Delay after signal loss before playing tone (default: 100ms)

2. **Tail End Tone**: Plays when the repeater returns to standby
   - Frequency: Adjustable tone frequency (default: 800Hz)
   - Duration: Adjustable tone length (default: 250ms)
   - PreTime: Delay before playing tone (default: 150ms)

3. **Repeater Tail Time**: How long the repeater remains keyed after courtesy tone
   - Adjustable duration in seconds (default: 1 second)
   - All timing values are displayed and configured in seconds for user convenience

4. **Time-Out Timer (TOT)**: Limits maximum transmission time
   - Configurable in seconds (default: 180 seconds)
   - When timeout occurs, the repeater plays the Tail End Tone and enters TOT LOCK state

5. **Minimum Pause Timer**: Enforces a minimum pause after a timeout
   - Prevents the same user from immediately re-triggering the repeater (default: 2 seconds)
   - Configured in seconds for easy understanding

## ABOUT CW BEACON
The controller includes a configurable CW beacon feature:

1. **Beacon Interval**: Time between automatic beacon transmissions
   - Configurable in minutes (default: 2 minutes)
   - Set to 0 to disable beacon

2. **CW Parameters**:
   - Speed: Adjustable in WPM (default: 28 WPM)
   - Tone: Adjustable frequency (default: 900Hz)
   - Weight: Fixed at 3.5:1 dash-to-dot ratio

3. **Beacon Message**:
   - Callsign: Primary identifier (default: YO3KSR)
   - Tail Info: Additional information (default: QRV)

4. **Beacon Behavior**:
   - Only transmits when repeater is idle
   - Resets timer after timeout or transmission
   - Updates LCD to show "BEACON" during transmission

## ABOUT LCD INDICATIONS
The 16x2 LCD display provides the following information:

1. **Top Row**:
   - Left side: Mode indicator ("RSSI" or "LOGI")
   - Right side: Current status
     - "STAND BY": Repeater idle
     - "REPEATING": Repeater active
     - "ToT LOCK": Timeout occurred, waiting for signal to end or in pause period
     - "USER LOCK": Repeater manually disabled via web interface
     - "BEACON": Beacon transmission in progress

2. **Bottom Row**:
   - Left side: Callsign
   - Right side: Beacon interval (3 digits)

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
   - Includes a footer disclaimer: "By Adrian YO3HJV, a humble trace on the Earth"

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
   
   - Form Controls:
     - "Apply Settings" button (red) to save changes
     - "Download Settings" button (green) to export current settings as an INI file
     - All buttons have consistent width and centered text

All settings are saved to non-volatile memory and persist through power cycles.

## ABOUT WEB SERVER AND REPEATER COEXISTENCE

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
