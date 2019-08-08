from playsound import playsound
import time
from random import randint
import os
import threading
import pygame

class TurretSound():
    def __init__(self):
        #look for the files that can be played
        self.sounds_formats = [".wav", ".Wav", ".WAV", ".mp3", ".Mp3", ".MP3"] #the allowed formats that can be played
        self.sounds_targetfound = []  #the to be filled list with target found noises
        self.sounds_targetlost = []  #the to be filled list with target lost noises
        self.sounds_programmable = [] #the to be filled list for the programmable keys
        
        #look for target found
        for root, dirs, files in os.walk("Turret_sounds/TargetFound"):
            for file in files:
                for temp_format in self.sounds_formats:
                    if file.endswith(temp_format):
                         self.sounds_targetfound.append(os.path.join(root, file))

        #look for target lost
        for root, dirs, files in os.walk("Turret_sounds/TargetLost"):
            for file in files:
                for temp_format in self.sounds_formats:
                    if file.endswith(temp_format):
                        self.sounds_targetlost.append(os.path.join(root, file))
                        
        #look for programmable
        for root, dirs, files in os.walk("Turret_sounds/Programmable"):
            for file in files:
                for temp_format in self.sounds_formats:
                    if file.endswith(temp_format):
                        self.sounds_programmable.append(os.path.join(root, file))
        self.sounds_programmable = sorted(self.sounds_programmable)
        
        self.sounds_file_to_play = ""
        self.sound_file_playing = 0
        self.sound_state = 1 #muted or not (1 is sound, 0 is muted)
        pygame.mixer.init()
        
        self._stop_event = threading.Event()
        self.update_thread = threading.Thread(target=self.Update)
        self.update_thread.start()
                
        #debug functions
        print(self.sounds_targetfound)
        print(self.sounds_targetlost)
        print(self.sounds_programmable)
    
    def Update(self):
        while not self._stop_event.is_set():
            time.sleep(0.1)
            #print("update")
            if (pygame.mixer.music.get_busy() == 0):
                #print("File started")
                #playsound(self.sounds_file_to_play, True)
                self.sound_file_playing = 0
                
        
    def PlayFound(self, temp_file = -1):
        """Plays either a defined or a random sound in the 'Turret sounds/TargetFound' directory"""
        if (self.sound_state == 1):
            if (temp_file == -1): #if no file was given, pick one at random
                temp_file = randint(0, len(self.sounds_targetfound) - 1)
            self.sounds_file_to_play = self.sounds_targetfound[temp_file]
            self.sound_file_playing = 1
            pygame.mixer.music.load(self.sounds_file_to_play)
            pygame.mixer.music.play()
        
    def PlayLost(self, temp_file = -1):
        """Plays either a defined or a random sound in the 'Turret sounds/TargetLost' directory"""
        if (self.sound_state == 1):
            if (temp_file == -1): #if no file was given, pick one at random
                temp_file = randint(0, len(self.sounds_targetlost) - 1)
            self.sounds_file_to_play = self.sounds_targetlost[temp_file]
            self.sound_file_playing = 1
            pygame.mixer.music.load(self.sounds_file_to_play)
            pygame.mixer.music.play()
            
    def PlayProgrammable(self, temp_file = -1):
        """Plays either a defined or a random sound in the 'Turret sounds/TargetLost' directory"""
        if (self.sound_state == 1):
            if (temp_file == -1): #if no file was given, pick one at random
                temp_file = randint(0, len(self.sounds_programmable) - 1)
            self.sounds_file_to_play = self.sounds_programmable[temp_file]
            self.sound_file_playing = 1
            pygame.mixer.music.load(self.sounds_file_to_play)
            pygame.mixer.music.play()
    
    def IsPlaying(self):
        """returns a 1 if a sound is currently playing, else a 0"""
        return self.sound_file_playing
        
    def SetSound(self, temp_state):
        self.sound_state = temp_state
        
        
    



if __name__ == '__main__':
    temp_test = TurretSound()
    temp_test.PlayFound()
    while (temp_test.IsPlaying() == 1):
        time.sleep(0.05)
        #print("waiting")
    #print("Play next")
    temp_test.PlayProgrammable()
