import time
import sys
import random

# --- Write header ---
sys.stdout.write("gender,region,label,predict\n")
sys.stdout.flush()

while True:
    gender = random.choice(["man", "woman", "other"])
    region = random.choice(["here", "there", "everywhere", "nowhere"])
    label = random.choice(["0", "1"])
    predict = random.choice(["0", "1"])
    sys.stdout.write(f"{gender},{region},{label},{predict}\n")

    if random.random() < 0.05:
        sys.stdout.flush() # randomly flush for demonstration
    time.sleep(0.01) # mimics a slow algorithm (not needed)
