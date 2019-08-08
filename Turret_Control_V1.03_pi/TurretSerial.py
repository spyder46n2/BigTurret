
import serial
import threading
import time
from TurretSettings import TurretSettings

class Turret(serial.Serial):
    def __init__(self):
        self.ser = serial.Serial() #make an instance of serial connection
        self.ser.baudrate = 115200 #set baudrate
        self.ser.timeout = 0 #set timeout to 0, non blocking mode
        
        #status flags
        self.connection_state = 0 #whether connected or not
        self.started_state = 0 
        self.ok_state = 1 #whether the response of serial has been OK or not
        self.error_state = 0 #error of system
        self.inkjet_version = 0.0 #version of turret firmware
        
        self.send_get_status = 0 #whether to fetch a value
        self.send_status_buffer = "" #buffer for status
        self.status_state = 0 #which status to fetch
        
        self.code_buffer = []
        self.code_buffer_left = 0
        
        self.error_array = []
        self.error_array.append(0)
        self.warning_array = []
        self.warning_array.append(0)
        
        self.window_output_buffer = "" #holds a buffer of what was sent out
        self.window_input_buffer = "" #holds a buffer of what was received
        
        self.turret_settings = TurretSettings()
        
    def Connect(self, serial_port):
        """Attempt to connect to the Turret"""
        self.com_port_raw = str(serial_port) #get value from set_com
        self.ser.port = self.com_port_raw #set com port
        
        if (self.connection_state == 0): #if not yet connected
            #print("attempting to open: " + self.com_port_raw)
            self.temp_com_success = 0
            try: #try to open a com port with it
                self.ser.open()
                self.temp_com_success = 1
            except:
                print ("Unable to open connection")
                nothing = 0
            if (self.temp_com_success == 1):
                print(self.com_port_raw + " for Turret opened")
                self.connection_state = 1
                self.started_state = 0 
                self.ok_state = 0 
                self.error_state = 0 
                self.homed_state = 0 
                self._stop_event = threading.Event()
                self.update_thread = threading.Thread(target=self.Update)
                self.update_thread.start()
                #self.status_thread = threading.Thread(target=self.GetStatus)
                #self.status_thread.start()
                return 1
            else:
                return 0
                
    def Disconnect(self):
        """close the connection to Turret"""
        if (self.connection_state == 1):
            self._stop_event.set()
            self.ser.close()
            print("Closing Turret connection")
            self.connection_state = 0
            return 0
            
    def Update(self):
        """Preforms all continous tasks required for Turret connection"""
        time.sleep(0.05)
        read_buffer = "" #used to store Serial read data
        while not self._stop_event.is_set():
            #print(self.ok_state)
            #get code
            temp_success = 0 #success value
            try: #attempt to read serial
                if (self.ser.in_waiting > 0):
                    temp_read = self.ser.read(self.ser.in_waiting) #add serial to read buffer
                    temp_read = str(temp_read.decode('utf-8'))
                    temp_success = 1
            except:
                print("Read error") #some mistake, otherwise ignore quietly
                break
            if temp_success == 1: 
                read_buffer += temp_read
                #print(read_buffer)
                #add line to read buffer
                
            temp_decode = read_buffer.partition('\n') #check for EOL conditions
            if (temp_decode[1] == '\n'): #if '\n' 
                read_buffer = temp_decode[2] #write remainder to buffer
                read_line = str(temp_decode[0])
                #read_line = read_line.lower() #make all lower case for checking #(DONT!!!)
                read_line = read_line.rstrip() #remove carriage return
                #print("reading line: " + str(read_line)) 
                #check purpose of response
                if (read_line.startswith('OK')): #if ok was found,
                    self.ok_state = 1 #set ok state to 1
                    #print("OK found, setting ok state")
                elif (read_line.startswith('Big turret V')): #if version was found was found,
                    self.ok_state = 1 #set ok state to 1
                    read_line = read_line.partition('V') #split at :
                    read_line = read_line[2]
                    self.inkjet_version = float(read_line)
                    self.started_state = 1
                    print("Version found: V" + str(self.inkjet_version))   
                elif (read_line.startswith('Warning: ')):
                    #print("warning found")
                    read_line = read_line.partition(' ') #split at ' '
                    read_line = read_line[2] #get end
                    temp_int = int(read_line)
                    #print(temp_int)
                    self.warning_array.append(temp_int)
                    #print(self.turret_settings.warning_table[temp_int])
                    #print (self.warning_array)
                elif (read_line.startswith('Error: ')):
                    #print("error found")
                    read_line = read_line.partition(' ') #split at ' '
                    read_line = read_line[2] #get end
                    temp_int = int(read_line)
                    #print(temp_int)
                    self.error_array.append(temp_int)
                    #print (self.error_array)
                else:
                    print("line not recognized: " + str(read_line))
                    
                
        
            #is ok state is 1, and line buffered, send new line
            if (self.ok_state == 1):
                if (self.send_get_status == 1): 
                    pass
                    #print("sending status")
                    #self.ok_state = 0 #set ok state to 0
                    #self.SerialWriteRaw(self.send_status_buffer + "\r",0) #send status request
                    #self.send_get_status = 0 #set get status to 0
                    #print("Getting status")
                elif (self.BufferLeft() > 0): #if there are lines left to print
                    #print(self.BufferLeft())
                    self.ok_state = 0 #set ok state to 0
                    self.BufferNext() #print next line in buffer to serial            
                        
    
    def SerialWriteRaw(self, input_string, temp_priority):
        """prints a line to the HP45 (no checks)
        priority is 0 for not send to output, and 1 for sent to output"""
        if (temp_priority == 1):
            self.window_output_buffer += input_string #add to the window buffer
        #print(input_string.encode('utf-8'))
        self.ser.write(input_string.encode('utf-8'))
        
    def SerialWriteBufferRaw(self, input_string):
        """Adds a line to the input buffer""" 
        if (self.connection_state == 1): #only work when connected
            self.code_buffer.append(str(input_string) + '\r') #add string to buffer
            self.code_buffer_left += 1 #add one to left value
            #print(self.BufferLeft())
    
    def BufferLeft(self):
        """returns how many lines are left in the buffer"""
        return self.code_buffer_left
    
    def BufferNext(self):
        """Writes the next line in the buffer to the serial"""
        if (self.BufferLeft() > 0): #if there are lines left in the buffer
            self.code_buffer_left -= 1 #subtract 1 from left value
            self.SerialWriteRaw(self.code_buffer[0],0) #print to HP45
            del self.code_buffer[0] #remove the written line
            #print("Buffer next")
    
    def GetStatus(self):
        """periodically sends a get status command"""
        time.sleep(5) #initial wait to get system time to start
        while not self._stop_event.is_set():
            time.sleep(0.1) #wait for 0.2 seconds
            if (self.status_state == 0): #get temp
                self.send_status_buffer = "GTP" #get temperature
                #print("Ask for temp from HP45")
            if (self.status_state == 1): #get pos
                self.send_status_buffer = "GEP" #Get encoder position
                #print("Ask for pos from HP45")
            if (self.status_state == 2): #get write left
                self.send_status_buffer = "BWL" #Get write left
                #print("Ask for WL from HP45")
            self.send_get_status = 1
            self.status_state += 1
            if (self.status_state > 2): #reset state
                self.status_state = 0
    
    def GetWarning(self):
        """Returns the last known warning"""
        return self.warning_array[-1]
        
    def GetError(self):
        """returns the last known error"""
        return self.error_array[-1]
        
    def GetWindowOutput(self):
        """returns the entire string of what was sent since the 
        last call of this function, then clears that buffer"""
        temp_return = self.window_output_buffer #write to return value
        self.window_output_buffer = "" #clear buffer
        return temp_return #return response
        
    def GetWindowInput(self):
        """returns the entire string of what was received since the 
        last call of this function, then clears that buffer"""
        temp_return = self.window_input_buffer #write to return value
        self.window_input_buffer = "" #clear buffer
        return temp_return #return response
        
    def Start(self):
        self.SerialWriteBufferRaw("strt")
        
    def Stop(self):
        self.SerialWriteBufferRaw("stop")
    
    def SetGunPosition(self, temp_position_x, temp_position_y):
        """Takes the input x any y positions as 0-255 intger and print to turret guns"""
        if (self.connection_state == 1): #check if connected before sending 
            self.SerialWriteBufferRaw("gpos " + str(int(temp_position_x)) + " " + str(int(temp_position_y))) #send position

    def SetEyePosition(self, temp_position_x, temp_position_y):
        """Takes the input x any y positions as 0-255 intger and print to turret eye"""
        if (self.connection_state == 1): #check if connected before sending
            self.SerialWriteBufferRaw("epos " + str(int(temp_position_x)) + " " + str(int(temp_position_y))) #send position
    
    def Open(self):
        """opens the turret"""
        self.SerialWriteBufferRaw("open 1") #send brightness
        
    def Close(self):
        """closes the turret"""
        self.SerialWriteBufferRaw("open 0") #send brightness
            
    def SetEyeLight(self, temp_brightness):
        """Takes the brightness and write it to the eye"""
        if (self.connection_state == 1): #check if connected before sending
            self.SerialWriteBufferRaw("elig " + str(int(temp_brightness))) #send brightness
            
    def SetEyeDelay(self, temp_delay):
        """Takes the brightness and write it to the eye"""
        if (self.connection_state == 1): #check if connected before sending
            self.SerialWriteBufferRaw("elde " + str(int(temp_delay))) #send delay
            
    def SetEyeLaser(self, temp_brightness):
        """Takes the brightness and write it to the eye"""
        if (self.connection_state == 1): #check if connected before sending
            self.SerialWriteBufferRaw("elas " + str(int(temp_brightness))) #send brightness
    
    def SetEyeLimit(self, temp_state):
        """sets the eye laser limit (height limiter)"""
        if (self.connection_state == 1): #check if connected before sending
            self.SerialWriteBufferRaw("elim " + str(int(temp_state))) #send limit
    
    def SetGunPower(self, temp_power):
        """Takes the power and write it to the guns"""
        if (self.connection_state == 1): #check if connected before sending
            self.SerialWriteBufferRaw("fpwr " + str(int(temp_power))) #send power
            
    def SetFire(self, temp_state):
        """Takes the power and write it to the guns"""
        if (self.connection_state == 1): #check if connected before sending
            self.SerialWriteBufferRaw("fire " + str(int(temp_state))) #send state
            
    def SetSafetyOverride(self, temp_state):
        if (self.connection_state == 1): #check if connected before sending
            self.SerialWriteBufferRaw("!sfo " + str(int(temp_state))) #send override

    
