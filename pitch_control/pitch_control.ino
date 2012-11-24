void setup()
{
  Serial.begin(9600);
  pinMode(3, OUTPUT);
}

void loop()
{
  int a = analogRead(A0);
  int b = map(a, 0, 1023, 200, 1200);
  tone(3, b);
  Serial.print(a);
  Serial.print(" - ");
  Serial.println(b);
}

