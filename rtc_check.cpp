#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {

  Serial.begin(115200);

  Wire.begin(
    21,
    22
  );

  rtc.begin();
}

void loop() {

  DateTime now =
    rtc.now();

  Serial.print(
    now.timestamp()
  );

  Serial.println();

  delay(1000);
}