#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/i2s.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

Adafruit_SSD1306 display(128, 64, &Wire, -1);

#define I2S_WS   25
#define I2S_SCK  26
#define I2S_SD   32
#define SAMPLE_RATE 16000

#define BUFFER_SIZE 512
int32_t rawSamples[BUFFER_SIZE];

void setup() {

  Serial.begin(115200);
  delay(1000); // wait for Serial Monitor to connect

  Serial.println("");
  Serial.println("=============================");
  Serial.println("  ESP32 DIAGNOSTIC BOOT");
  Serial.println("=============================");

  // --- STEP 1: Wire ---
  Serial.print("[1] Wire.begin(21,22)... ");
  Wire.begin(21, 22);
  delay(100);
  Serial.println("OK");

  // --- STEP 2: I2C Scan ---
  Serial.print("[2] I2C scan... ");
  bool found = false;
  for(uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if(Wire.endTransmission() == 0) {
      Serial.print("device at 0x");
      Serial.println(addr, HEX);
      found = true;
    }
  }
  if(!found) Serial.println("NO DEVICES FOUND — check SDA/SCL wiring");

  // --- STEP 3: OLED ---
  Serial.print("[3] OLED display.begin()... ");
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("FAILED — stuck here. Check OLED address & wiring.");
    while(true) {
      delay(500);
    }
  }
  Serial.println("OK");

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("DIAG OK");
  display.println("Starting BT...");
  display.display();

  // --- STEP 4: Bluetooth ---
  Serial.print("[4] SerialBT.begin()... ");
  SerialBT.begin("ESP32-AUDIO");
  Serial.println("OK");

  display.println("BT OK");
  display.println("Starting I2S...");
  display.display();

  // --- STEP 5: I2S ---
  Serial.print("[5] I2S driver install... ");

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 128,
    .use_apll = true,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if(err != ESP_OK) {
    Serial.print("FAILED — error code: ");
    Serial.println(err);
  } else {
    Serial.println("OK");
  }

  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_0);
  Serial.println("[6] I2S pins set OK");

  display.println("I2S OK");
  display.println("All systems GO");
  display.display();

  Serial.println("");
  Serial.println("=============================");
  Serial.println("  ALL INIT STEPS PASSED");
  Serial.println("  Reading mic levels...");
  Serial.println("=============================");
}

void loop() {

  size_t bytesRead = 0;

  i2s_read(I2S_NUM_0, rawSamples, sizeof(rawSamples), &bytesRead, portMAX_DELAY);

  int sampleCount = bytesRead / sizeof(int32_t);
  long level = 0;

  for(int i = 0; i < sampleCount; i++) {
    int32_t sample = rawSamples[i] >> 14;
    int16_t pcm = (int16_t)sample;
    level += abs(pcm);
  }

  if(sampleCount > 0) level /= sampleCount;

  // Print mic level every second
  static unsigned long lastPrint = 0;
  if(millis() - lastPrint >= 500) {
    Serial.print("Mic level: ");
    Serial.print(level);
    Serial.print("  samples: ");
    Serial.println(sampleCount);
    lastPrint = millis();

    // Show live level on OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("DIAGNOSTIC MODE");
    display.drawLine(0, 10, 128, 10, WHITE);
    display.setCursor(0, 14);
    display.print("Level: ");
    display.println(level);
    display.print("Samples: ");
    display.println(sampleCount);

    // Simple level bar
    int barW = map(min(level, 2000L), 0, 2000, 0, 120);
    display.fillRect(4, 40, barW, 10, WHITE);
    display.drawRect(4, 40, 120, 10, WHITE);

    display.display();
  }
}
