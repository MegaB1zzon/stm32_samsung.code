#include <Arduino.h>

String inputBuffer;
int temperatureC = 0;
bool temperatureReceived = false;

// Настройки пинов — измените при необходимости
const uint8_t MOSFET_PIN = PA0; // пин управления MOSFET
const uint8_t FAN_PWM_PIN = PA1; // ШИМ-пин для вентилятора

bool mosfetOn = false;

// Параметры для ШИМ-контроля вентилятора
const float FAN_TEMP_MIN = 28.0; // 0% при 28°C
const float FAN_TEMP_MAX = 35.0; // 100% при 35°C

void processCommand(const String &cmd) {
  String s = cmd;
  s.trim();
  if (s.length() == 0) {
    return;
  }

  int len = s.length();
  int suffixLen = 0; // number of bytes used by suffix

  // Check ASCII 'C' or 'c'
  if (len >= 1) {
    char last = s.charAt(len - 1);
    if (last == 'C' || last == 'c') {
      suffixLen = 1;
    }
  }

  // Check Cyrillic 'С' (U+0421 -> 0xD0 0xA1) or 'с' (U+0441 -> 0xD1 0x81)
  if (suffixLen == 0 && len >= 2) {
    uint8_t b1 = (uint8_t)s.charAt(len - 2);
    uint8_t b2 = (uint8_t)s.charAt(len - 1);
    if ((b1 == 0xD0 && b2 == 0xA1) || (b1 == 0xD1 && b2 == 0x81)) {
      suffixLen = 2;
    }
  }

  if (suffixLen == 0) {
    Serial.println("Error: send value ending with C, e.g. 16C");
    return;
  }

  String numberPart = s.substring(0, len - suffixLen);
  numberPart.trim();
  int value = numberPart.toInt();

  if (numberPart.length() == 0 || (value == 0 && numberPart != "0")) {
    Serial.println("Error: invalid temperature number");
    return;
  }

  temperatureC = value;
  temperatureReceived = true;

  Serial.print("Temperature received: ");
  Serial.print(temperatureC);
  Serial.println(" °C");
}

// Обновляет состояние MOSFET и ШИМ вентилятора на основе temperatureC
void updateOutputs() {
  // Hysteresis для MOSFET: включаем при >33, выключаем при <30
  if (temperatureC > 33 && !mosfetOn) {
    digitalWrite(MOSFET_PIN, HIGH);
    mosfetOn = true;
    Serial.println("MOSFET: ON");
  } else if (temperatureC < 30 && mosfetOn) {
    digitalWrite(MOSFET_PIN, LOW);
    mosfetOn = false;
    Serial.println("MOSFET: OFF");
  }

  // ШИМ-вентилятор: линейно от 0% (FAN_TEMP_MIN) до 100% (FAN_TEMP_MAX)
  float t = (float)temperatureC;
  int pwm = 0;
  if (t <= FAN_TEMP_MIN) {
    pwm = 0;
  } else if (t >= FAN_TEMP_MAX) {
    pwm = 255;
  } else {
    float ratio = (t - FAN_TEMP_MIN) / (FAN_TEMP_MAX - FAN_TEMP_MIN);
    if (ratio < 0) ratio = 0;
    if (ratio > 1) ratio = 1;
    pwm = (int)round(ratio * 255.0);
  }
  analogWrite(FAN_PWM_PIN, pwm);
  int percent = (int)round((pwm / 255.0) * 100.0);
  Serial.print("Fan PWM: ");
  Serial.print(percent);
  Serial.println(" %");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW);
  pinMode(FAN_PWM_PIN, OUTPUT);
  analogWrite(FAN_PWM_PIN, 0);

  Serial.println("STM32 ready. Send temperature like 16C or 16С");
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      processCommand(inputBuffer);
      inputBuffer = "";
    } else {
      inputBuffer += c;
      if (inputBuffer.length() > 32) {
        inputBuffer = inputBuffer.substring(inputBuffer.length() - 32);
      }
    }
  }

  if (temperatureReceived) {
    temperatureReceived = false;
    updateOutputs();
  }
}
