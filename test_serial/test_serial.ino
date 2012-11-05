const int speakerPin = 3;
const int ledPin = 2;

void beep(int freq, int duration=50)
{
  tone(speakerPin, freq);
  delay(duration);
  noTone(speakerPin);
  delay(50);
}

void setup()
{
  Serial.begin(9600);
  Serial.println("We are ready!");
  pinMode(speakerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  //beep(500);
  //beep(700);
  //beep(600);
  //beep(800);
  //beep(700);
  //beep(900);
  beep(1000, 100);
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
      beep(500);
    }
  }
}

