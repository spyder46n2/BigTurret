

class TurretSettings():
    def __init__(self):
        self.autostart = 1
        self.com_port = "/dev/ttyACM0"
        #self.com_port = "com6"
        self.eye_limiter = 0
        self.gun_power = 127
        
        self.eye_laser_default_on = 1
        self.eye_light_default_on = 1
        self.eye_brightness = 255
        self.eye_brightness_steps = 10 
        self.eye_on_time = 2000 #how long it takes for the eye to turn on on boot
        
        self.open_delay = 1000
        self.close_delay = 2000
        
        self.fire_delay = 8000
        
        self.safety_override = 0
        
        self.warning_table = {0: 'nothing', 
        10: 'feeder left top locked',
        15: 'feeder left bottom locked',
        20: 'feeder right top locked',
        25: 'feeder right bottom locked',
        30: 'feeder left top runaway', 
        35: 'feeder left bottom runaway', 
        40: 'feeder right top runaway',
        45: 'feeder right bottom runaway'}
        
        self.error_table = {0: 'nothing', 
        10: 'pan out of bounds',
        15: 'open out of bounds',
        20: 'pan axis locked',
        25: 'open axis locked',
        50: 'opening timout',
        55: 'closing return to center timeout',
        56: 'closing movement timeout',
        80: 'pan passage check',
        85: 'left tilt passage check',
        86: 'right tilt passage check'}
        
