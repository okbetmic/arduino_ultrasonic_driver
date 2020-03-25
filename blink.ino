void setup() {
  pinMode(13, OUTPUT); 
}

void loop() {
  int i = 0; 
  long int t = 1000000/30000;

  for(i = 0; i < t; i++){
    digitalWrite(13, 1);
    delayMicroseconds(i);
    digitalWrite(13, 1);
    delayMicroseconds(t - i);
  }
}
