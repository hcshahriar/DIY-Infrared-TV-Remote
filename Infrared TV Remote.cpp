 /*
 * DIY IR TV Remote Control
 * Supports Arduino/ESP32 with IR LED output
 * Uses NEC protocol by default (can be customized)
 * Includes button matrix and optional IR receiver for learning
 * Optional OLED display support
 */

// Configuration section - customize for your setup
#define USE_ESP32 false  // Set to true if using ESP32
#define USE_OLED false   // Set to true if using OLED display
#define DEBUG_SERIAL true // Enable serial debug output
#define LEARN_MODE false // Enable IR code learning mode

#include <IRremote.h> // For Arduino
#if USE_ESP32
  #include <IRremoteESP8266.h>
  #include <IRsend.h>
#endif

#if USE_OLED
  #include <Adafruit_SSD1306.h>
  #include <Adafruit_GFX.h>
  #define SCREEN_WIDTH 128
  #define SCREEN_HEIGHT 64
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#endif

// Pin configuration
const int IR_LED_PIN = 3;          // IR LED connected to this pin
const int IR_RECEIVER_PIN = 2;     // Optional IR receiver
const int NUM_ROWS = 4;            // Keypad rows
const int NUM_COLS = 4;            // Keypad columns

// Keypad row/column pins
byte rowPins[NUM_ROWS] = {8, 9, 10, 11};    // R1, R2, R3, R4
byte colPins[NUM_COLS] = {12, A0, A1, A2};  // C1, C2, C3, C4

// Button matrix mapping
char keys[NUM_ROWS][NUM_COLS] = {
  {'1','2','3','U'},  // 1, 2, 3, Channel Up
  {'4','5','6','D'},  // 4, 5, 6, Channel Down
  {'7','8','9','+'},  // 7, 8, 9, Volume Up
  {'*','0','#','-'}   // *, 0, #, Volume Down
};

// Customize these codes for your TV (NEC format)
unsigned long IR_CODES[16] = {
  0xFFA25D, // 0 - Power
  0xFF6897, // 1 
  0xFF9867, // 2
  0xFFB04F, // 3
  0xFF30CF, // 4
  0xFF18E7, // 5
  0xFF7A85, // 6
  0xFF10EF, // 7
  0xFF38C7, // 8
  0xFF5AA5, // 9
  0xFF42BD, // * - Mute
  0xFF4AB5, // 0
  0xFF52AD, // # - Input
  0xFF629D, // U - Channel Up
  0xFFA857, // D - Channel Down
  0xFF906F  // + - Volume Up
  0xFFE01F  // - - Volume Down
};

#if USE_ESP32
  IRsend irsend(IR_LED_PIN);
#else
  IRsend irsend;
#endif

IRrecv irrecv(IR_RECEIVER_PIN);
decode_results results;

void setup() {
  #if DEBUG_SERIAL
    Serial.begin(115200);
    Serial.println("DIY IR Remote Initializing...");
  #endif

  // Initialize keypad pins
  for (byte r = 0; r < NUM_ROWS; r++) {
    pinMode(rowPins[r], INPUT_PULLUP);
  }
  for (byte c = 0; c < NUM_COLS; c++) {
    pinMode(colPins[c], OUTPUT);
    digitalWrite(colPins[c], HIGH);
  }

  // Initialize IR components
  irsend.begin();
  if (LEARN_MODE) {
    irrecv.enableIRIn();
    #if DEBUG_SERIAL
      Serial.println("IR Learning Mode Enabled");
    #endif
  }

  #if USE_OLED
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      #if DEBUG_SERIAL
        Serial.println(F("OLED allocation failed"));
      #endif
    } else {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.println("DIY IR Remote");
      display.display();
      delay(2000);
    }
  #endif
}

void loop() {
  static unsigned long lastPress = 0;
  const unsigned long debounceTime = 200; // ms
  
  char key = getKeyPressed();
  
  if (key != 0 && (millis() - lastPress) > debounceTime) {
    lastPress = millis();
    handleButtonPress(key);
  }

  if (LEARN_MODE) {
    checkIRReceiver();
  }
}

char getKeyPressed() {
  // Scan keypad matrix for pressed keys
  for (byte c = 0; c < NUM_COLS; c++) {
    digitalWrite(colPins[c], LOW);
    
    for (byte r = 0; r < NUM_ROWS; r++) {
      if (digitalRead(rowPins[r]) == LOW) {
        digitalWrite(colPins[c], HIGH);
        return keys[r][c];
      }
    }
    
    digitalWrite(colPins[c], HIGH);
  }
  return 0; // No key pressed
}

void handleButtonPress(char key) {
  int codeIndex = -1;
  String keyName = "";
  
  // Map button to function
  switch(key) {
    case '1': codeIndex = 1; keyName = "1"; break;
    case '2': codeIndex = 2; keyName = "2"; break;
    case '3': codeIndex = 3; keyName = "3"; break;
    case '4': codeIndex = 4; keyName = "4"; break;
    case '5': codeIndex = 5; keyName = "5"; break;
    case '6': codeIndex = 6; keyName = "6"; break;
    case '7': codeIndex = 7; keyName = "7"; break;
    case '8': codeIndex = 8; keyName = "8"; break;
    case '9': codeIndex = 9; keyName = "9"; break;
    case '0': codeIndex = 11; keyName = "0"; break;
    case '*': codeIndex = 10; keyName = "Mute"; break;
    case '#': codeIndex = 12; keyName = "Input"; break;
    case 'U': codeIndex = 13; keyName = "Ch+"; break;
    case 'D': codeIndex = 14; keyName = "Ch-"; break;
    case '+': codeIndex = 15; keyName = "Vol+"; break;
    case '-': codeIndex = 16; keyName = "Vol-"; break;
    default: return;
  }

  if (codeIndex >= 0) {
    sendIRCommand(codeIndex);
    
    #if DEBUG_SERIAL
      Serial.print("Button pressed: ");
      Serial.print(keyName);
      Serial.print(" - Sending code: 0x");
      Serial.println(IR_CODES[codeIndex], HEX);
    #endif
    
    #if USE_OLED
      display.clearDisplay();
      display.setCursor(0,0);
      display.print("Button: ");
      display.println(keyName);
      display.print("Code: 0x");
      display.println(IR_CODES[codeIndex], HEX);
      display.display();
    #endif
  }
}

void sendIRCommand(int codeIndex) {
  #if USE_ESP32
    irsend.sendNEC(IR_CODES[codeIndex]);
  #else
    irsend.sendNEC(IR_CODES[codeIndex], 32);
  #endif
}

void checkIRReceiver() {
  // Check for incoming IR signals in learn mode
  if (irrecv.decode(&results)) {
    #if DEBUG_SERIAL
      Serial.print("Received IR code: 0x");
      Serial.println(results.value, HEX);
      Serial.println("Press button to assign this code");
    #endif
    
    #if USE_OLED
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("IR Code Received:");
      display.print("0x");
      display.println(results.value, HEX);
      display.println("Press button to assign");
      display.display();
    #endif
    
    // Wait for button press to assign this code
    unsigned long startTime = millis();
    while (millis() - startTime < 5000) { // 5 second timeout
      char key = getKeyPressed();
      if (key != 0) {
        int codeIndex = -1;
        switch(key) {
          case '1': codeIndex = 1; break;
          case '2': codeIndex = 2; break;
          case '3': codeIndex = 3; break;
          case '4': codeIndex = 4; break;
          case '5': codeIndex = 5; break;
          case '6': codeIndex = 6; break;
          case '7': codeIndex = 7; break;
          case '8': codeIndex = 8; break;
          case '9': codeIndex = 9; break;
          case '0': codeIndex = 11; break;
          case '*': codeIndex = 10; break;
          case '#': codeIndex = 12; break;
          case 'U': codeIndex = 13; break;
          case 'D': codeIndex = 14; break;
          case '+': codeIndex = 15; break;
          case '-': codeIndex = 16; break;
          default: break;
        }
        
        if (codeIndex >= 0) {
          IR_CODES[codeIndex] = results.value;
          #if DEBUG_SERIAL
            Serial.print("Assigned code 0x");
            Serial.print(results.value, HEX);
            Serial.print(" to button ");
            Serial.println(key);
          #endif
          
          #if USE_OLED
            display.clearDisplay();
            display.setCursor(0,0);
            display.print("Assigned to ");
            display.println(key);
            display.display();
            delay(2000);
          #endif
          break;
        }
      }
    }
    
    irrecv.resume ( ); 
  }
}
