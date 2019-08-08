import sys, pygame
from SupportFunctions import *
from TurretSound import TurretSound
from TurretSerial import Turret
from TurretSettings import TurretSettings
import time
import platform

#check if a camera can be enabled
camera_enabled = 1
print(platform.system())
if (platform.system() == 'Windows'):
    camera_enabled = 0
    print("Pygame cam incompatible with windows, disabling camera functions")
    
pygame.init()

if (camera_enabled == 1):
    import pygame.camera
    pygame.camera.init()

turret_sound = TurretSound()

size = width, height = 640, 640
click_size = click_width, click_height = 640, 480
click_size_border = 100 #how much fluff there is around the borders
screen = pygame.display.set_mode(size)
pygame.display.set_caption('Turret Controller')
game_icon = pygame.image.load("Turret controll icon.bmp")
pygame.display.set_icon(game_icon)

if (camera_enabled == 1):
    print(pygame.camera.list_cameras())
    cam_list = pygame.camera.list_cameras()
    cam = pygame.camera.Camera(cam_list[0],(640,480))
    cam.start()

update_limiter = 0
error_limiter = 0

turret = Turret()
turret_settings = TurretSettings()

turret_started = 0
turret_target_state = 0
turret_target_spoted = 0
turret_target_lost = 0
turret_open = 0
turret_firing = 0

key_lock = 0

manual_open_state = 0 #open or closed
manual_pan_position = 127 #pan position
manual_tilt_position = 127 #tilt position
manual_fire_state = 0 #firing or not
manual_fire_power = 100 #0-255 gun power
manual_laser_state = 0 #laser on or off
manual_laser_limiter = 0 #laser higher than 127
manual_eye_power = 0 #how bright the eye is
manual_tracking_state = 0 #tracking objects or manual mode
manual_calibrate_color = 0 
manual_sound_state = 1 #sound on or off
manual_safety_override = 0 #safeties!
manual_eye_delay = 0 #how long a state change takes
manual_view_state = 0 #whether to show raw or converted image
manual_position_update = 0

sender_state = 0 #0 is inactive, 1 is starting, 2 is started
sender_file = 0
sender_time_target = 0 #when to do the next line


#turret start requirements
if (turret_settings.autostart == 1):
    print("Connecting to turret")
    turret.Connect(turret_settings.com_port)
    turret.SetEyeLimit(turret_settings.eye_limiter)
    turret.SetGunPower(turret_settings.gun_power)
    turret.SetSafetyOverride(turret_settings.safety_override)
    turret.Start()
    turret_started = 1
    
    while (turret.started_state == 0):
        if (pygame.time.get_ticks() % 500 == 0 and update_limiter == 0):
            update_limiter = 1
            print("Still connecting")
        if (pygame.time.get_ticks() % 500 == 1):
            update_limiter = 0
            
    #turret.SetEyeLight(255)
            

    
def ButtonTest():
    keys=pygame.key.get_pressed()
    if keys[pygame.K_LEFT]:
        print("left")
        turret.SetEyeLight(0)
    if keys[pygame.K_RIGHT]:
        print("right")
        turret.SetEyeLight(255)
    if keys[pygame.K_UP]:
        print("up")
        turret.Connect("com9")
    if keys[pygame.K_DOWN]:
        print("down")
        turret.Disconnect()
    if keys[pygame.K_SPACE]:
        pass
            
    if (pygame.mouse.get_pressed()[0]):
        click_position = pygame.mouse.get_pos()
        click_width_pos = int(RemapValue(click_position[0], 0, width, 0,256))
        click_height_pos = int(RemapValue(click_position[1], height, 0, -1,255))
        print(str(click_width_pos) + "," + str(click_height_pos))
        turret.SetGunPosition(click_width_pos, click_height_pos)

#prints all values on screen
def PrintStates():
    temp_red = (255,0,0)
    temp_black = (0,0,0)
    font = pygame.font.Font('freesansbold.ttf', 12) 
    
    # create a text suface object, 
    # on which text is drawn on it. 
    open_text = font.render('[o][c] Open state: ' + str(manual_open_state) + "       ", True, temp_red, temp_black) 
    opentextrect = open_text.get_rect() 
     
    pos_text = font.render('[arrows] position: ' + str(manual_pan_position) + ", " + str(manual_tilt_position) + "       ", True, temp_red, temp_black)
    postextrect = pos_text.get_rect()  
    
    fire_text = font.render('[space] Fire state: ' + str(manual_fire_state) + "       ", True, temp_red, temp_black) 
    firetextrect = fire_text.get_rect() 
    
    firepwr_text = font.render('[,][.] Fire power: ' + str(manual_fire_power) + "       ", True, temp_red, temp_black) 
    firepwrtextrect = firepwr_text.get_rect() 
    
    eyepwr_text = font.render('[e] Eye power: ' + str(manual_eye_power) + "       ", True, temp_red, temp_black) 
    eyepwrtextrect = eyepwr_text.get_rect()
    
    eyedel_text = font.render('[d] Eye delay: ' + str(manual_eye_delay) + "       ", True, temp_red, temp_black) 
    eyedeltextrect = eyedel_text.get_rect()
    
    laser_text = font.render('[l] Laser: ' + str(manual_laser_state) + "       ", True, temp_red, temp_black) 
    lasertextrect = laser_text.get_rect()  
    
    laserlim_text = font.render('[i] Laser limit: ' + str(manual_laser_limiter) + "       ", True, temp_red, temp_black) 
    lasertextrectlim = laserlim_text.get_rect()
     
    sound_text = font.render('[s] Sound: ' + str(manual_sound_state) + "       ", True, temp_red, temp_black) 
    soundtextrectlim = sound_text.get_rect() 
    
    safety_text = font.render('[del] SAFETY OVERRIDE: ' + str(manual_safety_override) + "       ", True, temp_red, temp_black) 
    safetytextrectlim = safety_text.get_rect() 
     
      
    # create a rectangular object for the 
    # text surface object 
    
      
    # set the center of the rectangular object. 
    opentextrect.topleft = (0, 480) 
    screen.blit(open_text, opentextrect) 
    
    postextrect.topleft = (0, 496) 
    screen.blit(pos_text, postextrect) 
    
    firetextrect.topleft = (0, 512) 
    screen.blit(fire_text, firetextrect) 
    
    firepwrtextrect.topleft = (0, 528) 
    screen.blit(firepwr_text, firepwrtextrect) 
    
    eyepwrtextrect.topleft = (0, 544) 
    screen.blit(eyepwr_text, eyepwrtextrect) 
    
    eyedeltextrect.topleft = (0, 560) 
    screen.blit(eyedel_text, eyedeltextrect) 
    
    lasertextrect.topleft = (0, 576) 
    screen.blit(laser_text, lasertextrect) 
    
    lasertextrectlim.topleft = (0, 592) 
    screen.blit(laserlim_text, lasertextrectlim) 
    
    soundtextrectlim.topleft = (0, 608) 
    screen.blit(sound_text, soundtextrectlim) 
    
    safetytextrectlim.topleft = (0, 624) 
    screen.blit(safety_text, safetytextrectlim)
    
#used to send default programms to the turret
def SenderStart(temp_file):
    global sender_state
    global sender_file
    #try to open file
    try:
        sender_file = open("play files/" + str(temp_file) + ".txt", "r")
        sender_state = 1
        print("file started")
    except:
        sender_state = 0
        print("opening failed")
    
def SenderUpdate():
    global sender_state
    global sender_time_target
    global sender_file
    if (sender_state == 1):
        #check if it is time for the next command
        if (pygame.time.get_ticks() > sender_time_target):
            #line by line, send the file
            temp_line = sender_file.readline() #get line
            
            #check for end of line conditions
            if (temp_line == ""): #if line is empty
                print("end of file")
                sender_state = 0
            
            #check for alternative commands
            #check for comment
            if (temp_line.startswith('#')):
                pass
            #check for delay
            elif (temp_line.startswith('delay')):
                temp_delay = temp_line.partition(' ') #split at :
                temp_delay = int(temp_delay[2])
                #print(temp_delay)
                sender_time_target = pygame.time.get_ticks() + temp_delay #add waiting time
                print("Waiting " + str(temp_delay) + "ms")
            #check for sound
            elif (temp_line.startswith('sound1')):
                temp_sound = temp_line.partition(' ') #split at :
                temp_sound = int(temp_sound[2])
                turret_sound.PlayFound(temp_sound)
            elif (temp_line.startswith('soundp')):
                temp_sound = temp_line.partition(' ') #split at :
                temp_sound = int(temp_sound[2])
                turret_sound.PlayProgrammable(temp_sound)
            
            #send otherwise
            else:
                turret.SerialWriteBufferRaw(temp_line.rstrip()) #print actual line
                print(temp_line.rstrip())

while 1: #main loop
    for event in pygame.event.get():
        if event.type == pygame.QUIT: 
            turret.Disconnect()
            sys.exit()
    
    #update sender
    SenderUpdate()
    
    #once every 0.1s, update the key pressed function ONCE
    if (pygame.time.get_ticks() % 50 == 1 and update_limiter == 0):
        update_limiter = 1
        keys=pygame.key.get_pressed()
        key_pressed = 0
        
        #safety override
        if keys[pygame.K_DELETE]:
            if (key_lock == 0):
                if (manual_safety_override == 0):
                    manual_safety_override = 1
                else:
                    manual_safety_override = 0
                turret.SetSafetyOverride(manual_safety_override)
            key_pressed = 1
        
        #eye power
        if keys[pygame.K_e]:
            if (key_lock == 0):
                if (manual_eye_power == 0):
                    manual_eye_power = 255
                else:
                    manual_eye_power = 0
                turret.SetEyeLight(manual_eye_power)
            key_pressed = 1
            
        #eye delay
        if keys[pygame.K_d]:
            if (key_lock == 0):
                if (manual_eye_delay == 0):
                    manual_eye_delay = 1000
                else:
                    manual_eye_delay = 0
                turret.SetEyeDelay(manual_eye_delay)
            key_pressed = 1
            
        #laser
        if keys[pygame.K_l]:
            if (key_lock == 0):
                if (manual_laser_state == 0):
                    manual_laser_state = 1
                else:
                    manual_laser_state = 0
                turret.SetEyeLaser(manual_laser_state)
            key_pressed = 1
            
        #laser limit
        if keys[pygame.K_i]:
            if (key_lock == 0):
                if (manual_laser_limiter == 0):
                    manual_laser_limiter = 1
                else:
                    manual_laser_limiter = 0
                turret.SetEyeLimit(manual_laser_limiter)
            key_pressed = 1
            
        #mute (sound)
        #laser
        if keys[pygame.K_s]:
            if (key_lock == 0):
                if (manual_sound_state == 0):
                    manual_sound_state = 1
                else:
                    manual_sound_state = 0
                turret_sound.SetSound(manual_sound_state)
            key_pressed = 1
        
        #gun power
        if keys[pygame.K_COMMA]:
            if (key_lock == 0):
                if (manual_fire_power >= 10):
                    manual_fire_power -= 10
                turret.SetGunPower(manual_fire_power)
            key_pressed = 1
        if keys[pygame.K_PERIOD]:
            if (key_lock == 0):
                if (manual_fire_power <= 240):
                    manual_fire_power += 10
                turret.SetGunPower(manual_fire_power)
            key_pressed = 1
        
        #start functions
        if keys[pygame.K_F1]:
            if (key_lock == 0):
                if (sender_state == 0):
                    print("starting 1")
                    SenderStart(1)            
            key_pressed = 1
        if keys[pygame.K_F2]:
            if (key_lock == 0):
                if (sender_state == 0):
                    print("starting 2")
                    SenderStart(2)            
            key_pressed = 1
        if keys[pygame.K_F3]:
            if (key_lock == 0):
                if (sender_state == 0):
                    print("starting 3")
                    SenderStart(3)            
            key_pressed = 1
        if keys[pygame.K_F4]:
            if (key_lock == 0):
                if (sender_state == 0):
                    print("starting 4")
                    SenderStart(4)            
            key_pressed = 1
        if keys[pygame.K_F5]:
            if (key_lock == 0):
                if (sender_state == 0):
                    print("starting 5")
                    SenderStart(5)            
            key_pressed = 1
        if keys[pygame.K_F6]:
            if (key_lock == 0):
                if (sender_state == 0):
                    print("starting 6")
                    SenderStart(6)            
            key_pressed = 1
        if keys[pygame.K_F7]:
            if (key_lock == 0):
                if (sender_state == 0):
                    print("starting 7")
                    SenderStart(7)            
            key_pressed = 1
        if keys[pygame.K_F8]:
            if (key_lock == 0):
                if (sender_state == 0):
                    print("starting 8")
                    SenderStart(8)            
            key_pressed = 1
        if keys[pygame.K_F9]:
            if (key_lock == 0):
                if (sender_state == 0):
                    print("starting 9")
                    SenderStart(9)            
            key_pressed = 1
        if keys[pygame.K_F10]:
            if (key_lock == 0):
                if (sender_state == 0):
                    print("starting 10")
                    SenderStart(10)            
            key_pressed = 1
        if keys[pygame.K_F11]:
            if (key_lock == 0):
                if (sender_state == 0):
                    print("starting 11")
                    SenderStart(11)            
            key_pressed = 1
        if keys[pygame.K_F12]:
            if (key_lock == 0):
                if (sender_state == 0):
                    print("starting 12")
                    SenderStart(12)            
            key_pressed = 1
        if keys[pygame.K_p]:
            sender_state = 0 #abort file
            print("aborting reader")
        
        #programmable sound
        if keys[pygame.K_KP0]:
            if (key_lock == 0):
                if (sender_state == 0):
                    turret_sound.PlayProgrammable(0)            
            key_pressed = 1
        if keys[pygame.K_KP1]:
            if (key_lock == 0):
                if (sender_state == 0):
                    turret_sound.PlayProgrammable(1)            
            key_pressed = 1
        if keys[pygame.K_KP2]:
            if (key_lock == 0):
                if (sender_state == 0):
                    turret_sound.PlayProgrammable(2)            
            key_pressed = 1
        if keys[pygame.K_KP3]:
            if (key_lock == 0):
                if (sender_state == 0):
                    turret_sound.PlayProgrammable(3)            
            key_pressed = 1
        if keys[pygame.K_KP4]:
            if (key_lock == 0):
                if (sender_state == 0):
                    turret_sound.PlayProgrammable(4)            
            key_pressed = 1
        if keys[pygame.K_KP5]:
            if (key_lock == 0):
                if (sender_state == 0):
                    turret_sound.PlayProgrammable(5)            
            key_pressed = 1
        if keys[pygame.K_KP6]:
            if (key_lock == 0):
                if (sender_state == 0):
                    turret_sound.PlayProgrammable(6)            
            key_pressed = 1
        if keys[pygame.K_KP7]:
            if (key_lock == 0):
                if (sender_state == 0):
                    turret_sound.PlayProgrammable(7)            
            key_pressed = 1
        if keys[pygame.K_KP8]:
            if (key_lock == 0):
                if (sender_state == 0):
                    turret_sound.PlayProgrammable(8)            
            key_pressed = 1
        if keys[pygame.K_KP9]:
            if (key_lock == 0):
                if (sender_state == 0):
                    turret_sound.PlayProgrammable(9)            
            key_pressed = 1
            
        #pan (no limiter)
        if keys[pygame.K_LEFT]:
            if (manual_pan_position >= 8):
                manual_position_update = 1
                manual_pan_position -= 5
        if keys[pygame.K_RIGHT]:
            if (manual_pan_position <= 248):
                manual_position_update = 1
                manual_pan_position += 5
                
        #tilt (no limiter)
        if keys[pygame.K_DOWN]:
            if (manual_tilt_position >= 8):
                manual_position_update = 1
                manual_tilt_position -= 5
        if keys[pygame.K_UP]:
            if (manual_tilt_position <= 248):
                manual_position_update = 1
                manual_tilt_position += 5
                
        #open
        if keys[pygame.K_o]:
            if (key_lock == 0):
                if (sender_state == 0):
                    print("sending open command")
                    turret.Open()            
            key_pressed = 1
            
        #close
        if keys[pygame.K_c]:
            if (key_lock == 0):
                if (sender_state == 0):
                    print("sending close command")
                    turret.Close()            
            key_pressed = 1
        
        #fire (no limiter)
        if keys[pygame.K_SPACE]:
            if (manual_fire_state == 0):
                manual_fire_state = 1
                turret.SetFire(manual_fire_state)
        else:
            if (manual_fire_state == 1):
                manual_fire_state = 0
                turret.SetFire(manual_fire_state)
        
        #set home
        if keys[pygame.K_h]:
            if (key_lock == 0):
                manual_position_update = 1
                manual_pan_position = 127
                manual_tilt_position = 127
            key_pressed = 1
            
        #key locker (makes sure a button only toggles)
        if (key_pressed == 0):
            key_lock = 0
        else:
            key_lock = 1
        
        
        if (pygame.mouse.get_pressed()[0]):            
            click_position = pygame.mouse.get_pos()
            click_width_pos = int(RemapValue(click_position[0], click_size_border, click_width-click_size_border, 0,256))
            click_height_pos = int(RemapValue(click_position[1], click_height-click_size_border, click_size_border, -1,255))
            if (click_width_pos > 250):
                click_width_pos = 250
            if (click_width_pos < 5):
                click_width_pos = 5
            if (click_height_pos > 250):
                click_height_pos = 250
            if (click_height_pos < 5):
                click_height_pos = 5
            manual_pan_position = click_width_pos
            manual_tilt_position = click_height_pos
            manual_position_update = 1
                    
                    
        if (manual_position_update == 1):
            manual_position_update = 0
            #print(str(manual_pan_position) + "," + str(manual_tilt_position))
            turret.SetGunPosition(manual_pan_position, manual_tilt_position)
            #print(pygame.time.get_ticks())
        
                    
        #display loop
        if (camera_enabled == 1):
            image1 = cam.get_image()
            image1 = pygame.transform.scale(image1,(640,480))
            image1 = pygame.transform.flip(image1,True,True)
            screen.blit(image1,(0,0))
        PrintStates()
        pygame.display.update()
                    
            
    #reset the limiter that makes sure it happens only once
    if (pygame.time.get_ticks() % 50 == 2):
        update_limiter = 0
    
    #check errors and warnings
    if (pygame.time.get_ticks() % 2000 == 1 and error_limiter == 0):
        error_limiter = 1
        temp_warning = turret.GetWarning()
        if (temp_warning != 0):
            print("Warning: " + str(turret_settings.warning_table[temp_warning]))
        temp_error = turret.GetError()
        if (temp_error != 0):
            print("Error: " + str(turret_settings.error_table[temp_error]))
    
    if (pygame.time.get_ticks() % 2000 == 2):
        error_limiter = 0
        
