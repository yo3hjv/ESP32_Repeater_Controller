/*
 * ESP32 Repeater Controller
 * Migrated from ATMEGA328 to ESP32 WROOM platform
 * v1.0
 *
 * Based on original code by Adrian Florescu YO3HJV
 * Modified for ESP32 with WiFi functionality
 */

// WiFi Libraries
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>

// Additional libraries for repeater functionality
#include <Preferences.h>  // For storing settings

// WiFi Configuration
const char* ap_ssid = "ESP32_Repeater";
const char* ap_password = "1234";

// Web Server on port 80
WebServer server(80);

// Global variables for WiFi mode
bool apMode = true;  // Start in AP mode by default
String ssid;
String password;

// Preferences object for storing settings
Preferences preferences;

// Pin Definitions for Repeater Controller
const int PttPin = 12;           // PTT digital output pin
const int CarDetPin = 14;        // Carrier Detect Logic pin (changed from original pin 6)
const int RssiPin = 36;          // Read RSSI analog input (ESP32 ADC pin, changed from A2)
const int CW_pin = 25;           // Pin for audio Beacons Courtesy tone output (changed from pin 5)
const int TxLedPin = 2;          // Built-in LED on most ESP32 boards

// Global variables for repeater operation
bool CarDetON = false;           // Carrier detect state
bool RssiDetON = false;          // RSSI detect state
bool ReceiveNOW = false;         // Combined receive state
bool RepeatNOW = false;          // Validated receive state
bool qsoON = false;              // QSO in progress

// Configuration variables (will be loaded from preferences)
int RssiHthresh = 2500;     // RSSI high threshold
int RssiLthresh = 2000;     // RSSI low threshold
int AKtime = 400;           // Anti-kerchunking time in ms
int HoldTime = 200;         // Hold time in ms
int fragTime = 200;         // Fragment time in ms
int ToTime = 180;           // Timeout time in seconds
int CourtesyInterval = 500; // Courtesy tone delay in ms
int RepeaterTailTime = 1000; // Repeater tail time in ms
int BeacInterval = 10;      // Beacon interval in minutes
int CWspeed = 18;           // CW speed in WPM
int CWtone = 700;           // CW tone frequency in Hz
int CourtesyTone1Freq = 800; // Courtesy tone 1 frequency in Hz
int CourtesyTone2Freq = 600; // Courtesy tone 2 (EndTone) frequency in Hz
int CourtesyTone1Dur = 200;  // Courtesy tone 1 duration in ms
int CourtesyTone2Dur = 150;  // Courtesy tone 2 (EndTone) duration in ms
bool useRssiMode = false;        // Mode selection
bool CarrierActiveHigh = false;  // Carrier detect active high/low

// Morse code definitions
struct t_mtab { char c, pat; };
struct t_mtab morsetab[] = {
    {'.', 106},
    {',', 115},
    {'?', 76},
    {'/', 41},
    {'A', 6},
    {'B', 17},
    {'C', 21},
    {'D', 9},
    {'E', 2},
    {'F', 20},
    {'G', 11},
    {'H', 16},
    {'I', 4},
    {'J', 30},
    {'K', 13},
    {'L', 18},
    {'M', 7},
    {'N', 5},
    {'O', 15},
    {'P', 22},
    {'Q', 27},
    {'R', 10},
    {'S', 8},
    {'T', 3},
    {'U', 12},
    {'V', 24},
    {'W', 14},
    {'X', 25},
    {'Y', 29},
    {'Z', 19},
    {'1', 62},
    {'2', 60},
    {'3', 56},
    {'4', 48},
    {'5', 32},
    {'6', 33},
    {'7', 35},
    {'8', 39},
    {'9', 47},
    {'0', 63}
};
#define N_MORSE  (sizeof(morsetab)/sizeof(morsetab[0]))
#define DOTLEN  (1200/CWspeed)
#define DASHLEN  (3.5*(1200/CWspeed))  // CW weight 3.5 / 1

// Beacon message settings (will be loaded from preferences)
String Callsign = "YO3KSR";   // Default callsign
String TailInfo = "QRV";      // Default tail info

// Timing variables
unsigned long previousMillis = 0;        // For beacon timing
unsigned long receiveStartTime = 0;      // When signal first detected
unsigned long receiveEndTime = 0;        // When signal last ended
unsigned long repeatStartTime = 0;       // When repeater activated

// RSSI validation variables
unsigned long rssiHighStartTime = 0;     // When signal first went above high threshold
unsigned long rssiLowStartTime = 0;      // When signal first went below low threshold
bool signalAboveHigh = false;            // Tracking if signal is above high threshold
bool signalBelowLow = false;             // Tracking if signal is below low threshold

// Debug flags
bool debugMain = true;         // Debug main functionality
bool debugCarrDetect = false;  // Debug carrier detect
bool debugRssiDetect = false;  // Debug RSSI detect
bool debugBeacon = false;      // Debug beacon

// Function prototypes
void setupWiFi();
void handleRoot();
void handleWiFiSetup();
void handleWiFiSave();
void handleSettings();
void handleSettingsSave();
void handleNotFound();

// Repeater function prototypes
void CarrDetect();
void RssiDetect();
void RepeaterMode();
void RxValidation();
void Repeat();
void Ptt(bool state);
void Beacon();
void Courtesy();
void EndTone();
void Tot();
void Info();

// CW function prototypes
void dash();
void dit();
void send(char c);
void sendmsg(const char *str);

void setup() {
  // Start serial communication
  Serial.begin(115200);
  Serial.println("\nESP32 Repeater Controller Starting...");
  

  
  // Initialize SPIFFS for storing configuration
  if(!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
  }
  
  // Initialize Preferences
  preferences.begin("repeater", false);
  
  // Load settings from preferences
  loadSettings();
  
  // Setup pins
  pinMode(PttPin, OUTPUT);
  pinMode(CarDetPin, INPUT_PULLUP); // Restored to original INPUT_PULLUP
  pinMode(CW_pin, OUTPUT);
  pinMode(TxLedPin, OUTPUT);
  

  
  // Initialize outputs
  digitalWrite(PttPin, LOW);
  digitalWrite(TxLedPin, LOW);
  
  // Setup WiFi
  setupWiFi();
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/wifi", HTTP_GET, handleWiFiSetup);
  server.on("/wifisave", HTTP_POST, handleWiFiSave);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/settingssave", HTTP_POST, handleSettingsSave);
  server.onNotFound(handleNotFound);
  
  // Start web server
  server.begin();
  Serial.println("HTTP server started");
  
  // Initial debug message
  if (debugMain) {
    Serial.println("Repeater mode: " + String(useRssiMode ? "RSSI" : "Carrier Detect"));
    Serial.println("Repeater ready");
  }
}

void loop() {

  
  // Handle web server clients
  server.handleClient();
  
  // Repeater controller logic
  if (useRssiMode) {
    RssiDetect();  // RSSI mode
  } else {
    CarrDetect();  // Carrier detect mode
  }
  
  // Process repeater logic
  RepeaterMode();
  RxValidation();
  Repeat();
  
  // Check for beacon timing
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= (BeacInterval * 60000) && !RepeatNOW) {
    previousMillis = currentMillis;
    Beacon();
  }
}

// Load settings from preferences
void loadSettings() {
  // Load repeater settings
  RssiHthresh = preferences.getInt("RssiHthresh", 12);
  RssiLthresh = preferences.getInt("RssiLthresh", 9);
  AKtime = preferences.getInt("AKtime", 400);
  HoldTime = preferences.getInt("HoldTime", 200);
  fragTime = preferences.getInt("fragTime", 500);
  ToTime = preferences.getInt("ToTime", 180);
  BeacInterval = preferences.getInt("BeacInterval", 7);
  CourtesyInterval = preferences.getInt("CourtesyInt", 50);
  RepeaterTailTime = preferences.getInt("RepTailTime", 1000);
  CWspeed = preferences.getInt("CWspeed", 20);
  CWtone = preferences.getInt("CWtone", 700);
  CourtesyTone1Freq = preferences.getInt("CTone1Freq", 800);
  CourtesyTone2Freq = preferences.getInt("CTone2Freq", 600);
  CourtesyTone1Dur = preferences.getInt("CTone1Dur", 200);
  CourtesyTone2Dur = preferences.getInt("CTone2Dur", 150);
  
  // Load boolean settings
  useRssiMode = preferences.getBool("useRssiMode", false);
  CarrierActiveHigh = preferences.getBool("CarrActHigh", false);
  
  // Load strings
  Callsign = preferences.getString("Callsign", "YO3KSR");
  TailInfo = preferences.getString("TailInfo", "QRV");
  
  // Load debug settings
  debugMain = preferences.getBool("debugMain", true);
  debugCarrDetect = preferences.getBool("debugCarr", false);
  debugRssiDetect = preferences.getBool("debugRssi", false);
  debugBeacon = preferences.getBool("debugBeacon", false);
}

// Save settings to preferences
void saveSettings() {
  // Save repeater settings
  preferences.putInt("RssiHthresh", RssiHthresh);
  preferences.putInt("RssiLthresh", RssiLthresh);
  preferences.putInt("AKtime", AKtime);
  preferences.putInt("HoldTime", HoldTime);
  preferences.putInt("fragTime", fragTime);
  preferences.putInt("ToTime", ToTime);
  preferences.putInt("BeacInterval", BeacInterval);
  preferences.putInt("CourtesyInt", CourtesyInterval);
  preferences.putInt("RepTailTime", RepeaterTailTime);
  preferences.putInt("CWspeed", CWspeed);
  preferences.putInt("CWtone", CWtone);
  preferences.putInt("CTone1Freq", CourtesyTone1Freq);
  preferences.putInt("CTone2Freq", CourtesyTone2Freq);
  preferences.putInt("CTone1Dur", CourtesyTone1Dur);
  preferences.putInt("CTone2Dur", CourtesyTone2Dur);
  
  // Save boolean settings
  preferences.putBool("useRssiMode", useRssiMode);
  preferences.putBool("CarrActHigh", CarrierActiveHigh);
  
  // Save strings
  preferences.putString("Callsign", Callsign);
  preferences.putString("TailInfo", TailInfo);
  
  // Save debug settings
  preferences.putBool("debugMain", debugMain);
  preferences.putBool("debugCarr", debugCarrDetect);
  preferences.putBool("debugRssi", debugRssiDetect);
  preferences.putBool("debugBeacon", debugBeacon);
}

// Carrier Detect function
void CarrDetect() {
  int carrierState = digitalRead(CarDetPin);
  
  // Determine if carrier is active based on user setting
  bool carrierActive = (CarrierActiveHigh) ? (carrierState == HIGH) : (carrierState == LOW);
  
  // Set CarDetON based on carrier state
  CarDetON = carrierActive;
  
  if (debugCarrDetect) {
    Serial.print("Carrier: ");
    Serial.println(CarDetON ? "ON" : "OFF");
  }
}

// RSSI Detect function
void RssiDetect() {
  // ESP32 ADC has 12-bit resolution (0-4095)
  int rssiValue1 = analogRead(RssiPin);
  int rssiValue2 = analogRead(RssiPin);
  int rssiValue3 = analogRead(RssiPin);
  int RssiVal = (rssiValue1 + rssiValue2 + rssiValue3) / 3;
  
  // Map to a similar scale as the original code for compatibility
  RssiVal = map(RssiVal, 0, 4095, 0, 100);
  
  // Apply hysteresis logic with time validation
  unsigned long currentMillis = millis();
  
  // Check if signal is above high threshold
  if (RssiVal > RssiHthresh) {
    if (!signalAboveHigh) {
      // Signal just crossed above high threshold
      signalAboveHigh = true;
      rssiHighStartTime = currentMillis;
    }
    // Reset the low threshold timer since we're above high threshold
    signalBelowLow = false;
    
    // Check if signal has been above threshold long enough to activate
    if (currentMillis - rssiHighStartTime >= AKtime) {
      RssiDetON = true;
    }
  }
  // Check if signal is below low threshold
  else if (RssiVal < RssiLthresh) {
    if (!signalBelowLow) {
      // Signal just crossed below low threshold
      signalBelowLow = true;
      rssiLowStartTime = currentMillis;
    }
    // Reset the high threshold timer since we're below low threshold
    signalAboveHigh = false;
    
    // Check if signal has been below threshold long enough to deactivate
    if (currentMillis - rssiLowStartTime >= HoldTime) {
      RssiDetON = false;
    }
  }
  // Signal is between thresholds - maintain current state
  else {
    // Reset both timers since we're in the hysteresis zone
    signalAboveHigh = false;
    signalBelowLow = false;
  }
  
  if (debugRssiDetect) {
    Serial.print("RSSI: ");
    Serial.print(RssiVal);
    Serial.print(", Above High: ");
    Serial.print(signalAboveHigh);
    Serial.print(", Below Low: ");
    Serial.print(signalBelowLow);
    Serial.print(", RSSI Detect: ");
    Serial.println(RssiDetON ? "ON" : "OFF");
  }
}

// Repeater Mode function
void RepeaterMode() {
  // Set ReceiveNOW based on selected mode
  if (useRssiMode) {
    ReceiveNOW = RssiDetON;
  } else {
    ReceiveNOW = CarDetON;
  }
}

// RX Validation function
void RxValidation() {
  unsigned long currentMillis = millis();
  
  // Signal just became active
  if (ReceiveNOW && !RepeatNOW) {
    if (receiveStartTime == 0) {
      receiveStartTime = currentMillis;
    }
    
    // Check if signal has been present long enough (AKtime - Anti-Kerchunking)
    if (currentMillis - receiveStartTime >= AKtime) {
      RepeatNOW = true;
      if (debugMain) {
        Serial.println("Signal validated - Repeater activated");
      }
    }
  }
  // Signal just became inactive
  else if (!ReceiveNOW && RepeatNOW) {
    if (receiveEndTime == 0) {
      receiveEndTime = currentMillis;
    }
    
    // Check if signal has been absent long enough (HoldTime)
    if (currentMillis - receiveEndTime >= HoldTime) {
      RepeatNOW = false;
      receiveStartTime = 0;
      if (debugMain) {
        Serial.println("Signal lost - Repeater deactivated");
      }
    }
  }
  // Signal state changed back to active before timeout
  else if (ReceiveNOW && receiveEndTime != 0) {
    // Reset end time if signal returns
    receiveEndTime = 0;
  }
  // Signal state changed back to inactive before validation
  else if (!ReceiveNOW && receiveStartTime != 0 && !RepeatNOW) {
    // Reset start time if signal disappears before validation
    receiveStartTime = 0;
  }
}

// Repeat function
void Repeat() {
  static bool lastRepeatState = false;
  static unsigned long timeoutStartTime = 0;
  static unsigned long fragmentStartTime = 0;
  static unsigned long courtesyStartTime = 0;
  static unsigned long tailStartTime = 0;
  static bool courtesyPending = false;
  static bool tailActive = false;
  unsigned long currentMillis = millis();
  
  // State changed from inactive to active
  if (RepeatNOW && !lastRepeatState) {
    qsoON = true;
    Ptt(true);
    timeoutStartTime = currentMillis;
    lastRepeatState = true;
    // Cancel any pending courtesy tone or tail
    courtesyPending = false;
    tailActive = false;
    if (debugMain) {
      Serial.println("QSO started");
    }
  }
  // State changed from active to inactive
  else if (!RepeatNOW && lastRepeatState) {
    // Check if this is a brief interruption (fragment)
    if (fragmentStartTime == 0) {
      fragmentStartTime = currentMillis;
    }
    
    // If fragment time exceeded, start end of transmission sequence
    if (currentMillis - fragmentStartTime >= fragTime) {
      // Set up courtesy tone to play after interval (non-blocking)
      courtesyStartTime = currentMillis;
      courtesyPending = true;
      lastRepeatState = false;
      fragmentStartTime = 0;
      if (debugMain) {
        Serial.println("QSO ended, courtesy tone pending");
      }
    }
  }
  // Repeater active, check for timeout
  else if (RepeatNOW && lastRepeatState) {
    // Reset fragment timer if we're still active
    fragmentStartTime = 0;
    
    // Check for timeout
    if (currentMillis - timeoutStartTime >= (ToTime * 1000)) {
      Tot();
      qsoON = false;
      Ptt(false);
      lastRepeatState = false;
      courtesyPending = false;
      tailActive = false;
      if (debugMain) {
        Serial.println("Timeout occurred");
      }
    }
  }
  // Repeater inactive, reset fragment timer
  else if (!RepeatNOW && !lastRepeatState) {
    fragmentStartTime = 0;
  }
  
  // Handle courtesy tone after delay (non-blocking)
  if (courtesyPending && (currentMillis - courtesyStartTime >= CourtesyInterval)) {
    Courtesy();
    courtesyPending = false;
    tailActive = true;
    tailStartTime = currentMillis;
    if (debugMain) {
      Serial.println("Courtesy tone played, tail active");
    }
  }
  
  // Handle tail time and end tone (non-blocking)
  if (tailActive && (currentMillis - tailStartTime >= RepeaterTailTime)) {
    EndTone();
    qsoON = false;
    Ptt(false);
    tailActive = false;
    if (debugMain) {
      Serial.println("Tail time expired, repeater going to standby");
    }
  }
}

// PTT function
void Ptt(bool state) {
  digitalWrite(PttPin, state ? HIGH : LOW);
  digitalWrite(TxLedPin, state ? HIGH : LOW);
}

// Beacon function
void Beacon() {
  if (debugBeacon) {
    Serial.println("Sending beacon");
  }
  
  // Only send beacon if not currently repeating
  if (!RepeatNOW) {
    Ptt(true);
    delay(50);
    
    // Send callsign
    sendmsg(Callsign.c_str());
    delay(7 * DOTLEN);
    
    // Send tail info
    sendmsg(TailInfo.c_str());
    delay(3 * DOTLEN);
    
    Ptt(false);
  }
}

// Courtesy tone function - signals end of a transmission
void Courtesy() {
  tone(CW_pin, CourtesyTone1Freq, CourtesyTone1Dur);
  delay(CourtesyTone1Dur + 10);
  
  if (debugMain) {
    Serial.println("Courtesy tone played - end of transmission");
  }
}

// End tone function - signals repeater going to standby
void EndTone() {
  tone(CW_pin, CourtesyTone2Freq, CourtesyTone2Dur);
  delay(CourtesyTone2Dur + 10);
  tone(CW_pin, CourtesyTone2Freq - 200, CourtesyTone2Dur);
  delay(CourtesyTone2Dur + 10);
  
  if (debugMain) {
    Serial.println("End tone played - repeater going to standby");
  }
}

// Timeout tone function
void Tot() {
  tone(CW_pin, 500, 200);
  delay(250);
  tone(CW_pin, 500, 200);
  delay(250);
}

// Info function (placeholder)
void Info() {
  // This will be developed later
}

// CW generation routines
void dash() {
  tone(CW_pin, CWtone); // Use configurable tone frequency
  delay(DASHLEN);
  noTone(CW_pin);
  delay(DOTLEN);
}

void dit() {
  tone(CW_pin, CWtone); // Use configurable tone frequency
  delay(DOTLEN);
  noTone(CW_pin);
  delay(DOTLEN);
}

void send(char c) {
  int i;
  if (c == ' ') {
    delay(7 * DOTLEN);
    return;
  }
  
  for (i = 0; i < N_MORSE; i++) {
    if (morsetab[i].c == c) {
      unsigned char p = morsetab[i].pat;
      while (p != 1) {
        if (p & 1)
          dash();
        else
          dit();
        p = p / 2;
      }
      delay(2 * DOTLEN);
      return;
    }
  }
}

void sendmsg(const char *str) {
  char c;
  while ((c = *str++)) {
    send(c);
  }
}

// WiFi Setup Function
void setupWiFi() {
  // Try to load saved WiFi credentials
  if (SPIFFS.exists("/wifi.txt")) {
    File file = SPIFFS.open("/wifi.txt", "r");
    if (file) {
      ssid = file.readStringUntil('\n');
      password = file.readStringUntil('\n');
      ssid.trim();
      password.trim();
      file.close();
      
      // Try to connect to WiFi with saved credentials
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid.c_str(), password.c_str());
      
      Serial.print("Connecting to WiFi ");
      Serial.print(ssid);
      
      // Wait for connection with timeout
      int timeout = 0;
      while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        Serial.print(".");
        timeout++;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("Connected to WiFi. IP address: ");
        Serial.println(WiFi.localIP());
        apMode = false;
        
        // Setup mDNS responder
        if (MDNS.begin("repeater")) {
          Serial.println("MDNS responder started");
          MDNS.addService("http", "tcp", 80);
        }
        
        return;
      } else {
        Serial.println("\nFailed to connect to WiFi. Starting AP mode.");
      }
    }
  }
  
  // If we reach here, either no saved credentials or connection failed
  // Start AP mode
  WiFi.mode(WIFI_AP);
  
  // Ensure AP is created with correct name and password
  bool apStarted = WiFi.softAP("ESP32_Repeater", "1234");
  
  Serial.println("WiFi AP mode started: " + String(apStarted ? "Success" : "Failed"));
  Serial.print("SSID: ESP32_Repeater");
  Serial.print(" (Password: 1234)");
  Serial.print("\nIP address: ");
  Serial.println(WiFi.softAPIP());
  
  apMode = true;
}

// Root page handler
void handleRoot() {
  String html = "<!DOCTYPE html>\n"
              "<html>\n"
              "<head>\n"
              "  <title>ESP32 Repeater Controller</title>\n"
              "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
              "  <style>\n"
              "    body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }\n"
              "    h1 { color: #0066cc; }\n"
              "    .btn { display: inline-block; padding: 10px 20px; margin: 10px 0; background-color: #0066cc; color: white; text-decoration: none; border-radius: 4px; }\n"
              "    .info { background-color: #e0f0ff; padding: 15px; border-radius: 4px; }\n"
              "  </style>\n"
              "</head>\n"
              "<body>\n"
              "  <h1>ESP32 Repeater Controller</h1>\n";
  
  if (apMode) {
    html += "  <div class=\"info\">\n"
            "    <p>Device is in Access Point mode.</p>\n"
            "    <p>SSID: " + String(ap_ssid) + "</p>\n"
            "    <p>Connect to this WiFi network and configure the device.</p>\n"
            "  </div>\n";
  } else {
    html += "  <div class=\"info\">\n"
            "    <p>Device is connected to WiFi network: " + ssid + "</p>\n"
            "    <p>IP address: " + WiFi.localIP().toString() + "</p>\n"
            "  </div>\n";
  }
  
  html += "  <p><a href=\"/wifi\" class=\"btn\">WiFi Setup</a></p>\n"
          "  <p><a href=\"/settings\" class=\"btn\">Repeater Settings</a></p>\n"
          "</body>\n"
          "</html>\n";
  
  server.send(200, "text/html", html);
}

// WiFi setup page handler
void handleWiFiSetup() {
  String html = "<!DOCTYPE html>\n"
              "<html>\n"
              "<head>\n"
              "  <title>WiFi Setup</title>\n"
              "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
              "  <style>\n"
              "    body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }\n"
              "    h1 { color: #0066cc; }\n"
              "    .form-group { margin-bottom: 15px; }\n"
              "    label { display: block; margin-bottom: 5px; }\n"
              "    input[type=text], input[type=password] { width: 100%; padding: 8px; box-sizing: border-box; }\n"
              "    input[type=submit] { padding: 10px 20px; background-color: #0066cc; color: white; border: none; border-radius: 4px; cursor: pointer; }\n"
              "    .back { display: inline-block; margin-top: 15px; color: #0066cc; }\n"
              "  </style>\n"
              "</head>\n"
              "<body>\n"
              "  <h1>WiFi Setup</h1>\n"
              "  <form method=\"POST\" action=\"/wifisave\">\n"
              "    <div class=\"form-group\">\n"
              "      <label for=\"ssid\">SSID:</label>\n"
              "      <input type=\"text\" id=\"ssid\" name=\"ssid\" value=\"" + ssid + "\">\n"
              "    </div>\n"
              "    <div class=\"form-group\">\n"
              "      <label for=\"password\">Password:</label>\n"
              "      <input type=\"password\" id=\"password\" name=\"password\" value=\"" + password + "\">\n"
              "    </div>\n"
              "    <input type=\"submit\" value=\"Save\">\n"
              "  </form>\n"
              "  <p><a href=\"/\" class=\"back\">Back to Home</a></p>\n"
              "</body>\n"
              "</html>\n";
  
  server.send(200, "text/html", html);
}

// WiFi save handler
void handleWiFiSave() {
  ssid = server.arg("ssid");
  password = server.arg("password");
  
  // Save to SPIFFS
  File file = SPIFFS.open("/wifi.txt", "w");
  if (file) {
    file.println(ssid);
    file.println(password);
    file.close();
    Serial.println("WiFi credentials saved");
  } else {
    Serial.println("Failed to open file for writing");
  }
  
  String html = "<!DOCTYPE html>\n"
              "<html>\n"
              "<head>\n"
              "  <title>WiFi Setup</title>\n"
              "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
              "  <style>\n"
              "    body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }\n"
              "    h1 { color: #0066cc; }\n"
              "    .message { background-color: #e0f0ff; padding: 15px; border-radius: 4px; }\n"
              "    .back { display: inline-block; margin-top: 15px; color: #0066cc; }\n"
              "  </style>\n"
              "  <meta http-equiv=\"refresh\" content=\"5; URL=/\">\n"
              "</head>\n"
              "<body>\n"
              "  <h1>WiFi Setup</h1>\n"
              "  <div class=\"message\">\n"
              "    <p>Settings saved. The device will now attempt to connect to the new WiFi network.</p>\n"
              "    <p>You will be redirected to the home page in 5 seconds.</p>\n"
              "  </div>\n"
              "  <p><a href=\"/\" class=\"back\">Back to Home</a></p>\n"
              "</body>\n"
              "</html>\n";
  
  server.send(200, "text/html", html);
  
  // Attempt to connect with new credentials
  delay(1000);
  setupWiFi();
}

// Settings page handler
void handleSettings() {
  // Create HTML using String concatenation to avoid C++ string literal limitations
  String html = "<!DOCTYPE html>\n";
  html += "<html>\n";
  html += "<head>\n";
  html += "  <title>Repeater Settings</title>\n";
  html += "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
  html += "  <style>\n";
  html += "    body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }\n";
  html += "    h1, h2 { color: #0066cc; }\n";
  html += "    .form-group { margin-bottom: 15px; }\n";
  html += "    label { display: block; margin-bottom: 5px; }\n";
  html += "    input[type=text], input[type=number] { width: 100%; padding: 8px; box-sizing: border-box; }\n";
  html += "    input[type=checkbox] { margin-right: 8px; }\n";
  html += "    input[type=submit] { padding: 10px 20px; background-color: #0066cc; color: white; border: none; border-radius: 4px; cursor: pointer; }\n";
  html += "    .back { display: inline-block; margin-top: 15px; color: #0066cc; }\n";
  html += "    .section { background-color: #f5f5f5; padding: 15px; margin-bottom: 20px; border-radius: 4px; }\n";
  html += "  </style>\n";
  html += "</head>\n";
  html += "<body>\n";
  html += "  <h1>Repeater Settings</h1>\n";
  html += "  <form method=\"POST\" action=\"/settingssave\">\n";
  
  // Repeater Mode Section
  html += "    <div class=\"section\">\n";
  html += "      <h2>Repeater Mode</h2>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label><input type=\"checkbox\" name=\"useRssiMode\" ";
  if (useRssiMode) {
    html += "checked";
  }
  html += "> Use RSSI Mode (unchecked = Carrier Detect)</label>\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label><input type=\"checkbox\" name=\"CarrierActiveHigh\" ";
  if (CarrierActiveHigh) {
    html += "checked";
  }
  html += "> Carrier Detect Active High (unchecked = Active Low)</label>\n";
  html += "      </div>\n";
  html += "    </div>\n";
  
  // RSSI Settings Section
  html += "    <div class=\"section\">\n";
  html += "      <h2>RSSI Settings</h2>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"RssiHthresh\">RSSI High Threshold:</label>\n";
  html += "        <input type=\"number\" id=\"RssiHthresh\" name=\"RssiHthresh\" value=\"" + String(RssiHthresh) + "\">\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"RssiLthresh\">RSSI Low Threshold:</label>\n";
  html += "        <input type=\"number\" id=\"RssiLthresh\" name=\"RssiLthresh\" value=\"" + String(RssiLthresh) + "\">\n";
  html += "      </div>\n";
  html += "    </div>\n";
  
  // Timing Settings Section
  html += "    <div class=\"section\">\n";
  html += "      <h2>Timing Settings</h2>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"AKtime\">Anti-Kerchunking Time (ms):</label>\n";
  html += "        <input type=\"number\" id=\"AKtime\" name=\"AKtime\" value=\"" + String(AKtime) + "\">\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"HoldTime\">Hold Time (ms):</label>\n";
  html += "        <input type=\"number\" id=\"HoldTime\" name=\"HoldTime\" value=\"" + String(HoldTime) + "\">\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"fragTime\">Fragment Time (ms):</label>\n";
  html += "        <input type=\"number\" id=\"fragTime\" name=\"fragTime\" value=\"" + String(fragTime) + "\">\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"ToTime\">Timeout Time (seconds):</label>\n";
  html += "        <input type=\"number\" id=\"ToTime\" name=\"ToTime\" value=\"" + String(ToTime) + "\">\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"CourtesyInterval\">Courtesy Tone Delay (ms):</label>\n";
  html += "        <input type=\"number\" id=\"CourtesyInterval\" name=\"CourtesyInterval\" value=\"" + String(CourtesyInterval) + "\">\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"RepeaterTailTime\">Repeater Tail Time (ms):</label>\n";
  html += "        <input type=\"number\" id=\"RepeaterTailTime\" name=\"RepeaterTailTime\" value=\"" + String(RepeaterTailTime) + "\">\n";
  html += "      </div>\n";
  html += "    </div>\n";
  
  // Tone Settings Section
  html += "    <div class=\"section\">\n";
  html += "      <h2>Tone Settings</h2>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"CourtesyTone1Freq\">Courtesy Tone 1 Frequency (Hz):</label>\n";
  html += "        <input type=\"number\" id=\"CourtesyTone1Freq\" name=\"CourtesyTone1Freq\" value=\"" + String(CourtesyTone1Freq) + "\">\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"CourtesyTone1Dur\">Courtesy Tone 1 Duration (ms):</label>\n";
  html += "        <input type=\"number\" id=\"CourtesyTone1Dur\" name=\"CourtesyTone1Dur\" value=\"" + String(CourtesyTone1Dur) + "\">\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"CourtesyTone2Freq\">Courtesy Tone 2 Frequency (Hz):</label>\n";
  html += "        <input type=\"number\" id=\"CourtesyTone2Freq\" name=\"CourtesyTone2Freq\" value=\"" + String(CourtesyTone2Freq) + "\">\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"CourtesyTone2Dur\">Courtesy Tone 2 Duration (ms):</label>\n";
  html += "        <input type=\"number\" id=\"CourtesyTone2Dur\" name=\"CourtesyTone2Dur\" value=\"" + String(CourtesyTone2Dur) + "\">\n";
  html += "      </div>\n";
  html += "    </div>\n";
  
  // Beacon Settings Section
  html += "    <div class=\"section\">\n";
  html += "      <h2>Beacon Settings</h2>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"BeacInterval\">Beacon Interval (minutes):</label>\n";
  html += "        <input type=\"number\" id=\"BeacInterval\" name=\"BeacInterval\" value=\"" + String(BeacInterval) + "\">\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"CWspeed\">CW Speed (WPM):</label>\n";
  html += "        <input type=\"number\" id=\"CWspeed\" name=\"CWspeed\" value=\"" + String(CWspeed) + "\">\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"CWtone\">CW Tone Frequency (Hz):</label>\n";
  html += "        <input type=\"number\" id=\"CWtone\" name=\"CWtone\" value=\"" + String(CWtone) + "\">\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"Callsign\">Callsign (max 8 chars):</label>\n";
  html += "        <input type=\"text\" id=\"Callsign\" name=\"Callsign\" maxlength=\"8\" value=\"" + String(Callsign) + "\">\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label for=\"TailInfo\">Tail Info (max 10 chars):</label>\n";
  html += "        <input type=\"text\" id=\"TailInfo\" name=\"TailInfo\" maxlength=\"10\" value=\"" + String(TailInfo) + "\">\n";
  html += "      </div>\n";
  html += "    </div>\n";
  
  // Debug Settings Section
  html += "    <div class=\"section\">\n";
  html += "      <h2>Debug Settings</h2>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label><input type=\"checkbox\" name=\"debugMain\" ";
  if (debugMain) {
    html += "checked";
  }
  html += "> Main Debug</label>\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label><input type=\"checkbox\" name=\"debugCarrDetect\" ";
  if (debugCarrDetect) {
    html += "checked";
  }
  html += "> Carrier Detect Debug</label>\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label><input type=\"checkbox\" name=\"debugRssiDetect\" ";
  if (debugRssiDetect) {
    html += "checked";
  }
  html += "> RSSI Debug</label>\n";
  html += "      </div>\n";
  html += "      <div class=\"form-group\">\n";
  html += "        <label><input type=\"checkbox\" name=\"debugBeacon\" ";
  if (debugBeacon) {
    html += "checked";
  }
  html += "> Beacon Debug</label>\n";
  html += "      </div>\n";
  html += "    </div>\n";
  
  // Submit Button
  html += "    <input type=\"submit\" value=\"Save Settings\">\n";
  html += "  </form>\n";
  html += "  <p><a href=\"/\" class=\"back\">Back to Home</a></p>\n";
  html += "</body>\n";
  html += "</html>\n";
  
  server.send(200, "text/html", html);
}

// Settings save handler
void handleSettingsSave() {
  // Get mode settings
  useRssiMode = server.hasArg("useRssiMode");
  CarrierActiveHigh = server.hasArg("CarrierActiveHigh");
  
  // Get RSSI settings
  if (server.hasArg("RssiHthresh")) {
    RssiHthresh = server.arg("RssiHthresh").toInt();
  }
  if (server.hasArg("RssiLthresh")) {
    RssiLthresh = server.arg("RssiLthresh").toInt();
  }
  
  // Get timing settings
  if (server.hasArg("AKtime")) {
    AKtime = server.arg("AKtime").toInt();
  }
  if (server.hasArg("HoldTime")) {
    HoldTime = server.arg("HoldTime").toInt();
  }
  if (server.hasArg("fragTime")) {
    fragTime = server.arg("fragTime").toInt();
  }
  if (server.hasArg("ToTime")) {
    ToTime = server.arg("ToTime").toInt();
  }
  if (server.hasArg("CourtesyInterval")) {
    CourtesyInterval = server.arg("CourtesyInterval").toInt();
  }
  if (server.hasArg("RepeaterTailTime")) {
    RepeaterTailTime = server.arg("RepeaterTailTime").toInt();
  }
  
  // Get beacon settings
  if (server.hasArg("BeacInterval")) {
    BeacInterval = server.arg("BeacInterval").toInt();
  }
  if (server.hasArg("CWspeed")) {
    CWspeed = server.arg("CWspeed").toInt();
  }
  if (server.hasArg("CWtone")) {
    CWtone = server.arg("CWtone").toInt();
  }
  
  // Get tone settings
  if (server.hasArg("CourtesyTone1Freq")) {
    CourtesyTone1Freq = server.arg("CourtesyTone1Freq").toInt();
  }
  if (server.hasArg("CourtesyTone1Dur")) {
    CourtesyTone1Dur = server.arg("CourtesyTone1Dur").toInt();
  }
  if (server.hasArg("CourtesyTone2Freq")) {
    CourtesyTone2Freq = server.arg("CourtesyTone2Freq").toInt();
  }
  if (server.hasArg("CourtesyTone2Dur")) {
    CourtesyTone2Dur = server.arg("CourtesyTone2Dur").toInt();
  }
  if (server.hasArg("Callsign")) {
    Callsign = server.arg("Callsign");
  }
  if (server.hasArg("TailInfo")) {
    TailInfo = server.arg("TailInfo");
  }
  
  // Get debug settings
  debugMain = server.hasArg("debugMain");
  debugCarrDetect = server.hasArg("debugCarrDetect");
  debugRssiDetect = server.hasArg("debugRssiDetect");
  debugBeacon = server.hasArg("debugBeacon");
  
  // Save settings to preferences
  saveSettings();
  
  // Send confirmation page
  String html = "<!DOCTYPE html>\n"
              "<html>\n"
              "<head>\n"
              "  <title>Settings Saved</title>\n"
              "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
              "  <style>\n"
              "    body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }\n"
              "    h1 { color: #0066cc; }\n"
              "    .message { background-color: #e0f0ff; padding: 15px; border-radius: 4px; }\n"
              "    .back { display: inline-block; margin-top: 15px; color: #0066cc; }\n"
              "  </style>\n"
              "  <meta http-equiv=\"refresh\" content=\"3; URL=/\">\n"
              "</head>\n"
              "<body>\n"
              "  <h1>Settings Saved</h1>\n"
              "  <div class=\"message\">\n"
              "    <p>Your repeater settings have been saved.</p>\n"
              "    <p>You will be redirected to the home page in 3 seconds.</p>\n"
              "  </div>\n"
              "  <p><a href=\"/\" class=\"back\">Back to Home</a></p>\n"
              "</body>\n"
              "</html>\n";
  
  server.send(200, "text/html", html);
  
  if (debugMain) {
    Serial.println("Settings saved");
  }
}

// 404 Not Found handler
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  
  server.send(404, "text/plain", message);
}
