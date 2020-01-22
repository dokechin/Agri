#include <SPI.h>
#include <EPD1in54.h>
#include <EPDPaint.h>

#include <stdint.h>

#define COLORED     0
#define UNCOLORED   1

const int PIN_NUM = 34;
const int ANALOG_MAX = 4096;
const int TRY_COUNT = 3;

RTC_DATA_ATTR uint8_t data[24*7];
RTC_DATA_ATTR int run = 0;
char humidityStr[24];

unsigned long prev = 0;

unsigned char image[40000];
EPDPaint paint(image, 0, 0);    // width should be the multiple of 8
EPD1in54 epd(13, 25, 26, 27); // reset, dc, cs, busy

void setup() {
  Serial.begin(9600);

  Serial.print("setup()");

  pinMode(12, OUTPUT);

  Serial.println("Ready to Connect");

  if (epd.init(lutFullUpdate) != 0) {
    Serial.print("e-Paper init failed");
    return;
  }

  epd.clearFrameMemory(0xFF);   // bit set = white, bit reset = black
  epd.displayFrame();
  epd.clearFrameMemory(0xFF);   // bit set = white, bit reset = black
  epd.displayFrame();

  // 1000000us * 3600 = 1hourのタイマー設定
  esp_sleep_enable_timer_wakeup(3600000000);
//  esp_sleep_enable_timer_wakeup(1000000);

}

void loop() {

  Serial.print("loop");

  memcpy(data, &data[1], sizeof(data) - sizeof(int8_t));

  digitalWrite(12, HIGH);

  int sum = 0;
  for (int i=0;i<TRY_COUNT;i++){
    delay(1000);
    int value = analogRead(PIN_NUM);
    Serial.println(value);
      
    sum = sum + value;
  }
    
  digitalWrite(12, LOW);

  uint8_t humidity = ((sum / TRY_COUNT) * 255)/ ANALOG_MAX;
  humidity = ~humidity;
  data[24*7 -1] = humidity;

  sprintf(humidityStr, "Hum:%03d Run:%06d", humidity, run++);
  Serial.println(humidityStr);

  paint.setRotate(ROTATE_0);
  paint.setWidth(200);
  paint.setHeight(200);
  
  paint.clear(UNCOLORED);
  paint.drawStringAt(0, 20, humidityStr, &Font24, COLORED);

  for(int i=0;i<168;i++){
    paint.drawLine(i + 15 , 199, i+ 16, int(199 - (data[i] /255.0) * 168), COLORED);    
  }
      
  epd.setFrameMemory(paint.getImage(), 0, 0, paint.getWidth(), paint.getHeight());
  epd.displayFrame();
  epd.sleep();
  esp_deep_sleep_start();
}
