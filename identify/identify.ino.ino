// Define o pino do LED interno do ESP32-C3
const int ledPin = 8;

void setup() {
  pinMode(ledPin, OUTPUT);  // Configura o pino como saída
}

void loop() {
  digitalWrite(ledPin, HIGH);  // Acende o LED
  delay(500);                  // Espera 500 ms
  digitalWrite(ledPin, LOW);   // Apaga o LED
  delay(500);                  // Espera 500 ms
}
