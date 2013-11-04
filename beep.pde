



//    BEEP_SIMPLE: a short beep
//    BEEP_DOUBLE: double beep
//    BEEP_LONG: a longer beep 



#include <ArduinoRobot.h>

void setup(){
  Robot.begin();
  Robot.beginSpeaker();//Initialize the sound module
}

void loop(){
  Robot.beep(BEEP_SIMPLE);//Make a single beep sound
  delay(1000);
}
