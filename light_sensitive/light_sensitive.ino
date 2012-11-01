const int NUM_SMOOTH = 25;
int smooth[NUM_SMOOTH];
int total;
int currentIndex;
int lastOff = 0;

const int redPin = 3;

void setup()
{
  total = 0;
  currentIndex = 0;
  for (int i = 0; i < NUM_SMOOTH; ++i) {
    smooth[i] = 0;
  }
  Serial.begin(9600);
  pinMode(3, OUTPUT);
}

void loop()
{
  int a = analogRead(A0);
  total -= smooth[currentIndex];
  smooth[currentIndex] = a;
  total += smooth[currentIndex];
  currentIndex = (currentIndex+1) % NUM_SMOOTH;
  int avg = total / NUM_SMOOTH;
  int m = map(avg, 330, 670, 0, 255);
  m = constrain(m, 0, 255);
  Serial.print(a);
  Serial.print(" - ");
  Serial.print(avg);
  Serial.print(" - ");
  Serial.println(m);
  if (lastOff && m != 255) {
    for (int i = 0; i < 10; ++i) {
        digitalWrite(3, HIGH);
        delay(100);
        digitalWrite(3, LOW);
        delay(100);
    }
  }
  lastOff = (255 == m) ? 1 : 0;
  analogWrite(3, 255-m);
  delay(1000/NUM_SMOOTH);
}

