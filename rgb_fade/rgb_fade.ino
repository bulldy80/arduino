const int NUM_CHANNELS = 3;
const int channelPin[NUM_CHANNELS] = {3,5,6};
const int MAX_LEVEL = 1000;
const int MIN_LEVEL = 0;
int channelLevel[NUM_CHANNELS];
int channelDir[NUM_CHANNELS];
long lastTime;
int curChannel;
int phase;

void setup()
{
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  
  lastTime = millis();
  for (int i = 0; i < NUM_CHANNELS; ++i) {
    pinMode(channelPin[i], OUTPUT);
    channelLevel[i] = 0;
    channelDir[i] = 1;
  }
  curChannel = 0;
  phase = 0;
}

void loop()
{
  long t = millis();
  int timePassed = t - lastTime;
  lastTime = t;
  int inc = timePassed;
  channelLevel[curChannel] += inc * channelDir[curChannel];
  
  if (channelLevel[curChannel] > MAX_LEVEL) {
    channelLevel[curChannel] = MAX_LEVEL;
    channelDir[curChannel] = -1;
    curChannel = (curChannel + 1) % NUM_CHANNELS;
  }
  else if (channelLevel[curChannel] < MIN_LEVEL) {
    channelLevel[curChannel] = MIN_LEVEL;
    channelDir[curChannel] = 1;
    curChannel = (curChannel + 1) % NUM_CHANNELS;
    if (curChannel == phase) {
      curChannel = (curChannel + 1) % NUM_CHANNELS;
      phase = (phase + 1) % NUM_CHANNELS;
    }
  }
  
  for (int i = 0; i < NUM_CHANNELS; ++i) {
    int a = map(channelLevel[i], MIN_LEVEL, MAX_LEVEL, 4, 255);
    analogWrite(channelPin[i], 255-a);
  }
}

