


#include <Arduino.h>
#include "tasks/system_events.h"

void setup() {    
  Serial.begin(115200);
  delay(1000);
  init_system_supervisor();
  
}

void loop() {  
  ElegantOTA.loop();
}
