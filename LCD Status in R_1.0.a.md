# LCD Status Display System
## in R_1.0.a

The ESP32 Repeater Controller features a comprehensive LCD status display system that provides real-time information about the repeater's operational state. The display is updated based on a priority-ordered state machine that ensures the most relevant information is always shown.

## LCD Display Format

The 16x2 LCD display shows:
- **Top Row**: Detection mode (RSSI/LOGI) on the left, current status on the right
- **Bottom Row**: Repeater callsign on the left, beacon interval on the right

## Status Messages

The controller displays the following status messages in order of priority:

1. **USER LOCK** - Indicates the repeater has been manually locked by a user via the web interface
2. **ToT LOCK** - Shown when a transmission has exceeded the timeout limit and the controller is waiting for the signal to end
3. **CALM DOWN** - Displayed during the mandatory pause period after a timeout event
4. **BEACON** - Shown when the repeater is transmitting its identification beacon
5. **TAIL** - Indicates the repeater is in the tail period after a transmission
6. **REPEATING** - Shown when the repeater is actively receiving and retransmitting a signal
7. **STAND BY** - The default idle state when no signals are being processed
