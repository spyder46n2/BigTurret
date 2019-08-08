

class TurretMotion():
    def __init__(self):
        self.open_state = 0 #if the turret is open or closed
        
        self.gun_pan = 127 #the gun pan position
        self.gun_tilt = 127 #the gun tilt position
        self.gun_motion_state = 0 #whether there are errors or no (0 is ok)
        self.gun_motion_moving = 0 #whether the turret is moving
        
        self.gun_eye_link = 1 #whether eyes move together with the guns
        
        self.eye_pan = 127 #the eye pan position
        self.eye_tilt = 127 #the eye tilt position
        self.eye_light = 0 #the light of the eye
        self.eye_laser = 0 #the eye laser
        
        self.gun_left_master_feed = 0 #whether master feed is on
        self.gun_left_top_fire = 0 #whether the gun needs to fire
        self.gun_left_top_feedback = 0 #the feedback signal of the feeder
        self.gun_left_top_state = 0 #whether the feeder is jammed or not
        self.gun_left_bottom_fire = 0 #whether the gun needs to fire
        self.gun_left_bottom_feedback = 0 #the feedback signal of the feeder
        self.gun_left_bottom_state = 0 #whether the feeder is jammed or not
        
        self.gun_right_master_feed = 0 #whether master feed is on
        self.gun_right_top_fire = 0 #whether the gun needs to fire
        self.gun_right_top_feedback = 0 #the feedback signal of the feeder
        self.gun_right_top_state = 0 #whether the feeder is jammed or not
        self.gun_right_bottom_fire = 0 #whether the gun needs to fire
        self.gun_right_bottom_feedback = 0 #the feedback signal of the feeder
        self.gun_right_bottom_state = 0 #whether the feeder is jammed or not
