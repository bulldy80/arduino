const int speakerPin = 3;
const int ledPin = 2;

void setup()
{
  Serial.begin(9600);
  pinMode(speakerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  tone(speakerPin, 500);
  delay(500);
  noTone(speakerPin);
  delay(300);
  tone(speakerPin, 700);
  delay(200);
  noTone(speakerPin);
}

void loop()
{
  if (Serial.available()) {
    int numBlinks = Serial.parseInt();
    if (Serial.read() == '\n') {
      if (numBlinks > 10) {
        numBlinks = 10;
      }
      for (int i = 0; i < numBlinks; ++i) {
        digitalWrite(ledPin, HIGH);
        delay(200);
        digitalWrite(ledPin, LOW);
        delay(100);
      }
      Serial.print("Blinked ");
      Serial.print(numBlinks);
      Serial.println(" times!");
    }
  }
}

