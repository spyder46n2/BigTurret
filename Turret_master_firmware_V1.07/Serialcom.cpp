/*
   Serial class takes care of receiving and decoding the serial connection
   The serial port used is the FTDI usb to serial converter pressent on the Arduino Mega.
   The maximum adviced length for a full command is "max_read_length" including return characters.
   Any more and the buffers will overflow

   Each block consists of a semi fixed structure, the default is:
   CCCC-PPPPPPPP-PPPPPPPP-PPPPPPPPe

   The actual amount of P blocks can change depending on what is sent. The cap is 8

   - is a whitespace, to indicate an end of a line
   C = is a command. Each function has up to 4 letters command
   P = is an integer number, first number is #0, second #1 etc...
   e = end character ('\r' of '\n')

  The list of commands is as follows:
  -#com: set command echo. #0 1 for on, #0 0 for off
  -#num: set number echo. #0 1 for on, #0 0 for off

  high level commands
  -strt: starts the turret up
  -stop: stops the turret in current position

  power commands
  -alpw: set all power. #0 1 is on, #0 0 is off
  -eypw: set eye power. #0 1 is on, #0 0 is off
  -oppw: set open power. #0 1 is on, #0 0 is off
  -lgpw: set left gun power. #0 1 is on, #0 0 is off
  -rgpw: set right gun power. #0 1 is on, #0 0 is off

  gun motion command
  -open: open command. #0 0 for closed, #0 1 for open
  -gpos: gun position: #0 0 to 255 for pan, #1 0 to 255 for tilt (no gun motion when closed (if linked eye will move)
  -gena: gun enable: #0 0 for disabled, #0 1 for enabled

  eye command
  -epos: eye position: #0 0 to 255 for pan, #1 0 to 255 for tilt
  -eena: eye enable: #0 0 for disabled, #0 1 for enabled
  -elnk: eye link to motion: #0 1 for linked, #0 0 for unlinked (auto unlink on epos command)
  -elig: eye light: #0 0 for off, #0 255 for full (scaled 0-255)
  -elas: eye laser: #0 0 for off, #0 1 for on

  gun fire commands
  -fire: guns fire: #0 0 or 1 for left cease fire or fire, #1 0 or 1 for right cease fire or fire

  panic commands (override all safeties and checks)
  -!pan: turns the relay and the pan motor on for one second. #0 1 for positive, #0 -1 for negative
  -!ope: turn the open motor and relay on for one seconds. #0 1 for positive, #0 -1 for negative

  Feedback commands
  -?all: Give all measured values
  -?pan: Give pan position

*/

#include "Arduino.h"

#define max_read_length 130
#define max_numbers 8

class SerialCommand {
    //class member variables
    int8_t error_flag = 0; //what error if any is happening

    //read variables
    char read_buffer[64]; //buffer for storing serial characters
    char decode_buffer[max_read_length]; //buffer for storing serial line
    uint16_t decode_buffer_pos = 0; //how much is in the read buffer
    uint16_t serial_available; //number of available bytes in hardware buffer

    int16_t end_character_pos; //where the end character is

    //write variables
    char write_buffer[64]; //buffer for handling serial output
    uint8_t write_characters; //how many characters are in the buffer to be sent out

    //latest decoded line
    uint16_t serial_line_number;
    uint32_t serial_command;
    int32_t serial_small_value; //to be removed in code
    int32_t serial_number[max_numbers];
    uint8_t serial_raw[50]; //to be removed in code

  public:
    SerialCommand() {

    }

    void Begin() { //sends start message to indicate live connection
      Serial.begin(115200); //Speed is actually irrelephant for this serial
      write_characters = 0;
      Serial.println("Big turret V1.07"); //Send version number over serial
    }

    int16_t Update() { //updates all serial command functions
      /*
         Read what is currently in the buffer
         if a full line is in buffer, read and decode it
         If the program needs to act on a new line, update will return a 1, else a 0
         returns a -negative on error
      */
      serial_available = Serial.available(); //get number available
      if (serial_available) { //if there are lines in hardware buffer
        //Serial.print("Available: "); Serial.println(serial_available);
        serial_available = constrain(serial_available, 0, 64); //constrain lines available to hardware block size
        Serial.readBytes(read_buffer, serial_available); //read bytes from hardware buffer


        //calculate decode buffer space left
        int16_t temp_space_left = max_read_length;
        temp_space_left -= decode_buffer_pos;
        temp_space_left -= serial_available;
        //Serial.print("Spaceleft: "); Serial.println(temp_space_left);

        uint16_t temp_read = 0;
        if (temp_space_left > 0) { //add read_buffer to serial_buffer if possible
          for (uint16_t b = decode_buffer_pos; b < decode_buffer_pos + serial_available; b++) { //add to read buffer
            decode_buffer[b] = read_buffer[temp_read];
            // Serial.print("Adding: '"); Serial.print(decode_buffer[b]); Serial.println("' to buffer");
            temp_read ++; //add one to readbuffer position
          }
          decode_buffer_pos += serial_available;
          //Serial.print("Buffer: '"); for (uint16_t r = 0; r < max_read_length; r++) {
          //  Serial.print(decode_buffer[r]);
          //} Serial.println("'");

          //Serial.print("Decode buffer pos: "); Serial.println(decode_buffer_pos);

          end_character_pos = -1; //reset end character pos
          for (uint16_t b = 0; b < max_read_length; b++) { //loop through buffer to find end character (start at front)
            if (IsEndCharacter(decode_buffer[b]) == 2) { //if end of line character
              end_character_pos = b; //set end character search
              break; //break from search loop
            }
          }

          int16_t temp_start = 0, temp_end = 0; //keeps track of the blocks to decode
          uint8_t temp_character = 0; //keeps track of which character is being read
          uint8_t temp_read_error = 0, temp_ended = 0;
          uint32_t temp_decode;
          if (end_character_pos != -1) { //if end character found, start decoding from front
            //Serial.print("End of line character found: "); Serial.println(end_character_pos);

            //reset everything to 0 when reading starts
            serial_command = 0;
            for (uint8_t n = 0; n < max_numbers; n++) {
              serial_number[n] = 0;
            }
            //serial_small_value = 0;
            //for (uint8_t r = 0; r < 50; r++){
            //  serial_raw[r] = 0;
            //}

            //look for first break, command
            if (temp_read_error == 0 && temp_ended == 0) { //check if no error and not ended
              //Serial.println("Start looking for command");
              for (uint16_t e = temp_start; e <= end_character_pos; e++) { //look for second break, command
                if (IsEndCharacter(decode_buffer[e])) {
                  //Serial.println("break character found");
                  if (IsEndCharacter(decode_buffer[e]) == 2) {
                    temp_ended = 1;  //set ended to 1 if this was a line end, not a space
                    //Serial.println("Ended");
                  }
                  //Serial.print("End char in CM: "); Serial.println(e);
                  temp_end = e; //note the position where the line ends
                  for (int16_t d = temp_end - 1; d >= temp_start; d--) { //decode from end (LSB) to start (MSB)
                    temp_decode = decode_buffer[d];//find command (raw ascii to value)
                    temp_decode = temp_decode << 8 * temp_character; //store temp command
                    temp_character++;
                    serial_command |= temp_decode;
                  }
                  //Serial.print("Decoded CM: "); Serial.println(serial_command);
                  temp_start = e + 1; //set new start
                  temp_character = 0;
                  break; //break from current for loop
                }
              }
            }

            //look for second to ninth break, small value
            for (uint8_t n = 0; n < max_numbers; n++) { //look through all numbers
              int8_t temp_signed = 1; //set signed value (positive by default)
              if (temp_read_error == 0 && temp_ended == 0) { //check if no error and not ended
                //Serial.println("Start looking for number");
                for (uint16_t e = temp_start; e <= end_character_pos; e++) { //look for third break, small
                  if (IsEndCharacter(decode_buffer[e])) {
                    if (IsEndCharacter(decode_buffer[e]) == 2) {
                      temp_ended = 1;  //set ended to 1 if this was a line end, not a space
                      //Serial.println("Ended");
                    }
                    //Serial.print("End char in SV: "); Serial.println(e);
                    //look for first character being a sign
                    //Serial.println("Looking for sign: ");
                    if (decode_buffer[temp_start] == '-') { //if first character is sign
                      //Serial.println("Sign found");
                      temp_start ++; //ignore first character
                      temp_signed = -1; //set sign to -1
                    }
                    temp_end = e; //note the position where the line ends
                    for (int16_t d = temp_end - 1; d >= temp_start; d--) { //decode from end (LSB) to start (MSB)
                      if (IsB10(decode_buffer[d])) { //if B10 value
                        temp_decode = B10Lookup(decode_buffer[d]); //decode lead number
                        //Serial.print("Decode value to: "); Serial.println(temp_decode);
                        for (uint8_t p = 0; p < temp_character; p++) { //add powers until the number is correct
                          temp_decode *= 10;
                          //Serial.println("adding power of 10");
                        }
                        temp_character++;
                        serial_number[n] += temp_decode; //add value to lead number
                      }
                      else { //if any non B10 char found, go to error
                        temp_read_error = 1;
                        //Serial.print("Error");
                      }
                    }
                    serial_number[n] = serial_number[n] * temp_signed; //add sign to value
                    //Serial.print("Decoded num: "); Serial.println(serial_number[n]);
                    temp_start = e + 1; //set new start
                    temp_character = 0;
                    break; //break from current for loop
                  }
                }
              }
              if (temp_ended == 1) { //break from 8 number search loop if line has ended
                break;
              }
              //if by this point no end character is found, something is very wrong! it should though.
            }

            
            if (temp_read_error == 0 && temp_ended == 1) { //if ended and no mistakes
              //remove read bytes from decode buffer
              //Serial.println("Removing read lines");
              int16_t temp_reset = end_character_pos;//serial_available;
              for (uint16_t r = 0; r < max_read_length; r++) {
                if (decode_buffer_pos > 0) {
                  decode_buffer[r] = decode_buffer[temp_reset];
                  decode_buffer_pos--; //remove one pos from decode buffer
                  temp_reset++; //remove one to read value
                }
                else {
                  decode_buffer[r] = 0;
                }
              }
              
              //send received response
              Serial.println("OK"); //print ok
              return 1;  //return a 1
            }
            else {
              return 0; //mistake somewhere along the way
            }
          } //end of end character found if
        } //end of spaceleft > 0
        else { //if there was no space left in buffer
          return -1;
        }
      } //end of serial available
      return 0; //no new data, return a 0
    }
    uint16_t GetBufferLeft() { //gets how many characters are left in the buffer
      uint16_t temp_return = max_read_length - 1;
      temp_return -= decode_buffer_pos;
      return temp_return;
    }
    uint16_t GetLineNumber() { //returns the last read line number
      return serial_line_number;
    }
    uint32_t GetCommand() { //returns the last read command
      return serial_command;
    }
    int32_t GetNumber(uint8_t temp_input) { //returns the last read small value
      temp_input = constrain(temp_input, 0, 7);
      return serial_number[temp_input];
    }
    void GetRaw(uint8_t* temp_raw) { //takes an uint8_t[50] array and fills it with the last read value
      for (uint8_t r = 0; r < 50; r++) {
        temp_raw[r] = serial_raw[r];
      }
    }
    uint8_t IsEndCharacter(char temp_input) { //checks if a char is an end character, 1 for block end, 2 for line end
      if (temp_input == 10) return 2; //new line
      if (temp_input == 13) return 2; //carriage return
      if (temp_input == 32) return 1; //space
      return 0;
    }
    uint8_t IsB10(char temp_input) {  //reads a character and determines if it is a valid base 10 character
      if (temp_input >= '0' && temp_input <= '9') return 1;
      if (temp_input == '-') return 1; //negative is accepted in base 10 mode
      return 0; //no match found
    }
    uint8_t IsB64(char temp_input) { //reads a character and determines if it is a valid base 64 character
      if (temp_input >= 'A' && temp_input <= 'Z') return 1;
      if (temp_input >= 'a' && temp_input <= 'z') return 1;
      if (temp_input >= '0' && temp_input <= '9') return 1;
      if (temp_input == '+') return 1;
      if (temp_input == '/') return 1;
      return 0; //no match found
    }
    int8_t B10Lookup(char c) { //returns the 0-9 from ascii
      if (c >= '0' && c <= '9') return c - 48; //0-9
      return -1; //-1
    }
    int8_t B64Lookup(char c) { //Convert a base 64 character to decimal, (returns -1 if no valid character is found)
      if (c >= 'A' && c <= 'Z') return c - 'A'; //0-25
      if (c >= 'a' && c <= 'z') return c - 71; //26-51
      if (c >= '0' && c <= '9') return c + 4; //52-61
      if (c == '+') return 62; //62
      if (c == '/') return 63; //63
      return -1; //-1
    }
    char ToB64Lookup(uint8_t temp_input) {
      if (temp_input >= 0 && temp_input <= 25) return temp_input + 'A'; //0-25
      if (temp_input >= 26 && temp_input <= 51) return temp_input + 71; //26-51
      if (temp_input >= 52 && temp_input <= 61) return temp_input - 4; //52-61
      if (temp_input == 62) return '+';
      if (temp_input == 63) return '/';
      return '&'; //return '&' if false, this will trigger code reading it to mark as mistake
    }
    void WriteValueToB64(int32_t temp_input) { //adds a value to the writebuffer in B64
      //Serial.println("Encoding to B64");
      //Serial.print("Number: "); Serial.println(temp_input);
      uint8_t temp_sign = 0;
      uint8_t temp_6bit;
      char temp_reverse_array[5];
      if (temp_input < 0) { //if value is negative
        temp_sign = 1; //set negative flag to 1
        temp_input = abs(temp_input); //make the input positive
      }
      for (int8_t a = 0; a < 5; a++) { //loop through the input to convert it
        //Serial.print("Encoding: "); Serial.println(a);
        temp_6bit = 0; //reset 6 bit value
        temp_6bit = temp_input & 63; //read first 6 bits
        //Serial.print("Turns to: "); Serial.println(temp_6bit);
        temp_reverse_array[a] = ToB64Lookup(temp_6bit);
        temp_input = temp_input >> 6; //shift bits
      }
      //add reverse array to output buffer
      if (temp_sign == 1) { //add negative to value
        write_buffer[write_characters] = '-';
        write_characters++;
      }
      uint8_t temp_started = 0;
      for (int8_t a = 4; a >= 0; a--) {
        if (temp_started == 0) { //until a value is seen all zero's ('A''s) are ignored
          if (temp_reverse_array[a] != 'A') temp_started = 1; //start when first non 0 is found
          if (a == 0) temp_started = 1; //also start when the first character is reached
        }
        if (temp_started == 1) { //when started, write to output buffer
          write_buffer[write_characters] = temp_reverse_array[a];
          write_characters++;
        }
      }
    }
    void WriteTestArrayToB64(uint8_t temp_input[]) { //adds a test array to the writebuffer
      uint16_t temp_n = 0; //nozzle counter
      uint8_t temp_6bit; //6 bit decode value
      char temp_output[50]; //50 char output array
      for (uint8_t r = 0; r < 50; r++) {
        temp_output[r] = ' '; //reset array values
      }
      //decode 300 array to B64
      for (uint8_t B = 0; B < 50; B++) {
        temp_6bit = 0; //reset 6 bit value
        for (uint8_t b = 0; b < 6; b++) {
          //Serial.print("Decoding: "); Serial.print(temp_n); Serial.print(", Value: "); Serial.println(temp_input[temp_n]);
          temp_6bit |= (temp_input[temp_n] & 1) << b;
          temp_n++;
        }
        temp_output[B] = ToB64Lookup(temp_6bit); //write 6 bit value to output
        //Serial.print("Encoding: "); Serial.print(temp_6bit); Serial.print(", Value: "); Serial.println(temp_output[B]);
      }

      //write B64 to output buffer
      for (int8_t B = 49; B >= 0; B--) {
        write_buffer[write_characters] = temp_output[B];
        write_characters++;
      }
    }
    /*void SendResponse() { //will write the line in the buffer and mark it to be sent ASAP
      write_buffer[write_characters] = '\n'; //add carriage return
      write_characters++;
      Serial.write(write_buffer, write_characters);
      //Serial.send_now(); //send all in the buffer ASAP //send not not supported on a Mega ---------------------------------
      write_characters = 0;//reset write counter
      }
      void RespondTestPrinthead() {

      }
      void RespondTemperature(int32_t temp_temp) {
      write_characters = 4; //set characters to value after adding response header
      write_buffer[0] = 'G';
      write_buffer[1] = 'T';
      write_buffer[2] = 'P';
      write_buffer[3] = ':';
      WriteValueToB64(temp_temp); //convert temperature to 64 bit
      SendResponse(); //send temperature
      }
      void RespondEncoderPos(int32_t temp_pos) {
      write_characters = 4; //set characters to value after adding response header
      write_buffer[0] = 'G';
      write_buffer[1] = 'E';
      write_buffer[2] = 'P';
      write_buffer[3] = ':';
      WriteValueToB64(temp_pos); //convert temperature to 64 bit
      SendResponse(); //send temperature
      }
      void RespondBufferReadLeft(int32_t temp_left) {
      write_characters = 4; //set characters to value after adding response header
      write_buffer[0] = 'B';
      write_buffer[1] = 'R';
      write_buffer[2] = 'L';
      write_buffer[3] = ':';
      WriteValueToB64(temp_left); //convert temperature to 64 bit
      SendResponse(); //send left
      }
      void RespondBufferWriteLeft(int32_t temp_left) {
      write_characters = 4; //set characters to value after adding response header
      write_buffer[0] = 'B';
      write_buffer[1] = 'W';
      write_buffer[2] = 'L';
      write_buffer[3] = ':';
      WriteValueToB64(temp_left); //convert temperature to 64 bit
      SendResponse(); //send left
      }
      void RespondTestResults(uint8_t temp_mode, uint8_t temp_result[]) {
      write_characters = 4; //set characters to value after adding response header
      write_buffer[0] = 'T';
      write_buffer[1] = 'H';
      write_buffer[2] = 'D';
      write_buffer[3] = ':';
      WriteTestArrayToB64(temp_result); //convert 1B array to 64 bit
      SendResponse(); //send left
      }*/
};
