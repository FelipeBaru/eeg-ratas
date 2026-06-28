#include <Arduino.h>
// Test de multiplexor CD4051B — selección manual de canal (Arduino Uno, 5V)
// Elegís 1 canal (0 a 7) desde el Monitor Serie y leés COM en A0.

const int PIN_A   = 2;  // -> CD4051B pin 11 (A, bit0/LSB)
const int PIN_B   = 3;  // -> CD4051B pin 10 (B, bit1)
const int PIN_C   = 4;  // -> CD4051B pin 9  (C, bit2/MSB)
const int PIN_INH = 5;  // -> CD4051B pin 6  (INH)
const int PIN_COM = A0; // -> CD4051B pin 3  (COM, salida del MUX)

int canal = 0;

void seleccionarCanal(int ch) {
  digitalWrite(PIN_A, bitRead(ch, 0));
  digitalWrite(PIN_B, bitRead(ch, 1));
  digitalWrite(PIN_C, bitRead(ch, 2));
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(PIN_C, OUTPUT);
  pinMode(PIN_INH, OUTPUT);
  digitalWrite(PIN_INH, LOW);          // INH en LOW = MUX habilitado
  seleccionarCanal(canal);
  Serial.println(F("MUX CD4051B listo. Escribi un canal (0 a 7) y Enter."));
}

void loop() {
  if (Serial.available() > 0) {
    int n = Serial.parseInt();
    if (n >= 0 && n <= 7) {
      canal = n;
      seleccionarCanal(canal);
      Serial.print(F("Canal "));
      Serial.print(canal);
      Serial.print(F("  (CBA="));
      Serial.print(bitRead(canal,2));
      Serial.print(bitRead(canal,1));
      Serial.print(bitRead(canal,0));
      Serial.println(F(")"));
    }
  }
  Serial.print(F("Canal "));
  Serial.print(canal);
  Serial.print(F(" -> ADC = "));
  Serial.println(analogRead(PIN_COM));
  delay(250);
}
