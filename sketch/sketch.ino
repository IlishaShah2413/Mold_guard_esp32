// ============================================================
//  Mold Detector – Context-Aware (4 Room Profiles)
//  Profiles: Hospital | Food Storage | Archive | Residential
//  Hardware: ESP32-DevKit + LCD1602 + 2x Potentiometer +
//            3x LED + Buzzer + 1x Button (GPIO 27)
// ============================================================

#include <LiquidCrystal.h>
#include <WiFi.h>
#include <HTTPClient.h>

// -------- THINGSPEAK --------
const char* ssid     = "Wokwi-GUEST";
const char* password = "";
const char* apiKey   = "YOUR_API_KEY";
const char* server   = "http://api.thingspeak.com/update";

// -------- PINS --------
#define TEMP_PIN    34
#define HUM_PIN     35
#define LED_G       19
#define LED_Y       21
#define LED_R       22
#define BUZZER      23
#define BTN_MODE    27

LiquidCrystal lcd(5, 17, 16, 4, 2, 15);

// ============================================================
//  ROOM PROFILE DEFINITIONS
// ============================================================

enum RoomMode { HOSPITAL = 0, FOOD_STORE, ARCHIVE, RESIDENTIAL, NUM_MODES };

const char* roomNames[NUM_MODES] = {
  "Hospital/Med   ",
  "Food Storage   ",
  "Archive/Library",
  "Residential    "
};

const char* roomShort[NUM_MODES] = {
  "HOSP",
  "FOOD",
  "ARCH",
  "HOME"
};

struct MoldRisk {
  int         level;
  const char* line2;
};

// ============================================================
//  getRiskCategory() helpers
// ============================================================

// ---- HOSPITAL profile ----
// Safe: H < 50% (ASHRAE 170 / NFPA-99)
MoldRisk riskHospital(float t, float h) {
  int cat = 0;  // FIX: initialize to 0 (SAFE) to avoid undefined behavior
  if      (h < 50.0)                                      cat = 0;
  else if (h >= 50.0 && h < 60.0 && t < 24.0)           cat = 1;
  else if (h >= 50.0 && h < 60.0 && t >= 24.0)          cat = 2;
  else if (h >= 60.0 && h < 70.0 && t < 28.0)           cat = 3;
  else if (h >= 60.0 && h < 70.0 && t >= 28.0)          cat = 4;
  else if (h >= 70.0 && t < 28.0)                       cat = 5;
  else if (h >= 70.0 && t >= 28.0)                      cat = 6;

  switch (cat) {
    case 0: return {0, "SAFE           "};
    case 1: return {1, "Risk ~7 days   "};
    case 2: return {2, "Risk ~3-5 days "};
    case 3: return {3, "Risk ~48hrs!   "};
    case 4: return {4, "Risk ~24hrs!   "};
    case 5: return {5, "Risk ~12hrs!!  "};
    case 6: return {6, "Risk ~6hrs!!   "};
    default: return {0, "SAFE           "};
  }
}

// ---- FOOD STORAGE profile ----
// Safe: H < 55%, T < 21°C (USDA dry storage)
MoldRisk riskFoodStore(float t, float h) {
  int cat = 0;  // FIX: initialize to 0 (SAFE)
  if      (h < 55.0)                                          cat = 0;
  else if (h >= 55.0 && h < 65.0 && t < 21.0)               cat = 1;
  else if (h >= 55.0 && h < 65.0 && t >= 21.0 && t < 28.0)  cat = 2;
  else if (h >= 55.0 && h < 65.0 && t >= 28.0)              cat = 3;  // FIX: was missing
  else if (h >= 65.0 && h < 75.0 && t < 28.0)               cat = 3;
  else if (h >= 65.0 && h < 75.0 && t >= 28.0)              cat = 4;
  else if (h >= 75.0 && t < 28.0)                           cat = 5;
  else if (h >= 75.0 && t >= 28.0)                          cat = 6;

  switch (cat) {
    case 0: return {0, "SAFE           "};
    case 1: return {1, "Risk ~7 days   "};
    case 2: return {2, "Risk ~3-5 days "};
    case 3: return {3, "Risk ~48hrs!   "};
    case 4: return {4, "Risk ~24hrs!   "};
    case 5: return {5, "Risk ~12hrs!!  "};
    case 6: return {6, "Risk ~6hrs!!   "};
    default: return {0, "SAFE           "};
  }
}

// ---- ARCHIVE / LIBRARY profile ----
// Safe: H < 50%, T < 19°C (NISO/European standards)
MoldRisk riskArchive(float t, float h) {
  int cat = 0;  // FIX: initialize to 0 (SAFE)
  if      (h < 50.0)                                              cat = 0;
  else if (h >= 50.0 && h < 60.0 && t < 19.0)                   cat = 1;
  else if (h >= 50.0 && h < 65.0 && t >= 19.0 && t < 28.0)      cat = 2;
  else if (h >= 50.0 && h < 65.0 && t >= 28.0)                  cat = 3;  // FIX: was missing
  else if (h >= 65.0 && h < 75.0 && t < 28.0)                   cat = 3;
  else if (h >= 65.0 && h < 75.0 && t >= 28.0)                  cat = 4;
  else if (h >= 75.0 && t < 28.0)                               cat = 5;
  else if (h >= 75.0 && t >= 28.0)                              cat = 6;

  switch (cat) {
    case 0: return {0, "SAFE           "};
    case 1: return {1, "Risk ~7 days   "};
    case 2: return {2, "Risk ~3-5 days "};
    case 3: return {3, "Risk ~48hrs!   "};
    case 4: return {4, "Risk ~24hrs!   "};
    case 5: return {5, "Risk ~12hrs!!  "};
    case 6: return {6, "Risk ~6hrs!!   "};
    default: return {0, "SAFE           "};
  }
}

// ---- RESIDENTIAL profile ----
// Safe: H < 60% (EPA/WHO)
MoldRisk riskResidential(float t, float h) {
  int cat = 0;  // FIX: initialize to 0 (SAFE)
  if      (h < 60.0)                                             cat = 0;
  else if (h >= 60.0 && h < 70.0 && t < 25.0)                  cat = 1;  // FIX: was missing (gap)
  else if (h >= 60.0 && h < 70.0 && t >= 25.0 && t < 28.0)    cat = 1;
  else if (h >= 70.0 && h < 80.0 && t < 25.0)                  cat = 2;  // FIX: was missing (gap)
  else if (h >= 70.0 && h < 80.0 && t >= 25.0 && t < 28.0)    cat = 2;
  else if (h >= 60.0 && h < 70.0 && t >= 28.0)                 cat = 3;
  else if (h >= 70.0 && h < 80.0 && t >= 28.0)                 cat = 4;
  else if (h >= 80.0 && t < 28.0)                              cat = 5;
  else if (h >= 80.0 && t >= 28.0)                             cat = 6;

  switch (cat) {
    case 0: return {0, "SAFE           "};
    case 1: return {1, "Risk ~48hrs    "};
    case 2: return {2, "Risk ~24hrs!   "};
    case 3: return {3, "Risk ~18hrs!   "};
    case 4: return {4, "Risk ~12hrs!   "};
    case 5: return {5, "Risk ~6hrs!!   "};
    case 6: return {6, "Risk ~2hrs!!   "};
    default: return {0, "SAFE           "};
  }
}

// ---- Dispatch to correct profile ----
MoldRisk getMoldRisk(float t, float h, RoomMode mode) {
  switch (mode) {
    case HOSPITAL:    return riskHospital(t, h);
    case FOOD_STORE:  return riskFoodStore(t, h);
    case ARCHIVE:     return riskArchive(t, h);
    case RESIDENTIAL: return riskResidential(t, h);
    default:          return riskResidential(t, h);
  }
}

// ============================================================
//  THINGSPEAK
// ============================================================
void uploadToThingSpeak(float t, float h, int s, int mode) {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  String url = String(server)
               + "?api_key=" + apiKey
               + "&field1=" + String(t, 1)
               + "&field2=" + String(h, 1)
               + "&field3=" + String(s)
               + "&field4=" + String(mode);
  http.begin(url);
  int code = http.GET();
  Serial.print("ThingSpeak HTTP: "); Serial.println(code);
  http.end();
}

// ============================================================
//  GLOBALS
// ============================================================
float          temp, hum;
int            moldStatus   = 0;
RoomMode       currentMode  = HOSPITAL;
unsigned long  lastUpload   = 0;
const unsigned long uploadInterval = 30000;

unsigned long  lastBtnTime  = 0;
const unsigned long debounce = 300;
bool           btnWasHigh   = true;
unsigned long  buzzerStart  = 0;
bool           buzzerActive = false;

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);

  pinMode(LED_G,    OUTPUT);
  pinMode(LED_Y,    OUTPUT);
  pinMode(LED_R,    OUTPUT);
  pinMode(BUZZER,   OUTPUT);
  pinMode(BTN_MODE, INPUT_PULLUP);

  // FIX: ensure all outputs start LOW
  digitalWrite(LED_G,  LOW);
  digitalWrite(LED_Y,  LOW);
  digitalWrite(LED_R,  LOW);
  digitalWrite(BUZZER, LOW);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("  Mold Detector ");
  lcd.setCursor(0, 1);
  lcd.print(" Context-Aware  ");
  delay(1500);

  lcd.clear();
  lcd.print("Mode:");
  lcd.print(roomShort[currentMode]);
  lcd.setCursor(0, 1);
  lcd.print(roomNames[currentMode]);
  delay(1200);

  lcd.clear();
  lcd.print("Connecting WiFi ");
  WiFi.begin(ssid, password);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500); Serial.print("."); tries++;
  }
  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    lcd.print("WiFi OK!        ");
  } else {
    Serial.println("\nWiFi FAILED");
    lcd.print("WiFi Failed     ");
  }
  delay(1000);
  lcd.clear();
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  unsigned long now = millis();

  // ---- Button: cycle room mode ----
  bool btnNow = digitalRead(BTN_MODE);
  if (!btnNow && btnWasHigh && (now - lastBtnTime > debounce)) {
    lastBtnTime = now;
    btnWasHigh = false;
    currentMode = (RoomMode)((currentMode + 1) % NUM_MODES);

    Serial.print("Room mode changed to: ");
    Serial.println(roomNames[currentMode]);

    // FIX: turn off all LEDs and buzzer immediately on mode change
    digitalWrite(LED_G,  LOW);
    digitalWrite(LED_Y,  LOW);
    digitalWrite(LED_R,  LOW);
    digitalWrite(BUZZER, LOW);
    buzzerActive = false;

    lcd.clear();
    lcd.print("Mode:");
    lcd.print(roomShort[currentMode]);
    lcd.setCursor(0, 1);
    lcd.print(roomNames[currentMode]);
    delay(300);  // FIX: reduced from 800ms so LEDs update faster after mode change
    lcd.clear();
  } else {
    btnWasHigh = btnNow;
  }

  // ---- Read sensors ----
  temp = (analogRead(TEMP_PIN) / 4095.0) * 50.0;
  hum  = (analogRead(HUM_PIN)  / 4095.0) * 100.0;

  // ---- Compute mold risk ----
  MoldRisk risk = getMoldRisk(temp, hum, currentMode);
  moldStatus    = risk.level;

  // ---- Reset outputs before setting new state ----
  digitalWrite(LED_G,  LOW);
  digitalWrite(LED_Y,  LOW);
  digitalWrite(LED_R,  LOW);

  // ---- LCD Row 0: mode + T/H ----
  lcd.setCursor(0, 0);
  lcd.print(roomShort[currentMode]);
  lcd.print(" T:");
  lcd.print(temp, 0);
  lcd.print("C H:");
  lcd.print(hum, 0);
  lcd.print("%   ");

  // ---- LCD Row 1: risk ----
  lcd.setCursor(0, 1);
  lcd.print(risk.line2);

  // ---- LEDs + Non-blocking Buzzer ----
  switch (moldStatus) {
    case 0:
      // SAFE: green only
      digitalWrite(LED_G, HIGH);
      digitalWrite(BUZZER, LOW);
      buzzerActive = false;
      break;

    case 1:
    case 2:
      // LOW RISK: green + yellow
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_Y, HIGH);
      digitalWrite(BUZZER, LOW);
      buzzerActive = false;
      break;

    case 3:
    case 4:
      // MEDIUM RISK: yellow only
      digitalWrite(LED_Y, HIGH);
      digitalWrite(BUZZER, LOW);
      buzzerActive = false;
      break;

    case 5:
    case 6:
      // HIGH RISK: red + buzzer
      digitalWrite(LED_R, HIGH);
      // Non-blocking buzzer: 200ms ON / 300ms OFF cycle
      if (!buzzerActive) {
        buzzerActive = true;
        buzzerStart = now;
      }
      if ((now - buzzerStart) < 200) {
        digitalWrite(BUZZER, HIGH);
      } else if ((now - buzzerStart) < 500) {
        digitalWrite(BUZZER, LOW);
      } else {
        buzzerActive = false;  // reset so next loop restarts the cycle
      }
      break;
  }

  // ---- ThingSpeak every 30s ----
  if (now - lastUpload >= uploadInterval) {
    lastUpload = now;
    uploadToThingSpeak(temp, hum, moldStatus, (int)currentMode);
  }

  // ---- Serial debug ----
  Serial.printf("[%s] T:%.1fC H:%.1f%% Lv:%d -> %s\n",
                roomShort[currentMode], temp, hum, moldStatus, risk.line2);

  delay(300);
}
