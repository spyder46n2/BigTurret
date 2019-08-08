from SupportFunctions import *
from TurretSound import TurretSound
from TurretSerial import Turret
from TurretSettings import TurretSettings
import time

turret = Turret()
turret_settings = TurretSettings()
turret_sound = TurretSound()

if (turret_settings.autostart == 1):
    print("Connecting to turret")
    turret.Connect(turret_settings.com_port)
    turret.SetEyeLimit(turret_settings.eye_limiter)
    turret.SetGunPower(turret_settings.gun_power)
    turret.Start()
    turret_started = 1
else:
    com_port = input("Give com port: ")
    turret.Connect(com_port)
    
while (turret.started_state == 0):
        time.sleep(0.5)
        print("Still connecting")

    
while True:
    filenumber = input("What program to play: ")
    if (filenumber == 'q'):
        break
    print("Playing " + str(filenumber))

    playfile = open("play files/" + str(filenumber) + ".txt", "r")
    #print (playfile.read())

    while True:
        temp_line = playfile.readline()
        if (temp_line == ""): #check for end of line conditions
            print("end of file")
            break
        #print (temp_line.rstrip())
        
        #check for alternative commands
        if (temp_line.startswith('delay')):
            temp_delay = temp_line.partition(' ') #split at :
            temp_delay = int(temp_delay[2])
            #print(temp_delay)
            print("Waiting " + str(temp_delay) + "ms")
            temp_delay = int(temp_delay/100)
            for t in range(temp_delay):
                time.sleep(0.1)
                #print("waiting")
        elif (temp_line.startswith('sound1')):
            temp_sound = temp_line.partition(' ') #split at :
            temp_sound = int(temp_sound[2])
            turret_sound.PlayFound(temp_sound)
        else:
            turret.SerialWriteBufferRaw(temp_line.rstrip()) #print actual line
            print(temp_line.rstrip())
        
