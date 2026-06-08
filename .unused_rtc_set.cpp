/*
SET RTC DS3231 TIME
Upload sekali aja
*/

#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {

  Serial.begin(115200);

  Wire.begin(
    21,
    22
  );

  if (
    !rtc.begin()
  ) {

    Serial.println(
      "RTC NOT FOUND!"
    );

    while (1);
  }

  // =====================================
  // SET TIME
  // =====================================
  rtc.adjust(
    DateTime(
      F(__DATE__),
      F(__TIME__)
    )
  );

  Serial.println(
    "RTC TIME SET SUCCESS!"
  );

  DateTime now =
    rtc.now();

  Serial.print(
    "Current RTC Time: "
  );

  Serial.println(
    now.timestamp()
  );
}

void loop() {}