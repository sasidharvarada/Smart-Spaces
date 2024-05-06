import pygame
import time

# Initialize pygame
pygame.init()

# Set the sound file path
sound_file = "beep.wav"  # Replace with the actual path to your sound file

# Define the interval in seconds (10 minutes)
interval_seconds = 600

while True:
    try:
        # Load and play the sound
        pygame.mixer.music.load(sound_file)
        pygame.mixer.music.play()
        
        # Wait for the sound to finish
        while pygame.mixer.music.get_busy():
            pygame.time.Clock().tick(10)
        
        # Wait for the specified interval
        time.sleep(interval_seconds)
    
    except KeyboardInterrupt:
        # Exit the script if the user presses Ctrl+C
        pygame.mixer.quit()
        break
