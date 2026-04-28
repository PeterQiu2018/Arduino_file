#define LED 2
#define Touch 0
int buttonState = 255;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(Touch, INPUT);
  digitalWrite(LED, LOW);  // Turn off the LED initially
  delay(1000);
  digitalWrite(LED, HIGH);  // Turn off the LED initially
}

void loop() {
  // put your main code here, to run repeatedly:
  buttonState = digitalRead(Touch);
  Serial.println(buttonState);
  delay(50);
  // if (buttonState == 1){
  //   digitalWrite(LED, HIGH);
  // } else if (buttonState == 0){
  //   digitalWrite(LED, LOW);
  // }
}
