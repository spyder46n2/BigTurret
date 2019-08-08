/*
  The motion control tab controlls all motion, and all software safeties for said motion
  The main loop "MotionUpdate" holds all of the main functions. It checks whether any part of the turret is allowed to move
  and where all parts need to move

  When the turret is closed, the eye will automatically track to the given gun position.
  When the turret is opened, the eye will follow the gun position if the 2 are linked

  list of error codes:
  -10: pan out of bounds error
  -15: open out of bounds error
  -20: pan axis locked
  -25: open axis locked
  -50: opening timeout error
  -55: closing return to center timeout
  -56: closing movement timeout
  -80: Pan passage check error
  -85: Left tilt passage check error
  -86: Right tilt passage check error

  list of warning codes:
  -10: Left gun top feeder locked
  -15: Left gun bottom feeder locked
  -20: Right gun top feeder locked
  -25: Right gun bottom feeder locked
  -30: Left gun top feeder runaway
  -35: Left gun bottom feeder runaway
  -40: Right gun top feeder runaway
  -45: Right gun bottom feeder runaway
*/

#include <Servo.h>

uint8_t gun_open_invert = 1;
const uint8_t gun_open_motion[2] = {6, 7};
const uint8_t gun_open_pos[2] = {A2, A3};
const uint8_t gun_closed_check_main = 36;
const uint8_t gun_closed_check_guns[2] = {37, 38};
uint16_t gun_open_limits[2] = {470 , 768}; //closed and opened
uint8_t gun_open_error_bars = 5; //how close to target open needs to be to stop

uint8_t gun_pan_invert = 1;
const uint8_t gun_pan_motion[2] = {4, 5};
const uint8_t gun_pan_pos[2] = {A0, A1};
const uint8_t gun_pan_close_align = A7;
uint16_t gun_pan_limits[2] = {568 , 387}; //left most and right most (default 573,392
uint8_t gun_pan_error_bars = 3; //how close to target pan needs to be to stop

const uint8_t gun_left_flywheel = 22;
Servo gun_left_flywheel_servo;
const uint8_t gun_left_tilt = 26;
Servo gun_left_tilt_servo;
const uint8_t gun_left_close_align = A8;
const uint8_t gun_left_master_feed = 39;
const uint8_t gun_left_feeder[2] = {32, 33};
const uint8_t gun_left_feed_check[2] = {28, 29};

const uint8_t gun_left_invert = 1;
const uint8_t gun_left_tilt_center = 92;

const uint8_t gun_right_flywheel = 23;
Servo gun_right_flywheel_servo;
const uint8_t gun_right_tilt = 27;
Servo gun_right_tilt_servo;
const uint8_t gun_right_close_align = A9;
const uint8_t gun_right_master_feed = 40;
const uint8_t gun_right_feeder[2] = {34, 35};
const uint8_t gun_right_feed_check[2] = {30, 31};

const uint8_t gun_right_invert = 0;
const uint8_t gun_right_tilt_center = 88;

const uint8_t gun_armed_switch = 9;

const uint8_t gun_motion_range = 70; //how far the gun should move

const uint8_t eye_laser = 8;
const uint8_t eye_led = 13;
const uint8_t eye_pan = 24;
const uint8_t eye_tilt = 25;

uint8_t eye_pan_limit[2] = {94, 70};
uint8_t eye_tilt_limit[2] = {133, 63};
uint8_t eye_height_limiter = 0; //whether the eye can move up past center

Servo eye_pan_servo;
Servo eye_tilt_servo;

uint8_t motion_error_state = 0; //whether there is an error
uint8_t motion_warning_state = 0; //whether there is a warning
uint32_t motion_warning_target;
uint32_t motion_warning_delay = 1000;
uint8_t motion_error_bars = 10; //by how much a potmeter can vary
uint8_t motion_error_override = 0; //whether all safeties can be overridden or not

uint16_t motion_eye_pan_target, motion_eye_tilt_target; //eye desired position
uint16_t motion_gun_pan_target, motion_gunl_tilt_target, motion_gunr_tilt_target; //gun desired position
uint16_t motion_gun_pan_position, motion_gunl_tilt_position, motion_gunr_tilt_position; //gun pan/tilt current position
uint8_t motion_gun_pan_at_target, motion_gunl_tilt_at_target, motion_gunr_tilt_at_target; //whether the position is at the target
uint16_t motion_gun_pan_history, motion_gunl_tilt_history, motion_gunr_tilt_history; //the previous position the gun
uint8_t motion_gun_pan_power; //how much power is on the pan motor
int16_t motion_gunl_tilt_speed, motion_gunr_tilt_speed; //how fast the guns are tilting (if applicable)
int16_t motion_gun_tilt_max_speed = 12; //max speed in degrees per tick (currently 20 ticks per second)

uint32_t motion_pan_lock_target;
uint8_t motion_pan_lock_power_threshold = 10; //the lowest amount of power that is considered 'unpowered'
uint32_t motion_pan_lock_time = 2000; //how long the motor needs to stay unmoving to call a lock
uint32_t motion_pan_lock_pot_history; //the previous potmeter value
uint16_t motion_pan_lock_noise = 5; //by how much the pot needs to change to be considered moving

uint32_t motion_open_lock_target;
uint8_t motion_open_lock_power_threshold = 10; //the lowest amount of power that is considered 'unpowered'
uint32_t motion_open_lock_time = 2000; //how long the motor needs to stay unmoving to call a lock
uint32_t motion_open_lock_pot_history; //the previous potmeter value
uint16_t motion_open_lock_noise = 5; //by how much the pot needs to change to be considered moving


uint16_t motion_open_target; //open desired position
uint16_t motion_open_position; //where open is right now
uint8_t motion_open_at_target; //whether motion is at target
uint8_t motion_open_power; //how much power is on the open motor
uint32_t motion_open_timeout_target; //the variable checking if the open movement is about to timeout
uint32_t motion_open_timeout_time = 15000; //how long the open command can last before it timeouts

uint8_t motion_gun_tilt_link; //whether the both guns are linked
uint8_t motion_eye_gun_link; //whether the eye position is linked to the gun position
uint8_t motion_eye_block_tracking; //a temporary block after the turret is opened, to prevent the eye from snapping to center
int8_t motion_open_state; //whether the turret is open or closed (2 is opening, 1 is open, 0 is closed, -1 is closing)

uint32_t motion_update_target;
uint32_t motion_update_delay = 50; //the wait in ms between motion updates

//passage check variables
uint32_t motion_passage_check_target;
uint32_t motion_passage_check_delay = 25; //how often the passage check is done
uint16_t motion_passage_tilt_window = 20; //the size of the window (past the center, both sides) in which the flag has had to be seen
uint16_t motion_passage_pan_window = 20; //the size of the window (past the center, both sides) in which the flag has had to be seen
uint8_t motion_passage_pan_seen = 0;
uint8_t motion_passage_tiltl_seen = 0;
uint8_t motion_passage_tiltr_seen = 0;
int8_t motion_passage_pan_state = 0;
int8_t motion_passage_tiltl_state = 0;
int8_t motion_passage_tiltr_state = 0;

//gun feeder variables
uint8_t motion_gun_fire_power = 50; //(0 to 180)
uint8_t motion_gun_fire_all = 0;
uint8_t motion_gun_fire_state = 0;
uint32_t motion_gun_fire_time;
uint32_t motion_gun_fire_state_delay = 100;

uint32_t motion_gun_feeder_check_target;
uint32_t motion_gun_feeder_check_delay = 25; //how often the guns need to be checked
uint8_t motion_gun_feeder_first_start = 1; //first power of the feeders is allowed
uint8_t motion_gunl_feeder_powered = 0;
uint8_t motion_gunr_feeder_powered = 0;

uint32_t motion_gun_feeder_max_lock_time = 2000; //how long any feeder can be stuck before the power is cut
uint32_t motion_gunl_top_feeder_trigger, motion_gunl_bottom_feeder_trigger;
uint32_t motion_gunr_top_feeder_trigger, motion_gunr_bottom_feeder_trigger;
uint8_t motion_gunl_top_feeder_locked = 0;
uint8_t motion_gunl_bottom_feeder_locked = 0;
uint8_t motion_gunr_top_feeder_locked = 0;
uint8_t motion_gunr_bottom_feeder_locked = 0;

//wiggle variables
uint8_t motion_wiggle_max_deflection = 5; //the maximum number of wiggle allowed
uint32_t motion_wiggle_target;
uint32_t motion_wiggle_delay = 1000;
int8_t motion_wiggle_pan_offset, motion_wiggle_ltilt_offset, motion_wiggle_rtilt_offset;
//Be aware that rtilt offset is compared to the offset value after ltilt. Normally the motion handler only considers ltilt

//eye light variables
uint8_t motion_eye_led_value = 0;
uint8_t motion_eye_led_target = 0;
uint8_t motion_eye_led_history = 0;
uint16_t motion_eye_led_delay = 0;
uint32_t motion_eye_led_time_started, motion_eye_led_time_target;
uint32_t motion_eye_led_update_target;
uint32_t motion_eye_led_update_delay = 50;



void MotionBegin() {
  pinMode(gun_open_motion[0], OUTPUT);
  pinMode(gun_open_motion[1], OUTPUT);
  pinMode(gun_open_pos[0], INPUT);
  pinMode(gun_open_pos[1], INPUT);
  pinMode(gun_closed_check_main, INPUT);
  pinMode(gun_closed_check_guns[0], INPUT);
  pinMode(gun_closed_check_guns[1], INPUT);

  pinMode(gun_pan_motion[0], OUTPUT);
  pinMode(gun_pan_motion[1], OUTPUT);
  pinMode(gun_pan_pos[0], INPUT);
  pinMode(gun_pan_pos[1], INPUT);
  pinMode(gun_pan_close_align, INPUT);

  pinMode(gun_left_flywheel, OUTPUT);
  pinMode(gun_left_tilt, OUTPUT);
  pinMode(gun_left_close_align, INPUT);
  pinMode(gun_left_master_feed, OUTPUT);
  pinMode(gun_left_feeder[0], OUTPUT);
  pinMode(gun_left_feeder[1], OUTPUT);
  pinMode(gun_left_feed_check[0], INPUT);
  pinMode(gun_left_feed_check[1], INPUT);

  pinMode(gun_right_flywheel, OUTPUT);
  pinMode(gun_right_tilt, OUTPUT);
  pinMode(gun_right_close_align, INPUT);
  pinMode(gun_right_master_feed, OUTPUT);
  pinMode(gun_right_feeder[0], OUTPUT);
  pinMode(gun_right_feeder[1], OUTPUT);
  pinMode(gun_right_feed_check[0], INPUT);
  pinMode(gun_right_feed_check[1], INPUT);

  pinMode(eye_laser, OUTPUT);
  pinMode(eye_led, OUTPUT);
  pinMode(eye_pan, OUTPUT);
  pinMode(eye_tilt, OUTPUT);

  pinMode(gun_armed_switch, INPUT);

  digitalWrite(eye_laser, 0);
  digitalWrite(eye_led, 0);

  MotionFlywheelPower(1);
  MotionFlywheelSpeed(0);

  motion_gun_tilt_link = 1; //link both gun tilts
  motion_eye_gun_link = 1; //default eye link to gun position

  motion_gun_pan_target = 127; //set pan to center
  motion_gun_pan_history = motion_gun_pan_target;
  motion_gunl_tilt_position = gun_left_tilt_center; //set left side variables
  motion_gunl_tilt_target = motion_gunl_tilt_position;
  motion_gunl_tilt_history = motion_gunl_tilt_position;
  motion_gunr_tilt_position = gun_left_tilt_center; //set right side variables
  motion_gunr_tilt_target = motion_gunr_tilt_position;
  motion_gunr_tilt_history = motion_gunr_tilt_position;
}

void MotionUpdate() {
  //motion_error_state = 0; //temporary override of errors
  if (motion_error_state == 0 || motion_error_override == 1) { //if there is no error or there is an override
    if (millis() > motion_update_target) {
      motion_update_target = millis() + motion_update_delay; //set new time
      //Serial.println("MUp");

      //set watchdog alive flag

      uint8_t temp_allow_gun_motion = 0;
      uint8_t temp_allow_open_motion = 0; //allows open motion
      uint16_t temp_gun_pan_target, temp_gun_tilt_target, temp_gun_rtilt_target;

      //get potmeter values
      motion_gun_pan_position = MotionGetPanPositionRaw(); //get pan position
      motion_open_position = MotionGetOpenPositionRaw(); //get open position

      //check if gun is where it wants to be
      if (motion_open_state == 1) { //if gun is confirmed open, allow motion
        temp_allow_gun_motion = 1; //under open conditions, unconditional movement allowed
        temp_gun_pan_target = motion_gun_pan_target; //write to motion values (but keep originals)
        temp_gun_tilt_target = motion_gunl_tilt_target; //write to motion values (but keep originals)
        motion_wiggle_rtilt_offset = 0; //reset r wiggle offset to 0 (it is actually handled in the main loop)
      }

      //if closing or opening
      else if (motion_open_state == 2) { //if opening, open and go to target
        temp_allow_open_motion = 1;
        if (motion_open_at_target == 1) {
          Serial.println("OP1");
          motion_open_state = 1;
          motion_eye_block_tracking = 1; //set the temporary eye unlink function
          temp_allow_open_motion = 0;
        }
        else {
          MotionSetOpenTarget(255); //set target to open
        }
        if (millis() > motion_open_timeout_target) {
          motion_error_state = 50; //set opening timeout
          //motion_open_timeout_time
        }

      }

      else if (motion_open_state == -1) { //if closing, go to center, check flags
        //go to center
        temp_allow_gun_motion = 1; //allow movement (unchecked later if necessary)
        temp_gun_pan_target = 127;
        temp_gun_tilt_target = 127;
        uint8_t temp_closed = 0;
        if (motion_error_override == 0){
          if (motion_gun_pan_at_target == 1 && motion_gunl_tilt_at_target == 1 && MotionGetPanCloseAlign(1) > 850 && MotionGetLeftTiltCloseAlign(1) > 850 && MotionGetRightTiltCloseAlign(1) > 850 && MotionFeedersClear() == 1) { //if the gun is at the center
            temp_closed = 1;
          }
        }
        if (motion_error_override == 1){
          if (motion_gun_pan_at_target == 1 && motion_gunl_tilt_at_target == 1) { //if the gun is at the center without extra checks
            temp_closed = 1;
          }
        }
        if (temp_closed == 1) { //if the gun is at the center
          Serial.println("CL1");
          motion_open_timeout_target = millis() + motion_open_timeout_time; //reset timeout
          temp_allow_gun_motion = 0;
          temp_allow_open_motion = 1;
          MotionSetOpenTarget(0); //set target to closed
          motion_open_state = -2;
        }

        if (millis() > motion_wiggle_target){ //wiggle check
          motion_wiggle_target = millis() + motion_wiggle_delay; //set new wiggle update
          if (motion_gun_pan_at_target == 1){ //if the axes are supposed to be on target
            if (MotionGetPanCloseAlign(1) <= 920){ //check if pan needs to move
              Serial.println("PAN MISAL");
              motion_wiggle_pan_offset = 0; //reset wiggle
              motion_wiggle_pan_offset = random(-motion_wiggle_max_deflection, motion_wiggle_max_deflection); //get a random wiggle
            }
          }
          if (motion_gunl_tilt_at_target == 1){
            if (MotionGetLeftTiltCloseAlign(1) <= 920){ //check if ltilt needs to move
              Serial.println("LTIL MISAL");
              motion_wiggle_ltilt_offset = 0;
              motion_wiggle_ltilt_offset = random(-motion_wiggle_max_deflection, motion_wiggle_max_deflection); //get a random wiggle
            }
          
            if (MotionGetRightTiltCloseAlign(1) <= 920){ //check if rtilt needs to move
              Serial.println("RTIL MISAL");
              motion_wiggle_rtilt_offset = 0;
              motion_wiggle_rtilt_offset = random(-motion_wiggle_max_deflection, motion_wiggle_max_deflection); //get a random wiggle
              //Serial.println(motion_wiggle_rtilt_offset);
            }
          }
        }
        temp_gun_pan_target += motion_wiggle_pan_offset; //add or subtract wiggle
        temp_gun_tilt_target += motion_wiggle_ltilt_offset; //add of subtract offset
        if (motion_wiggle_rtilt_offset != 0){
          temp_gun_tilt_target += random(-1,1); //add another random to tilt if the r side motion requires movement to force movement
        }
        //rtilt is not added, since it is handled MUCH further in the actual motion block (sorry, I was a little shortsigted here)
        
        if (millis() > motion_open_timeout_target) {
          motion_error_state = 55; //set closing timeout
          //motion_open_timeout_time
        }
      }

      else if (motion_open_state == -2) { //if closing, close
        temp_allow_gun_motion = 0;
        temp_allow_open_motion = 1;

        if (motion_open_at_target == 1 && MotionGetClosedSwitch() == 1) {
          Serial.println("CL2");
          motion_open_state = 0;
          temp_allow_gun_motion = 0;
          temp_allow_open_motion = 0;
        }
        else {
          MotionSetOpenTarget(0); //set target to closed
        }
        if (millis() > motion_open_timeout_target) {
          motion_error_state = 56; //set closing timeout
          //motion_open_timeout_time
        }
      }

      //if closed
      else if (motion_open_state == 0) { //if closed, make sure everything is unpowered
        temp_allow_gun_motion = 0;
        temp_allow_open_motion = 0;
      }

      if (temp_allow_gun_motion == 1) { //when motion is allowed, update the motion loop
        //check DC positions
        //check where the potmeter is right now
        //Set DC motor driven movements ############################
        static uint16_t st_gun_pan_pot_target;
        st_gun_pan_pot_target = map(temp_gun_pan_target, 0, 255, gun_pan_limits[0], gun_pan_limits[1]); //check what potmeter positions are desired
        //Serial.print("Pan: "); Serial.print(motion_gun_pan_position); Serial.print(", "); Serial.println(st_gun_pan_pot_target);
        //set the motor to move towards the position
        if (motion_gun_pan_position > st_gun_pan_pot_target - gun_pan_error_bars && motion_gun_pan_position < st_gun_pan_pot_target + gun_pan_error_bars) { //motion is at target
          MotionSetPanRaw(1, 0);
          motion_gun_pan_at_target = 1; //mark pan motion as at target
          motion_eye_block_tracking = 0; //relink eyes to gpos if this was blocked after opening
        }
        else if (motion_gun_pan_position > st_gun_pan_pot_target) { //if target is bigger (requires positive movement)
          MotionSetPanRaw(1, 255);
          motion_gun_pan_at_target = 0; //mark the pan motion as not at target
        }
        else if (motion_gun_pan_position < st_gun_pan_pot_target) { //if target is smaller (requires negative movement)
          MotionSetPanRaw(-1, 255);
          motion_gun_pan_at_target = 0; //mark the pan motion as not at target
        }

        //check tilt position values #######################
        if (motion_gun_tilt_link == 1) { //if there is a link between the gun tilts
        }

        //determine required speed to match tilt to pan --------------------
        //give position purely on where we want to be, excluding max speed, left side---
        uint16_t temp_pan_byte_pos = map(motion_gun_pan_position, gun_pan_limits[0], gun_pan_limits[1], 0, 255); //get current byte pos for pan
        //desired position
        uint16_t temp_current_target = map(temp_pan_byte_pos, motion_gun_pan_history, temp_gun_pan_target, motion_gunl_tilt_history, temp_gun_tilt_target);
        if (motion_gun_pan_history == temp_gun_pan_target) {
          temp_current_target = temp_gun_tilt_target; //set current target to actual target if pan does not move at all
        }
        if (temp_gun_tilt_target > motion_gunl_tilt_position) { //if target requires positive move
          motion_gunl_tilt_speed = temp_current_target - motion_gunl_tilt_position; //set desired speed
          //Serial.print("+Limited: "); Serial.println(motion_gunl_tilt_speed);
          motion_gunl_tilt_speed = constrain(motion_gunl_tilt_speed, 0, 255); //limit speed to positive
        }
        else if (temp_gun_tilt_target < motion_gunl_tilt_position) { //if target requires negative move
          motion_gunl_tilt_speed = motion_gunl_tilt_position - temp_current_target; //set desired speed
          //Serial.print("-Limited: "); Serial.println(motion_gunl_tilt_speed);
          motion_gunl_tilt_speed = constrain(motion_gunl_tilt_speed, 0, 255); //limit speed to positive
        }
        if (motion_gunl_tilt_speed > motion_gun_tilt_max_speed) { //limit speed to max speed
          motion_gunl_tilt_speed = motion_gun_tilt_max_speed;
        }
        if (motion_gunl_tilt_speed == 0) {
          motion_gunl_tilt_speed = 1; //set to one to force final movements
        }

        //move  servo position to new target---
        if (temp_gun_tilt_target > motion_gunl_tilt_position) { //if target requires positive move
          if (temp_gun_tilt_target - motion_gunl_tilt_position < motion_gunl_tilt_speed) { //check if move is smaller than current speed
            motion_gunl_tilt_position = temp_gun_tilt_target; //set pos to target
          }
          else {
            motion_gunl_tilt_position += motion_gunl_tilt_speed; //add speed to position
          }
          MotionLeftTiltRaw(motion_gunl_tilt_position);
          MotionRightTiltRaw(motion_gunl_tilt_position+motion_wiggle_rtilt_offset);
          motion_gunl_tilt_at_target = 0;
          motion_gunr_tilt_at_target = 0;
        }
        else if (temp_gun_tilt_target < motion_gunl_tilt_position) { //if target requires negative move
          if (motion_gunl_tilt_position - temp_gun_tilt_target < motion_gunl_tilt_speed) { //check if move is smaller than current speed
            motion_gunl_tilt_position = temp_gun_tilt_target; //set pos to target
          }
          else {
            motion_gunl_tilt_position -= motion_gunl_tilt_speed; //add speed to position
          }
          MotionLeftTiltRaw(motion_gunl_tilt_position);
          MotionRightTiltRaw(motion_gunl_tilt_position+motion_wiggle_rtilt_offset); //the wiggle offset is also added here
          motion_gunl_tilt_at_target = 0;
          motion_gunr_tilt_at_target = 0;
        }
        else { //no movement
          motion_gunl_tilt_at_target = 1;
          motion_gunr_tilt_at_target = 1;
        }
      }
      else { //soft disable everything
        MotionSetPanRaw(1, 0); //set pan motor off
      }

      if (temp_allow_open_motion == 1) { //if open and close movement is allowed
        //Serial.println("open movement");
        static uint16_t st_open_pot_target;
        st_open_pot_target = map(motion_open_target, 0, 255, gun_open_limits[0], gun_open_limits[1]); //check what potmeter positions are desired
        //Serial.print("Pan: "); Serial.print(motion_open_position); Serial.print(", "); Serial.println(st_open_pot_target);

        //set the motor to move towards the position
        if (motion_open_position > st_open_pot_target - gun_open_error_bars && motion_open_position < st_open_pot_target + gun_open_error_bars) { //motion is at target
          MotionSetOpenRaw(1, 0);
          motion_open_at_target = 1;
          //Serial.println("open at target");
        }
        else if (motion_gun_pan_position > st_open_pot_target) { //if target is bigger (requires positive movement)
          MotionSetOpenRaw(-1, 255);
          motion_open_at_target = 0;
        }
        else if (motion_gun_pan_position < st_open_pot_target) { //if target is smaller (requires negative movement)
          MotionSetOpenRaw(1, 255);
          motion_open_at_target = 0;
        }
      }
      else {
        MotionSetOpenRaw(1, 0); //set open motor off
        //Serial.println("else all off");
      }

      //error checks ################
      //check if a motion is errorous
      if (gun_pan_limits[0] < gun_pan_limits[1]) { //if potmeter is not inverted
        if (motion_gun_pan_position < gun_pan_limits[0] - motion_error_bars || motion_gun_pan_position > gun_pan_limits[1] + motion_error_bars) { //check potmeter out of bounds
          motion_error_state = 10; //set pan error 10
        }
      }
      else { //if potmeter is inverted
        if (motion_gun_pan_position < gun_pan_limits[1] - motion_error_bars || motion_gun_pan_position > gun_pan_limits[0] + motion_error_bars) { //check potmeter out of bounds
          motion_error_state = 10; //set pan error 10
        }
      }

      if (gun_open_limits[0] < gun_open_limits[1]) { //if potmeter is not inverted
        if (motion_open_position < gun_open_limits[0] - motion_error_bars || motion_open_position > gun_open_limits[1] + motion_error_bars) { //check potmeter out of bounds
          motion_error_state = 15; //set pan error 15
        }
      }
      else { //if potmeter is inverted
        if (motion_open_position < gun_open_limits[1] - motion_error_bars || motion_open_position > gun_open_limits[0] + motion_error_bars) { //check potmeter out of bounds
          motion_error_state = 15; //set pan error 15
        }
      }

      //check locked position (powered but unmoving)
      if (motion_gun_pan_power > motion_pan_lock_power_threshold) { //check if motor is powered
        if (motion_gun_pan_position < motion_pan_lock_pot_history - motion_pan_lock_noise || motion_gun_pan_position > motion_pan_lock_pot_history + motion_pan_lock_noise) { //see if potmeter has changed
          motion_pan_lock_target = millis() + motion_pan_lock_time; //set new last moved time
          motion_pan_lock_pot_history = motion_gun_pan_position; //set new potentiometer history
          //Serial.println("locked changed");
        }

        if (millis() > motion_pan_lock_target ) { //if the timer has ran out.
          motion_error_state = 20; //set pan lock timer overflow
        }
      }
      else {
        motion_pan_lock_target = millis() + motion_pan_lock_time; //reset the timer to prevent issues
      }

      if (motion_open_power > motion_open_lock_power_threshold) { //check if motor is powered
        if (motion_open_position < motion_open_lock_pot_history - motion_open_lock_noise || motion_open_position > motion_open_lock_pot_history + motion_open_lock_noise) { //see if potmeter has changed
          motion_open_lock_target = millis() + motion_open_lock_time; //set new last moved time
          motion_open_lock_pot_history = motion_open_position; //set new potentiometer history
          //Serial.println("locked changed");
        }

        if (millis() > motion_open_lock_target ) { //if the timer has ran out.
          motion_error_state = 25; //set pan lock timer overflow
        }
      }
      else {
        motion_open_lock_target = millis() + motion_open_lock_time; //reset the timer to prevent issues
      }

      //check if the eye is linked to the gun, if so, match eye with gun (direct control is handled in the main loop)
      if (motion_eye_gun_link == 1) {
        if (motion_open_state == 1 && motion_eye_block_tracking == 0) { //if eyes follow the measured movement and this is not blocked
          uint16_t temp_pan_byte_pos;
          temp_pan_byte_pos = map(motion_gun_pan_position, gun_pan_limits[0], gun_pan_limits[1], 0, 255); //get current byte pos for pan
          EyeSetPosition(temp_pan_byte_pos, motion_gunl_tilt_position);
        }
        else { //if turret is closed and the eyes are free to move
          EyeSetPosition(motion_gun_pan_target, motion_gunl_tilt_target);
        }
      }
    } //end of motion update

    //preform passage check
    if (millis() > motion_passage_check_target) {
      motion_passage_check_target = millis() + motion_passage_check_delay;
      //passage check looks if the close align sensor is triggered when any of the movements should move past. If not, something is not aligned, and an error is given
      if (motion_open_state == 1) { //only check when motion is enabled
        //general function. Get a state for each axis. 1 is moving towards positive, -1 is moving towards negative, 0 is starting state
        //if the axis is past the center plus an offset, the state is set to the other side. If it was 1, is now -1, and nothing was seen, there is an error
        //a variable keeps the flag seen value. It resets on every state toggle.
        //Serial.println("Passage check");

        //get flag states
        uint8_t temp_pan_flag = MotionGetPanCloseAlign(0);
        uint8_t temp_tiltl_flag = MotionGetLeftTiltCloseAlign(0);
        uint8_t temp_tiltr_flag = MotionGetRightTiltCloseAlign(0);

        //set seen states
        if (temp_pan_flag == 1) motion_passage_pan_seen = 1;
        if (temp_tiltl_flag == 1) motion_passage_tiltl_seen = 1;
        if (temp_tiltr_flag == 1) motion_passage_tiltr_seen = 1;
        //Serial.println(temp_tiltl_flag);

        //get current positions in byte values
        uint16_t temp_pan_pos = map(motion_gun_pan_position, gun_pan_limits[0], gun_pan_limits[1], 0, 255);
        uint16_t temp_tiltl_pos = motion_gunl_tilt_position;
        uint16_t temp_tiltr_pos = motion_gunl_tilt_position; //for now right is one on one linked
        //Serial.print(temp_tiltl_pos); Serial.print(","); Serial.println(temp_tiltr_pos);

        //check if a state needs to be toggled
        //check pan
        if (temp_pan_pos < 127 - motion_passage_pan_window) {
          //Serial.println("Ltilt lower check");
          if (motion_passage_pan_state == 0 || (motion_passage_pan_state == 1 && motion_passage_pan_seen == 1)) {
            //Serial.println("Pan Lower check OK");
            motion_passage_pan_state = -1;
            motion_passage_pan_seen = 0;
          }
          if (motion_passage_pan_state == 1 && motion_passage_pan_seen == 0) {
            //set error
            motion_error_state = 80;
          }
        }
        else if (temp_pan_pos > 127 + motion_passage_pan_window) {
          //Serial.println("Ltilt upper check");
          if (motion_passage_pan_state == 0 || (motion_passage_pan_state == -1 && motion_passage_pan_seen == 1)) {
            //Serial.println("Pan upper check OK");
            motion_passage_pan_state = 1;
            motion_passage_pan_seen = 0;
          }
          if (motion_passage_pan_state == -1 && motion_passage_pan_seen == 0) {
            //set error
            motion_error_state = 80;
          }
        }

        //check left tilt
        if (temp_tiltl_pos < 127 - motion_passage_tilt_window) {
          if (motion_passage_tiltl_state == 0 || (motion_passage_tiltl_state == 1 && motion_passage_tiltl_seen == 1)) {
            //Serial.println("LTilt Lower check OK");
            motion_passage_tiltl_state = -1;
            motion_passage_tiltl_seen = 0;
          }
          if (motion_passage_tiltl_state == 1 && motion_passage_tiltl_seen == 0) {
            //set error
            motion_error_state = 85;
          }
        }
        else if (temp_tiltl_pos > 127 + motion_passage_tilt_window) {
          if (motion_passage_tiltl_state == 0 || (motion_passage_tiltl_state == -1 && motion_passage_tiltl_seen == 1)) {
            //Serial.println("LTilt Upper check OK");
            motion_passage_tiltl_state = 1;
            motion_passage_tiltl_seen = 0;
          }
          if (motion_passage_tiltl_state == -1 && motion_passage_tiltl_seen == 0) {
            //set error
            motion_error_state = 85;
          }
        }

        //check right tilt
        if (temp_tiltr_pos < 127 - motion_passage_tilt_window) {
          if (motion_passage_tiltr_state == 0 || (motion_passage_tiltr_state == 1 && motion_passage_tiltr_seen == 1)) {
            //Serial.println("RTilt Lower check OK");
            motion_passage_tiltr_state = -1;
            motion_passage_tiltr_seen = 0;
          }
          if (motion_passage_tiltr_state == 1 && motion_passage_tiltr_seen == 0) {
            //set error
            motion_error_state = 86;
          }
        }
        else if (temp_tiltr_pos > 127 + motion_passage_tilt_window) {
          if (motion_passage_tiltr_state == 0 || (motion_passage_tiltr_state == -1 && motion_passage_tiltr_seen == 1)) {
            //Serial.println("RTilt Upper check OK");
            motion_passage_tiltr_state = 1;
            motion_passage_tiltr_seen = 0;
          }
          if (motion_passage_tiltr_state == -1 && motion_passage_tiltr_seen == 0) {
            //set error
            motion_error_state = 86;
          }
        }
      }
    } //end of passage check


    if (millis() > motion_gun_feeder_check_target) {
      if (motion_open_state == 1) { //only check when open
        motion_gun_feeder_check_target = millis() + motion_gun_feeder_check_delay; //set new target

        //check if the feeders can be powered
        if (motion_gunl_feeder_powered == 1 || motion_gun_feeder_first_start == 1) {
          motion_gunl_feeder_powered = 1;
          MotionGunLeftMasterFeed(1);
        }
        else {
          MotionGunLeftMasterFeed(0);
        }

        if (motion_gunr_feeder_powered == 1 || motion_gun_feeder_first_start == 1) {
          motion_gunr_feeder_powered = 1;
          MotionGunRightMasterFeed(1);
        }
        else {
          MotionGunRightMasterFeed(0);
        }
        if (motion_gun_feeder_first_start == 1) {
          motion_gun_feeder_first_start = 0; //reset first start
          //reset lock times
          motion_gunl_top_feeder_trigger = millis() + motion_gun_feeder_max_lock_time;
          motion_gunl_bottom_feeder_trigger = millis() + motion_gun_feeder_max_lock_time;
          motion_gunr_top_feeder_trigger = millis() + motion_gun_feeder_max_lock_time;
          motion_gunr_bottom_feeder_trigger = millis() + motion_gun_feeder_max_lock_time;
        }

        //check if fire there are outstanding fire commands //change start sequence based on start time and state machine-----------------
        if (motion_gun_fire_all == 1 && MotionGetArmed() == 1) { //fire sequence
          //check if time requires a higher state
          if (millis() >  motion_gun_fire_time) {
            motion_gun_fire_time = millis() + motion_gun_fire_state_delay;
            motion_gun_fire_state ++;
            if (motion_gun_fire_state > 200){
              motion_gun_fire_state = 200; //cap maximum value
            }
            if (motion_gunl_feeder_powered == 0 && motion_gunr_feeder_powered == 0){ //if any of the feeders are unpowered, do not run state machine
              motion_gun_fire_state = 200; 
            }

            //state machine for the steps
            switch (motion_gun_fire_state) {
              case 1: //spool up flywheels, enable master feed
                MotionFlywheelSpeed(motion_gun_fire_power);
                MotionGunLeftMasterFeed(1);
                MotionGunRightMasterFeed(1);
                break;
              case 4: //start first gun
                MotionGunLeftFeed(0, 1);
                break;
              case 5: //start second gun
                MotionGunRightFeed(0, 1);
                break;
              case 7: //start third gun
                MotionGunLeftFeed(1, 1);
                break;
              case 8: //start fourth gun
                MotionGunRightFeed(1, 1);
                break;
            }
          }
        }
        else {
          Serial.println("Guns Not Armed");
          MotionFireAll(0); //reset guns          
        }
        if (motion_gun_fire_all == 0) { //cease sequence sequence
          //check if time requires a higher state
          if (millis() >  motion_gun_fire_time) {
            motion_gun_fire_time = millis() + motion_gun_fire_state_delay;
            motion_gun_fire_state ++;
            if (motion_gun_fire_state > 200){
              motion_gun_fire_state = 200; //cap maximum value
            }

            //state machine for the steps
            switch (motion_gun_fire_state) {
              case 1: //stop feeders
                MotionGunLeftFeed(0, 0);
                MotionGunLeftFeed(1, 0);
                MotionGunRightFeed(0, 0);
                MotionGunRightFeed(1, 0);
                break;
              case 9: //stop flywheels
                MotionFlywheelSpeed(0);
                break;
              case 10: //stop master feed
                MotionGunLeftMasterFeed(0);
                MotionGunRightMasterFeed(0);
                break;
              case 15: //check if any switch is not triggered
                if (MotionGunLeftGetFeed(0) == 0) {
                  motion_warning_state = 30;
                  motion_gunl_feeder_powered = 0;
                }
                if (MotionGunLeftGetFeed(1) == 0) {
                  motion_warning_state = 35;
                  motion_gunl_feeder_powered = 0;
                }
                if (MotionGunRightGetFeed(0) == 0) {
                  motion_warning_state = 40;
                  motion_gunr_feeder_powered = 0;
                }
                if (MotionGunRightGetFeed(1) == 0) {
                  motion_warning_state = 45;
                  motion_gunr_feeder_powered = 0;
                }
                break;
            }
          }
        }

        //check if feeders can be powered
        //check if any of the switches is triggered
        if (motion_gunl_feeder_powered == 1) {
          if (MotionGunLeftGetFeed(0) == 1) motion_gunl_top_feeder_trigger = millis() + motion_gun_feeder_max_lock_time;
          if (MotionGunLeftGetFeed(1) == 1) motion_gunl_bottom_feeder_trigger = millis() + motion_gun_feeder_max_lock_time;
          if (motion_gun_fire_all == 1) { //only check when it should be firing
            if (millis() > motion_gunl_top_feeder_trigger) motion_gunl_top_feeder_locked = 1;
          }
          if (motion_gun_fire_all == 1) {
            if (millis() > motion_gunl_bottom_feeder_trigger) motion_gunl_bottom_feeder_locked = 1;
          }
        }
        if (motion_gunr_feeder_powered == 1) {
          if (MotionGunRightGetFeed(0) == 1) motion_gunr_top_feeder_trigger = millis() + motion_gun_feeder_max_lock_time;
          if (MotionGunRightGetFeed(1) == 1) motion_gunr_bottom_feeder_trigger = millis() + motion_gun_feeder_max_lock_time;
          if (motion_gun_fire_all == 1) {
            if (millis() > motion_gunr_top_feeder_trigger) motion_gunr_top_feeder_locked = 1;
          }
          if (motion_gun_fire_all == 1) {
            if (millis() > motion_gunr_bottom_feeder_trigger) motion_gunr_bottom_feeder_locked = 1;
          }
        }

        //check if any needs to be shut down
        if (motion_gunl_top_feeder_locked == 1) {
          motion_gunl_feeder_powered = 0;
          motion_warning_state = 10;
        }
        if (motion_gunl_bottom_feeder_locked == 1) {
          motion_gunl_feeder_powered = 0;
          motion_warning_state = 15;
        }
        if (motion_gunr_top_feeder_locked == 1) {
          motion_gunr_feeder_powered = 0;
          motion_warning_state = 20;
        }
        if (motion_gunr_bottom_feeder_locked == 1) {
          motion_gunr_feeder_powered = 0;
          motion_warning_state = 25;
        }
      }
      else {
        //remove master power
        MotionGunLeftMasterFeed(0);
        MotionGunRightMasterFeed(0);
      }
    }

    //eye light state
    if (millis() > motion_eye_led_update_target) {
      motion_eye_led_update_target = millis() + motion_eye_led_update_delay;
      if (millis() < motion_eye_led_time_target){ //while target time is not yet reached
        //recalculate brightness
        uint32_t temp_calc = map(millis(), motion_eye_led_time_started, motion_eye_led_time_target, motion_eye_led_history, motion_eye_led_target);
        motion_eye_led_value = temp_calc; //set brightness value
        analogWrite(eye_led, motion_eye_led_value); //set brightness
      }
      else { //if time is passed, force to target
        analogWrite(eye_led, motion_eye_led_target); //set brightness
        motion_eye_led_value = motion_eye_led_target;
      }
    }
  }
  else { //if there is any error
    Serial.print("Error: "); Serial.println(motion_error_state);
    MotionSetPanRaw(1, 0); //set pan motor off
    delay(100);
  }
  if (millis() > motion_warning_target) { //print warnings
    motion_warning_target = millis() + motion_warning_delay;
    if (motion_warning_state != 0) {
      Serial.print("Warning: "); Serial.println(motion_warning_state);
    }
  }
}

void MotionStart() {
  //check if the turret is already open
  uint16_t temp_open_position =  MotionGetOpenPositionRaw(); //get open position
  //Serial.print("open pos: "); Serial.println(temp_open_position);
  if (gun_open_limits[0] < gun_open_limits[1]) { //if 0 (closed) is lower than 1 (open), not inverted
    if (temp_open_position > gun_open_limits[1] - gun_open_error_bars) {
      motion_open_state = 1; //set state to open
    }
    else {
      motion_open_state = 0; //set state to closed (regardless of truth)
    }
  }
  else {
    if (temp_open_position < gun_open_limits[1] - gun_open_error_bars) {
      motion_open_state = 1;
    }
    else {
      motion_open_state = 0; //set state to closed (regardless of truth)
    }
  }

  //set position to current position -----

  //attach eye servos
  EyeMotorPower(1);
  EyeSetPosition(127, 127);

  //turn on eye

  //attach gun servo's
  MotionLeftTiltPower(1);
  MotionLeftTiltRaw(127);
  //set right tilt
  MotionRightTiltPower(1);
  MotionRightTiltRaw(127);

  //check gun feeder states
  ////if feeder error, do not power master feed (pass error)
  ////else power master feed

}

void MotionStop() {
  MotionSetPanRaw(1, 0); //set pan motor off
  MotionSetOpenRaw(1, 0);//set open motor off
  //turn of all eye power
  EyeMotorPower(0); //detach eye motors
  //detach all gun motors
  MotionLeftTiltPower(0);
  MotionRightTiltPower(0);
}

//set the position (pan and tilt) of the gun, where 127 is the center for all movements.
void  MotionSetGunPosition(uint16_t temp_pan, uint16_t temp_tilt) {
  motion_gun_pan_history = motion_gun_pan_target; //set history
  motion_gun_pan_target = temp_pan; //set position
  motion_gunl_tilt_history = motion_gunl_tilt_target; //set history
  motion_gunl_tilt_target = temp_tilt; //set position

  //set at target flags to 0 until a cycle has passed
  motion_gunl_tilt_at_target = 0;
  motion_gunr_tilt_at_target = 0;
  motion_gun_pan_at_target = 0;

  /*motion_gunr_tilt_history = motion_gunr_tilt_target; //set history
    motion_gunr_tilt_target = temp_tilt; //set position*/
  //MotionLeftTiltRaw(temp_tilt);
  //Serial.print("Setting gun position: "); Serial.println(motion_gun_pan_target);
}

uint8_t MotionGetArmed(){
  return digitalRead(gun_armed_switch);
}

void MotionFireAll(uint8_t temp_state) {
  motion_gun_fire_all = temp_state;
  motion_gun_fire_state = 0; //reset fire state
  motion_gun_fire_time = millis(); //set time command was given
}

void MotionSetFirePower(uint8_t temp_state){
  motion_gun_fire_power = map(temp_state, 0, 255, 0, 180);
}

void MotionFeederReset() { //resets all feeder values and ereases warning
  Serial.println("Feed Reset");
  motion_gun_feeder_first_start = 1;
  motion_gun_fire_all = 0;
  motion_gunl_top_feeder_locked = 0;
  motion_gunl_bottom_feeder_locked = 0;
  motion_gunr_top_feeder_locked = 0;
  motion_gunr_bottom_feeder_locked = 0;
  motion_warning_state = 0;
}


//sets the motors for the pan directly, without any protections
void MotionSetPanRaw(int8_t temp_direction, uint8_t temp_speed) {
  if (gun_pan_invert == 1) { //invert direction if needed
    if (temp_direction == 1) {
      temp_direction = -1;
    }
    else {
      temp_direction = 1;
    }
  }
  if (temp_direction == 1) {
    analogWrite(gun_pan_motion[0], 0);
    analogWrite(gun_pan_motion[1], temp_speed);
  }
  else {
    analogWrite(gun_pan_motion[1], 0);
    analogWrite(gun_pan_motion[0], temp_speed);
  }
  motion_gun_pan_power = temp_speed; //set power variable
}

//returns the current raw pan position
uint16_t MotionGetPanPositionRaw() {
  return analogRead(gun_pan_pos[0]);
}

uint16_t MotionGetPanCloseAlign(uint8_t temp_state) { //get close align value (high or >700ish is triggered)
  if (temp_state == 1) {
    return analogRead(gun_pan_close_align);
  }
  else {
    return digitalRead(gun_pan_close_align);
  }
}

//sets the motors for the open directly, without any protections
void MotionSetOpenRaw(int8_t temp_direction, uint8_t temp_speed) {
  //Serial.print("open raw: "); Serial.println(temp_speed);
  if (gun_open_invert == 1) { //invert direction if needed
    if (temp_direction == 1) {
      temp_direction = -1;
    }
    else {
      temp_direction = 1;
    }
  }
  if (temp_direction == 1) {
    analogWrite(gun_open_motion[0], 0);
    analogWrite(gun_open_motion[1], temp_speed);
  }
  else {
    analogWrite(gun_open_motion[1], 0);
    analogWrite(gun_open_motion[0], temp_speed);
  }
  motion_open_power = temp_speed; //set power
}

void MotionOpen(uint8_t temp_input) {
  if (temp_input == 1 && motion_open_state == 0) { //input is opening and current state is closed
    motion_open_state = 2;
    MotionSetOpenTarget(255); //already set the postion to open, else some of the pass conditions will meet prematurely
    motion_open_timeout_target = millis() + motion_open_timeout_time; //set open or close timeout target
    Serial.println("OPCM");
  }
  if (temp_input == 0 && motion_open_state == 1) { //input is closing and current state is opened
    motion_open_state = -1;
    motion_open_timeout_target = millis() + motion_open_timeout_time; //set open or close timeout target
    Serial.println("CLCM");
  }
}

void MotionSetOpenTarget(uint8_t temp_pos) {
  motion_open_target = temp_pos;
  motion_open_at_target = 0;
}

uint8_t MotionGetOpenAtTarget() {

}

uint8_t MotionGetPanAtTarget() {

}

//returns the current raw open position
uint16_t MotionGetOpenPositionRaw() {
  return analogRead(gun_open_pos[0]);
}

uint8_t MotionGetClosedSwitch() {
  return digitalRead(gun_closed_check_main);
}

void MotionFlywheelPower(uint8_t temp_state) {
  if (temp_state == 1) {
    gun_left_flywheel_servo.attach(gun_left_flywheel);
    gun_right_flywheel_servo.attach(gun_right_flywheel);
  }
  else {
    gun_left_flywheel_servo.detach();
    gun_right_flywheel_servo.detach();
  }
}

void MotionFlywheelSpeed(uint8_t temp_state) {
  gun_left_flywheel_servo.write(temp_state);
  gun_right_flywheel_servo.write(temp_state);
}

uint8_t MotionFeedersClear() { //checks if all feeders are at home position
  uint8_t temp_return = 1;
  if (MotionGunLeftGetFeed(0) == 0) temp_return = 0;
  if (MotionGunLeftGetFeed(1) == 0) temp_return = 0;
  if (MotionGunRightGetFeed(0) == 0) temp_return = 0;
  if (MotionGunRightGetFeed(1) == 0) temp_return = 0;
  return temp_return;
}

//left side gun

void MotionLeftTiltPower(uint8_t temp_state) {
  if (temp_state == 1) {
    gun_left_tilt_servo.attach(gun_left_tilt);
  }
  else {
    gun_left_tilt_servo.detach();
  }
}

uint16_t MotionGetLeftTiltCloseAlign(uint8_t temp_state) {
  if (temp_state == 1) {
    return analogRead(gun_left_close_align);
  }
  else {
    return digitalRead(gun_left_close_align);
  }
}

void MotionLeftTiltRaw(uint8_t temp_state) {
  //Serial.println(temp_state);
  uint16_t temp_target;
  if (gun_left_invert == 0) {
    temp_target = map(temp_state, 0, 255, gun_left_tilt_center - gun_motion_range, gun_left_tilt_center + gun_motion_range);
  }
  else {
    temp_target = map(temp_state, 0, 255, gun_left_tilt_center + gun_motion_range, gun_left_tilt_center - gun_motion_range);
  }
  gun_left_tilt_servo.write(temp_target);
}

void MotionGunLeftMasterFeed(uint8_t temp_state) {
  digitalWrite(gun_left_master_feed, temp_state);
}

void MotionGunLeftFeed(uint8_t temp_feeder, uint8_t temp_state) {
  digitalWrite(gun_left_feeder[temp_feeder], temp_state);
}

uint8_t MotionGunLeftGetFeed(uint8_t temp_feeder) {
  return digitalRead(gun_left_feed_check[temp_feeder]);
}

//right side gun

void MotionRightTiltPower(uint8_t temp_state) {
  if (temp_state == 1) {
    gun_right_tilt_servo.attach(gun_right_tilt);
  }
  else {
    gun_right_tilt_servo.detach();
  }
}

uint16_t MotionGetRightTiltCloseAlign(uint8_t temp_state) {
  if (temp_state == 1) {
    return analogRead(gun_right_close_align);
  }
  else {
    return digitalRead(gun_right_close_align);
  }
}

void MotionRightTiltRaw(uint8_t temp_state) {
  //Serial.println(temp_state);
  uint16_t temp_target;
  if (gun_right_invert == 0) {
    temp_target = map(temp_state, 0, 255, gun_right_tilt_center - gun_motion_range, gun_right_tilt_center + gun_motion_range);
  }
  else {
    temp_target = map(temp_state, 0, 255, gun_right_tilt_center + gun_motion_range, gun_right_tilt_center - gun_motion_range);
  }
  gun_right_tilt_servo.write(temp_target);
}

void MotionGunRightMasterFeed(uint8_t temp_state) {
  digitalWrite(gun_right_master_feed, temp_state);
}

void MotionGunRightFeed(uint8_t temp_feeder, uint8_t temp_state) {
  digitalWrite(gun_right_feeder[temp_feeder], temp_state);
}

uint8_t MotionGunRightGetFeed(uint8_t temp_feeder) {
  return digitalRead(gun_right_feed_check[temp_feeder]);
}

//eye

void EyeSetLaser(uint8_t temp_state) {
  temp_state = constrain(temp_state, 0, 1);
  digitalWrite(eye_laser, temp_state);
}

void EyeSetLed(uint8_t temp_state) {
  if (motion_eye_led_delay == 0){
    analogWrite(eye_led, temp_state);
    motion_eye_led_value = temp_state;
    motion_eye_led_target = temp_state;
  }
  else {
    motion_eye_led_history = motion_eye_led_value; //set history
    motion_eye_led_target = temp_state;
    motion_eye_led_time_started = millis();
    motion_eye_led_time_target = millis() + motion_eye_led_delay;
  }
}

void EyeSetLedDelay(uint16_t temp_state){
  motion_eye_led_delay = temp_state;
}

void EyeMotorPower(uint8_t temp_state) {
  if (temp_state == 1) {
    eye_pan_servo.attach(eye_pan);
    eye_tilt_servo.attach(eye_tilt);
  }
  else {
    eye_pan_servo.detach();
    eye_tilt_servo.detach();
  }
}

void EyeSetPosition(uint8_t temp_pan, uint8_t temp_tilt) {
  //remap eye values from 0-255 to actual limits
  if (eye_height_limiter == 1) { //limit the eye height if the limiter is on
    temp_tilt = constrain(temp_tilt, 0, 127);
    //Serial.print("constrained tilt: "); Serial.println(temp_tilt);
  }
  temp_pan = map(temp_pan, 0, 255, eye_pan_limit[0], eye_pan_limit[1]);
  temp_tilt = map(temp_tilt, 0, 255, eye_tilt_limit[0], eye_tilt_limit[1]);

  //calculate tilt offset

  //write positions
  //Serial.print(temp_pan); Serial.print(", "); Serial.println(temp_tilt);
  eye_pan_servo.write(temp_pan);
  eye_tilt_servo.write(temp_tilt);
}

void EyeLink(uint8_t temp_state) {
  if (temp_state == 1) {
    motion_eye_gun_link = 1;
  }
  else {
    motion_eye_gun_link = 0;
  }
}

void EyeLimit(uint8_t temp_state) {
  if (temp_state == 1) {
    eye_height_limiter = 1;
  }
  else {
    eye_height_limiter = 0;
  }
}

uint8_t MotionGetError() {
  return motion_error_state;
}

uint8_t motionGetWarning() {
  return motion_warning_state;
}

void motionOverrideSafety(uint8_t temp_state){
  temp_state = constrain(temp_state, 0, 1);
  motion_error_override = temp_state;
}
