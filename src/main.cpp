


#include <Arduino.h>
#include "tasks/system_events.h"

void setup() {    
  Serial.begin(115200);
  delay(1000);
  // Serial.println("Hello Secure Boot V2");
  init_system_supervisor();
  
}

void loop() {  
  ElegantOTA.loop();
}
