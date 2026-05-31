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

  Serial.println(
    "RTC Running..."
  );
}

void loop() {

  DateTime now =
    rtc.now();

  Serial.print("\r"); // balik ke awal baris

  Serial.print(
    now.timestamp()
  );

  Serial.print("   "); 
  // padding biar karakter lama ketimpa

  delay(1000);
}