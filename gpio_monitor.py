#!/usr/bin/env python3

import RPi.GPIO as GPIO
import subprocess
import time
import logging
from datetime import datetime

# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(message)s',
    handlers=[
        logging.FileHandler('gpio_monitor.log'),
        logging.StreamHandler()
    ]
)

# Configuration
TRIGGER_PIN = 18  # BCM numbering
SCRIPT_PATH = './timelapse_capture.sh'
DEBOUNCE_TIME = 0.2  # seconds

# GPIO Setup
GPIO.setmode(GPIO.BCM)
GPIO.setup(TRIGGER_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)

last_trigger_time = 0

def gpio_callback(channel):
    """Callback function for GPIO interrupt"""
    global last_trigger_time
    current_time = time.time()
    # Debounce - ignore rapid successive triggers
    if current_time - last_trigger_time < DEBOUNCE_TIME:
        return
    last_trigger_time = current_time
    # Check if it's a falling edge (HIGH to LOW transition)
    if GPIO.input(TRIGGER_PIN) == GPIO.LOW:
        logging.info(f"Trigger detected at {datetime.now()}")
        try:
            result = subprocess.run([SCRIPT_PATH], capture_output=True, text=True, timeout=30)
            if result.returncode == 0:
                logging.info("Photo captured successfully")
            else:
                logging.error(f"Capture failed: {result.stderr}")
        except Exception as e:
            logging.error(f"Error: {e}")

# Setup interrupt on both rising and falling edges
GPIO.add_event_detect(TRIGGER_PIN, GPIO.BOTH, callback=gpio_callback)

logging.info(f"Monitoring GPIO pin {TRIGGER_PIN} for triggers...")

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    logging.info("Stopping GPIO monitor...")
finally:
    GPIO.cleanup()