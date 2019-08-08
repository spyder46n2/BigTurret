#include "Serialcom.cpp"

SerialCommand Ser;

int16_t serial_response;
uint8_t serial_echo_command = 0; //whether or not to echo commands
uint8_t serial_echo_numbers = 0; //whether or not to echo numbers
uint32_t serial_command;
int32_t serial_number[8];

void setup() {
  delay(500);
  Ser.Begin(); //start serial connection
  RelayBegin();
  MotionBegin(); //start motion
  //EyeMotorPower(1); //attach eye servo's

  //RelaySet(3, 1); //set eye relay on
  //RelaySet(2, 1); //set open relay on
}

void loop() {
  //ManualTest()
  serial_response = Ser.Update();
  if (serial_response == 1) { //if there was a line decoded
    serial_command = Ser.GetCommand(); //get command value
    for (uint8_t n = 0; n < 8; n++) {
      serial_number[n] = Ser.GetNumber(n); //get numbers
    }
    if (serial_echo_command == 1) {
      Serial.print("Command: "); Serial.println(serial_command);
    }
    if (serial_echo_numbers == 1) { //echo the number
      Serial.print("Number: "); Serial.print(serial_number[0]);
      for (uint8_t n = 1; n < 8; n++) {
        Serial.print(", "); Serial.print(serial_number[n]);
      }
      Serial.println("");
    }

    switch (serial_command) {
      case 1937011316: //strt
        TurretStart();
        break;

      case 1937010544: //stop
        TurretStop();
        break;

      case 593719149:  //#com, set command echo
        Serial.println("Set command echo");
        if (serial_number[0] == 1) serial_echo_command = 1;
        else serial_echo_command = 0;
        break;

      case 594441581:  //#num, set number echo
        Serial.println("Set number echo");
        if (serial_number[0] == 1) serial_echo_numbers = 1;
        else serial_echo_numbers = 0;
        break;

      case 1634496631:  //alpw, all power
        serial_number[0] = constrain(serial_number[0], 0 , 1);
        RelaySet(0, serial_number[0]); //set left relay
        RelaySet(1, serial_number[0]); //set right relay
        RelaySet(2, serial_number[0]); //set open relay
        RelaySet(3, serial_number[0]); //set eye relay
        break;

      case 1702457463:  //eypw, eye power
        serial_number[0] = constrain(serial_number[0], 0 , 1);
        RelaySet(3, serial_number[0]); //set eye relay
        break;

      case 1869639799:  //oppw, open power
        serial_number[0] = constrain(serial_number[0], 0 , 1);
        RelaySet(2, serial_number[0]); //set open relay
        break;

      case 1818718327:  //lgpw, left gun power
        serial_number[0] = constrain(serial_number[0], 0 , 1);
        RelaySet(0, serial_number[0]); //set left relay
        break;

      case 1919381623:  //rgpw, right gun power
        serial_number[0] = constrain(serial_number[0], 0 , 1);
        RelaySet(1, serial_number[0]); //set right relay
        break;

      case 1869636974:  //open, open command
        serial_number[0] = constrain(serial_number[0], 0 , 1);
        MotionOpen(serial_number[0]);
        break;

      case 1735421811:  //gpos, gun position
        serial_number[0] = constrain(serial_number[0], 0 , 255);
        serial_number[1] = constrain(serial_number[1], 0 , 255);
        MotionSetGunPosition(serial_number[0], serial_number[1]);
        break;

      case 1734700641:  //gena, gun enable
        break;

      case 1701867379:  //epos, eye position (unlinks elink)
        serial_number[0] = constrain(serial_number[0], 0 , 255);
        serial_number[1] = constrain(serial_number[1], 0 , 255);
        EyeSetPosition(serial_number[0], serial_number[1]);
        EyeLink(0);
        break;

      case 1701146209:  //eena, eye enable
        break;

      case 1701604971:  //elnk, eye position link to gun position
        serial_number[0] = constrain(serial_number[0], 0 , 1);
        EyeLink(serial_number[0]);
        break;

      case 1701603687:  //elig, eye light
        serial_number[0] = constrain(serial_number[0], 0 , 255);
        //Serial.print("Setting led to: "); Serial.println(serial_number[0]);
        EyeSetLed(serial_number[0]);
        break;

      case 1701602405: //elde, eye light delay
        serial_number[0] = constrain(serial_number[0], 0 , 32000);
        EyeSetLedDelay(serial_number[0]);
        break;

      case 1701601651:  //elas, eye laser
        serial_number[0] = constrain(serial_number[0], 0 , 1);
        EyeSetLaser(serial_number[0]);
        break;

      case 1701603693: //elim
        serial_number[0] = constrain(serial_number[0], 0 , 1);
        EyeLimit(serial_number[0]);
        break;

      case 1718186597:  //fire, fire guns
        serial_number[0] = constrain(serial_number[0], 0 , 1);
        MotionFireAll(serial_number[0]);
        break;

      case 1718646642: //fpwr: set firepower
        serial_number[0] = constrain(serial_number[0], 0 , 255);
        MotionSetFirePower(serial_number[0]);
        break;

      case 1718776692: //frst, feeder warning reset
        MotionFeederReset();
        break;

      case 1952805748: //test
        Serial.println("test");
        Serial.print(MotionGetPanCloseAlign(1)); Serial.print(", "); Serial.print(MotionGetLeftTiltCloseAlign(1)); Serial.print(", "); Serial.println(MotionGetRightTiltCloseAlign(1));
        Serial.print("Armed state: "); Serial.println(MotionGetArmed());
        break;

      case 561013102: //!pan
        RelaySet(3, 1); //set eye relay
        if (serial_number[0] == 1) {
          MotionSetPanRaw(1, 255);
        }
        else if (serial_number[0] == -1) {
          MotionSetPanRaw(-1, 255);
        }
        delay(1000);
        MotionSetPanRaw(1, 0);
        RelaySet(3, 0); //set eye relay
        break;

      case 560951397: //!ope
        RelaySet(2, 1); //set open relay
        if (serial_number[0] == 1) {
          MotionSetOpenRaw(1, 255);
        }
        else if (serial_number[0] == -1) {
          MotionSetOpenRaw(-1, 255);
        }
        delay(1000);
        MotionSetOpenRaw(1, 0);
        RelaySet(2, 0); //set open relay
        break;

      case 561210991: //!sfo 
        serial_number[0] = constrain(serial_number[0], 0 , 1);
        motionOverrideSafety(serial_number[0]);
        break;

      case 1063349356: //?all
        Serial.print("?all, Pan raw: "); Serial.print(MotionGetPanPositionRaw()); Serial.print(", Open raw: "); Serial.print(MotionGetOpenPositionRaw());
        Serial.print(", Pan Close Align: "); Serial.print(MotionGetPanCloseAlign(1)); Serial.print(", LTilt Close Align: "); Serial.print(MotionGetLeftTiltCloseAlign(1)); Serial.print(", RTilt Close Align: "); Serial.print(MotionGetRightTiltCloseAlign(1));
        Serial.print(", Closed switch: "); Serial.println(MotionGetClosedSwitch());
        break;

      case 1064329582: //?pan
        Serial.print("?pan, Pan raw: "); Serial.println(MotionGetPanPositionRaw());
        break;


        /*
          eye command
          -epos: eye position: #0 -127 to 127 for pan, #1 -127 to 127 for tilt
          -elnk: eye link to motion: #0 1 for linked, #0 0 for unlinked (auto unlink on epos command)
          -elig: eye light: #0 0 for off, #0 255 for full (scaled 0-255)
          -elas: eye laser: #0 0 for off, #0 1 for on

          gun fire commands
          -fire: guns fire: #0 0 or 1 for le
        */
    }

  } //end of response found

  //check if there is any error at this moment
  if (MotionGetError() > 0) { //if there is an error
    //turn off all relays
    RelaySet(0, 0);
    RelaySet(1, 0);
    RelaySet(2, 0);
    RelaySet(3, 0);
  }

  MotionUpdate(); //update the motion
}


void TurretStart() {
  //set relays to high
  RelaySet(0, 1); //set left relay
  RelaySet(1, 1); //set right relay
  RelaySet(2, 1); //set open relay
  RelaySet(3, 1); //set eye relay

  MotionStart(); //start up the motion
}

void TurretStop() {
  //set relays to low
  RelaySet(0, 0); //set left relay
  RelaySet(1, 0); //set right relay
  RelaySet(2, 0); //set open relay
  RelaySet(3, 0); //set eye relay
}

/*void TempEyeTest() {
  RelaySet(3, 1); //set eye relay on
  delay(1000);

  for (uint8_t e = 0; e < 255; e++) {
    EyeSetLed(e);
    delay(5);
  }

  EyeSetLaser(1);
  EyeMotorPower(1);
  EyeSetPosition(127, 127);
  delay(2000);

  EyeSetPosition(255, 0);
  delay(2000);
  EyeSetPosition(255, 255);
  delay(2000);
  EyeSetPosition(0, 255);
  delay(2000);
  EyeSetPosition(0, 0);
  delay(2000);

  EyeSetPosition(127, 127);
  delay(2000);

  EyeSetLaser(0);
  EyeMotorPower(0);

  for (uint8_t e = 255; e > 0; e--) {
    EyeSetLed(e);
    delay(5);
  }
  EyeSetLed(0);

  delay(1000);
  //RelaySet(3, 0); //set eye relay off
  delay(1000);
  }

  void TempPanTest() {
  RelaySet(3, 1); //set eye relay on
  delay(1000);

  Serial.println(MotionGetPanPositionRaw());
  MotionSetPanRaw(-1, 255);
  delay(500);
  Serial.println(MotionGetPanPositionRaw());
  MotionSetPanRaw(-1, 0);
  delay(1000);
  Serial.println(MotionGetPanPositionRaw());
  MotionSetPanRaw(1, 255);
  delay(1000);
  Serial.println(MotionGetPanPositionRaw());
  MotionSetPanRaw(1, 0);
  delay(1000);
  Serial.println(MotionGetPanPositionRaw());
  MotionSetPanRaw(-1, 255);
  delay(500);
  Serial.println(MotionGetPanPositionRaw());
  MotionSetPanRaw(-1, 0);
  delay(1000);

  delay(1000);
  //RelaySet(3, 0); //set eye relay off
  delay(1000);
  }

  void ManualTest() {
  if (Serial.available()) {
    char temp_char = Serial.read();
    if (temp_char == '0') {
      delay(1000);
    }
    else if (temp_char == '4') {
      Serial.println(MotionGetPanPositionRaw());
      MotionSetPanRaw(-1, 255);
      delay(200);
      Serial.println(MotionGetPanPositionRaw());
      MotionSetPanRaw(-1, 0);
    }
    else if (temp_char == '6') {
      Serial.println(MotionGetPanPositionRaw());
      MotionSetPanRaw(1, 255);
      delay(200);
      Serial.println(MotionGetPanPositionRaw());
      MotionSetPanRaw(1, 0);
    }
    else if (temp_char == '+') {
      Serial.println(MotionGetOpenPositionRaw());
      MotionSetOpenRaw(1, 255);
      delay(500);
      Serial.println(MotionGetOpenPositionRaw());
      MotionSetOpenRaw(1, 0);
    }
    else if (temp_char == '-') {
      Serial.println(MotionGetOpenPositionRaw());
      MotionSetOpenRaw(-1, 255);
      delay(500);
      Serial.println(MotionGetOpenPositionRaw());
      MotionSetOpenRaw(-1, 0);
    }
    else if (temp_char == '[') {
      RelaySet(3, 0); //set eye relay off
      RelaySet(2, 0); //set open relay off
      RelaySet(0, 0); //set left relay off
    }
    else if (temp_char == ']') {
      RelaySet(3, 1); //set eye relay on
      RelaySet(2, 1); //set open relay on
      RelaySet(0, 1); //set left relay on
    }
    else if (temp_char == ',') {
      EyeSetLed(0);
    }
    else if (temp_char == '.') {
      EyeSetLed(255);
    }
    else if (temp_char == '<') {
      EyeSetLaser(0);
      EyeMotorPower(0);
    }
    else if (temp_char == '>') {
      EyeSetLaser(1);
      EyeMotorPower(1);
      temp_eye_pan_pos = 127;
      temp_eye_tilt_pos = 127;
      EyeSetPosition(temp_eye_pan_pos, temp_eye_tilt_pos);
    }
    else if (temp_char == 'a') {
      temp_eye_pan_pos += 10;
      EyeSetPosition(temp_eye_pan_pos, temp_eye_tilt_pos);
    }
    else if (temp_char == 'd') {
      temp_eye_pan_pos -= 10;
      EyeSetPosition(temp_eye_pan_pos, temp_eye_tilt_pos);
    }
    else if (temp_char == 'w') {
      temp_eye_tilt_pos += 10;
      EyeSetPosition(temp_eye_pan_pos, temp_eye_tilt_pos);
    }
    else if (temp_char == 's') {
      temp_eye_tilt_pos -= 10;
      EyeSetPosition(temp_eye_pan_pos, temp_eye_tilt_pos);
    }
    else if (temp_char == 't') {
      MotionFlywheelPower(1);
      MotionFlywheelSpeed(0);
      MotionGunLeftMasterFeed(1);
      delay(2000);
      MotionFlywheelSpeed(100);
      delay(500);
      MotionGunLeftFeed(0, 1);
      delay(500);
      MotionGunLeftFeed(1, 1);
      delay(10000);
      MotionGunLeftFeed(0, 0);
      MotionGunLeftFeed(1, 0);
      delay(2000);
      MotionFlywheelSpeed(0);
      MotionFlywheelPower(0);
      MotionGunLeftMasterFeed(0);
    }
  }
  }

  /*flywheel test
   MotionFlywheelPower(1);
      MotionFlywheelSpeed(0);
      delay(2000);
      MotionFlywheelSpeed(50);
      delay(2000);
      MotionFlywheelSpeed(100);
      delay(2000);
      MotionFlywheelSpeed(150);
      delay(2000);
      MotionFlywheelSpeed(0);
      MotionFlywheelPower(0);
*/

/* tilt test
  MotionTiltPower(1);
      MotionTiltPosition(127);
      delay(2000);
      MotionTiltPosition(0);
      delay(2000);
      MotionTiltPosition(50);
      delay(2000);
      MotionTiltPosition(100);
      delay(2000);
      MotionTiltPosition(150);
      delay(2000);
      MotionTiltPosition(200);
      delay(2000);
      MotionTiltPosition(250);
      delay(2000);
      MotionTiltPosition(127);
      delay(2000);
      MotionTiltPower(0);
*/

/*
   fire demo
   MotionFlywheelPower(1);
     MotionFlywheelSpeed(0);
     MotionGunLeftMasterFeed(1);
     delay(2000);
     MotionFlywheelSpeed(100);
     delay(500);
     MotionGunLeftFeed(0,1);
     delay(500);
     MotionGunLeftFeed(1,1);
     delay(10000);
     MotionGunLeftFeed(0,0);
     MotionGunLeftFeed(1,0);
     delay(2000);
     MotionFlywheelSpeed(0);
     MotionFlywheelPower(0);
     MotionGunLeftMasterFeed(0);
*/
