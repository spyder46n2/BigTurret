const uint8_t relay_master[4] = {41,42,43,44};

void RelayBegin(){
  for (uint8_t r = 0; r < 4; r++){
    pinMode(relay_master[r], OUTPUT);
    digitalWrite(relay_master[r], 1);
  }
}

void RelaySet(uint8_t temp_relay, uint8_t temp_state){
  temp_relay = constrain(temp_relay,0,3);
  if (temp_state == 1){
    digitalWrite(relay_master[temp_relay], 0);
    //pinMode(relay_master[temp_relay], OUTPUT);
  }
  else {
    digitalWrite(relay_master[temp_relay], 1);
    //pinMode(relay_master[temp_relay], INPUT);
  }
}
