
// Reference:
// http://www.sitepoint.com/arduino-and-ruby-create-a-gmail-notifier/

void setup() {
  Serial.begin(1200);
 
  //Set pin 11 to be output pin (connect LED here)
  pinMode(11, OUTPUT);
 
  //set pin 11 to be high initially; LED is usually on, but, not blinking
  digitalWrite(11, HIGH);
}
 
//this procedure is called if we need to blink the LED
void blink_led() {
 
  //we can't just blink it once, because that wouldn't be noticeable
  //so, we blink it three times
  for(int i = 0; i<10; i++) {
    digitalWrite(11, HIGH);
    delay(100);
    digitalWrite(11, LOW);
    delay(100);
  }
 
  //reset the LED back to just on, not blinking
  digitalWrite(11, HIGH);
}
 
//this function is called when we get a character
//over USB/Serial
void got_char(char x) {
 
  //if we get b over Serial
  if(x == 'b') {
    //... blink the LED
    blink_led();
  }
}
 
void loop() {
  //check if there's any data available on serial
  if(Serial.available() > 0) {
    //if there is, we read it
    byte byte_read = Serial.read();
 
    //and call "got_char"
    got_char((char)byte_read);
  }
}
