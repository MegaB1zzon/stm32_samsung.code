#include <Arduino.h>

String inputBuffer;
int temperatureC = 0;
bool temperatureReceived = false;

void processCommand(const String &cmd) {
  String s = cmd;
  s.trim();
  if (s.length() == 0) {
    return;
  }

  char last = s.charAt(s.length() - 1);
  bool isCelsius = false;

  if (last == 'C' || last == 'c' || last == 'С' || last == 'с') {
    isCelsius = true;
  }

  if (!isCelsius) {
    Serial.println("Error: send value ending with C, e.g. 16C");
    return;
  }

  String numberPart = s.substring(0, s.length() - 1);
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

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
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
    // Тут можна використовувати temperatureC для керування обладнанням.
  }
}
