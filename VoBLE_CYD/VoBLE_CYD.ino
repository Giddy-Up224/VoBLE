#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <lvgl.h>
#include <TFT_eSPI.h>

// TFT and LVGL setup
TFT_eSPI tft = TFT_eSPI();
static const uint32_t screenWidth = 320;
static const uint32_t screenHeight = 240;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

// JK BMS Bluetooth configuration
static const char* BMS_MAC = "C8:47:80:23:4F:95"; // Replace with your BMS MAC address
static const char* SERVICE_UUID = "0000ffe0-0000-1000-8000-00805f9b34fb";
static const char* CHAR_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb";

// BMS data structure
struct BMSData {
  float voltage = 0.0;
  float current = 0.0;
  float soc = 0.0;
  float temperature = 0.0;
  float balance_current = 0.0;
  uint32_t uptime = 0;
  bool connected = false;
};

BMSData bmsData;

// BLE client objects
static BLEClient* pClient = nullptr;
static BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;
static bool doConnect = false;
static bool connected = false;

// LVGL objects
lv_obj_t * gauge_obj;
lv_meter_indicator_t * indic;
lv_obj_t * soc_label;
lv_obj_t * voltage_label;
lv_obj_t * current_label;
lv_obj_t * status_label;

// Buffer for BLE data
static std::vector<uint8_t> dataBuffer;

// JK BMS protocol functions
uint8_t calcCRC(const uint8_t* data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; i++) {
    crc += data[i];
  }
  return crc & 0xFF;
}

void createJKCommand(uint8_t address, uint8_t* command, size_t& len) {
  command[0] = 0xAA;
  command[1] = 0x55;
  command[2] = 0x90;
  command[3] = 0xEB;
  command[4] = address;
  command[5] = 0x00; // No additional data
  
  // Fill remaining bytes with zeros
  for (int i = 6; i < 19; i++) {
    command[i] = 0x00;
  }
  
  // Calculate and add CRC
  command[19] = calcCRC(command, 19);
  len = 20;
}

void parseJKResponse(const uint8_t* data, size_t len) {
  if (len < 300) return; // Minimum response size
  
  // Check header
  if (data[0] != 0x55 || data[1] != 0xAA || data[2] != 0xEB || data[3] != 0x90) {
    return;
  }
  
  uint8_t responseType = data[4];
  
  if (responseType == 0x02) { // Device state response
    // Parse voltage (32-bit little endian at offset 118+32)
    uint32_t voltageRaw = data[150] | (data[151] << 8) | (data[152] << 16) | (data[153] << 24);
    bmsData.voltage = voltageRaw * 0.001f;
    
    // Parse current (32-bit signed little endian at offset 126+32)
    int32_t currentRaw = data[158] | (data[159] << 8) | (data[160] << 16) | (data[161] << 24);
    bmsData.current = -currentRaw * 0.001f;
    
    // Parse SOC (8-bit at offset 141+32)
    bmsData.soc = data[173];
    
    // Parse temperature (16-bit signed little endian at offset 130+32)
    int16_t tempRaw = data[162] | (data[163] << 8);
    if (tempRaw != -2000) {
      bmsData.temperature = tempRaw / 10.0f;
    }
    
    // Parse balance current (16-bit signed little endian at offset 138+32)
    int16_t balanceRaw = data[170] | (data[171] << 8);
    bmsData.balance_current = balanceRaw / 1000.0f;
    
    // Parse uptime (32-bit little endian at offset 162+32)
    bmsData.uptime = data[194] | (data[195] << 8) | (data[196] << 16) | (data[197] << 24);
  }
}

// BLE callback functions
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                          uint8_t* pData, size_t length, bool isNotify) {
  
  // Check for header to clear buffer
  if (length >= 4 && pData[0] == 0x55 && pData[1] == 0xAA && pData[2] == 0xEB && pData[3] == 0x90) {
    dataBuffer.clear();
  }
  
  // Add data to buffer
  for (size_t i = 0; i < length; i++) {
    dataBuffer.push_back(pData[i]);
  }
  
  // Process complete messages
  if (dataBuffer.size() >= 300) {
    parseJKResponse(dataBuffer.data(), dataBuffer.size());
    dataBuffer.clear();
  }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("Connected to BMS");
    bmsData.connected = true;
    connected = true;
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("Disconnected from BMS");
    bmsData.connected = false;
    connected = false;
  }
};

bool connectToBMS() {
  if (pClient != nullptr && pClient->isConnected()) {
    return true;
  }
  
  BLEAddress serverAddress(BMS_MAC);
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  
  if (!pClient->connect(serverAddress)) {
    Serial.println("Failed to connect to BMS");
    return false;
  }
  
  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find service");
    pClient->disconnect();
    return false;
  }
  
  pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHAR_UUID));
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("Failed to find characteristic");
    pClient->disconnect();
    return false;
  }
  
  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }
  
  // Send initial commands
  uint8_t command[20];
  size_t cmdLen;
  
  // Device info command
  createJKCommand(0x97, command, cmdLen);
  pRemoteCharacteristic->writeValue(command, cmdLen);
  delay(100);
  
  // Device state command
  createJKCommand(0x96, command, cmdLen);
  pRemoteCharacteristic->writeValue(command, cmdLen);
  delay(100);
  
  return true;
}

// LVGL display flush callback
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t*)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

// LVGL input device callback (touchscreen)
void my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data) {
  uint16_t touchX, touchY;
  bool touched = tft.getTouch(&touchX, &touchY);
  
  if (touched) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = touchX;
    data->point.y = touchY;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

void setupLVGL() {
  lv_init();
  
  tft.begin();
  tft.setRotation(1);
  
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);
  
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
  
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);
}

void createUI() {
  // Create main container
  lv_obj_t * cont = lv_obj_create(lv_scr_act());
  lv_obj_set_size(cont, 320, 240);
  lv_obj_center(cont);
  
  // Create SOC gauge
  gauge_obj = lv_meter_create(cont);
  lv_obj_set_size(gauge_obj, 200, 200);
  lv_obj_set_pos(gauge_obj, 10, 10);
  
  // Add scale to meter
  lv_meter_scale_t * scale = lv_meter_add_scale(gauge_obj);
  lv_meter_set_scale_ticks(gauge_obj, scale, 51, 2, 10, lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_major_ticks(gauge_obj, scale, 10, 4, 15, lv_color_black(), 10);
  lv_meter_set_scale_range(gauge_obj, scale, 0, 100, 270, 135);
  
  // Add needle indicator
  indic = lv_meter_add_needle_line(gauge_obj, scale, 4, lv_palette_main(LV_PALETTE_RED), -10);
  
  // SOC label in center of gauge
  soc_label = lv_label_create(gauge_obj);
  lv_label_set_text(soc_label, "0%");
  lv_obj_set_style_text_font(soc_label, &lv_font_montserrat_24, 0);
  lv_obj_center(soc_label);
  
  // Status and data labels on the right
  status_label = lv_label_create(cont);
  lv_label_set_text(status_label, "Disconnected");
  lv_obj_set_pos(status_label, 220, 20);
  lv_obj_set_style_text_color(status_label, lv_palette_main(LV_PALETTE_RED), 0);
  
  voltage_label = lv_label_create(cont);
  lv_label_set_text(voltage_label, "0.000V");
  lv_obj_set_pos(voltage_label, 220, 50);
  
  current_label = lv_label_create(cont);
  lv_label_set_text(current_label, "0.000A");
  lv_obj_set_pos(current_label, 220, 80);
  
  // Title label
  lv_obj_t * title = lv_label_create(cont);
  lv_label_set_text(title, "JK BMS Monitor");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
  lv_obj_set_pos(title, 10, 220);
}

void updateUI() {
  static char textBuffer[32];
  
  // Update SOC gauge
  lv_meter_set_indicator_value(gauge_obj, indic, (int32_t)bmsData.soc);
  
  // Update SOC label
  snprintf(textBuffer, sizeof(textBuffer), "%.0f%%", bmsData.soc);
  lv_label_set_text(soc_label, textBuffer);
  
  // Update status
  if (bmsData.connected) {
    lv_label_set_text(status_label, "Connected");
    lv_obj_set_style_text_color(status_label, lv_palette_main(LV_PALETTE_GREEN), 0);
  } else {
    lv_label_set_text(status_label, "Disconnected");
    lv_obj_set_style_text_color(status_label, lv_palette_main(LV_PALETTE_RED), 0);
  }
  
  // Update voltage
  snprintf(textBuffer, sizeof(textBuffer), "%.3fV", bmsData.voltage);
  lv_label_set_text(voltage_label, textBuffer);
  
  // Update current
  snprintf(textBuffer, sizeof(textBuffer), "%.3fA", bmsData.current);
  lv_label_set_text(current_label, textBuffer);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting JK BMS Monitor for CYD");
  
  // Initialize Bluetooth
  BLEDevice::init("");
  
  // Initialize LVGL and display
  setupLVGL();
  createUI();
  
  // Attempt to connect to BMS
  doConnect = true;
}

void loop() {
  // Handle BLE connection
  if (doConnect == true) {
    if (connectToBMS()) {
      doConnect = false;
    } else {
      delay(5000); // Wait 5 seconds before retry
    }
  }
  
  // Request data periodically if connected
  static unsigned long lastDataRequest = 0;
  if (connected && millis() - lastDataRequest > 2000) { // Every 2 seconds
    uint8_t command[20];
    size_t cmdLen;
    createJKCommand(0x96, command, cmdLen);
    if (pRemoteCharacteristic != nullptr) {
      pRemoteCharacteristic->writeValue(command, cmdLen);
    }
    lastDataRequest = millis();
  }
  
  // Update UI
  updateUI();
  
  // Handle LVGL tasks
  lv_timer_handler();
  delay(5);
}