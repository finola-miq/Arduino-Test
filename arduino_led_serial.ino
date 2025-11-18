#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9
#define LED_PIN LED_BUILTIN
#define BAUD 115200

MFRC522 mfrc522(SS_PIN, RST_PIN);

// Your known tags
const String TAG1 = "F45B3200";  // turns LED ON
const String TAG2 = "4343A90D";  // turns LED OFF

// Function to read a full line from serial
String readLine() {
  static String buffer;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      String out = buffer;
      buffer = "";
      return out;
    }
    buffer += c;
    if (buffer.length() > 120) buffer = ""; // prevent overflow
  }
  return String();
}

// Helper to send LED state
void printState() {
  Serial.println(digitalRead(LED_PIN) ? "STATE:ON" : "STATE:OFF");
}

unsigned long lastTagMs = 0;
const unsigned long TAG_COOLDOWN_MS = 500;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(BAUD);
  delay(50);
  SPI.begin();
  mfrc522.PCD_Init();

  Serial.println("READY");
}

void loop() {
  // 1️⃣ Handle serial commands (from website)
  String line = readLine();
  if (line.length()) {
    line.trim();
    if (line.equalsIgnoreCase("key123")) {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("STATE:ON");
    } else if (line.equalsIgnoreCase("key321")) {
      digitalWrite(LED_PIN, LOW);
      Serial.println("STATE:OFF");
    } else if (line.equalsIgnoreCase("ping")) {
      Serial.println("pong");
    } else if (line.equalsIgnoreCase("status")) {
      printState();
    } else {
      Serial.print("ERR:UNKNOWN_CMD ");
      Serial.println(line);
    }
  }

  // 2️⃣ Handle RFID tag reads
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    unsigned long now = millis();
    if (now - lastTagMs >= TAG_COOLDOWN_MS) {
      lastTagMs = now;

      // Build UID
      String uid = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        byte b = mfrc522.uid.uidByte[i];
        if (b < 0x10) uid += "0";
        uid += String(b, HEX);
      }
      uid.toUpperCase();

      Serial.print("TAG:");
      Serial.println(uid);

      // Compare with your known tags
      if (uid == TAG1) {
        Serial.println("ACCESS:ALLOWED");
        digitalWrite(LED_PIN, HIGH);
        Serial.println("STATE:ON");
      } 
      else if (uid == TAG2) {
        Serial.println("ACCESS:SECOND_TAG");
        digitalWrite(LED_PIN, LOW);
        Serial.println("STATE:OFF");
      } 
      else {
        Serial.println("ACCESS:DENIED");
      }
    }

    // Stop communication
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}
