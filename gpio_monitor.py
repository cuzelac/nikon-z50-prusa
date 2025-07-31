#!/usr/bin/env python3

import RPi.GPIO as GPIO
import subprocess
import time
import logging
import os
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
DEBOUNCE_TIME = 0.2  # seconds
PHOTO_DIR = os.path.expanduser("~/timelapse")
MAX_RETRIES = 3
CAPTURE_TIMEOUT = 30

# GPIO Setup
GPIO.setmode(GPIO.BCM)
GPIO.setup(TRIGGER_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)

last_trigger_time = 0

def setup_photo_directory():
    """Create photo directory if it doesn't exist"""
    try:
        os.makedirs(PHOTO_DIR, exist_ok=True)
        logging.info(f"Photo directory ready: {PHOTO_DIR}")
        return True
    except Exception as e:
        logging.error(f"Failed to create photo directory {PHOTO_DIR}: {e}")
        return False

def check_camera():
    """Check if camera is connected and accessible"""
    try:
        result = subprocess.run(['gphoto2', '--auto-detect'], 
                              capture_output=True, text=True, timeout=10)
        if result.returncode == 0 and 'usb:' in result.stdout:
            logging.info("Camera detected and ready")
            return True
        else:
            logging.error("No camera detected - check USB connection and camera mode (should be PTP)")
            return False
    except subprocess.TimeoutExpired:
        logging.error("Camera detection timed out")
        return False
    except Exception as e:
        logging.error(f"Error checking camera: {e}")
        return False

def capture_photo():
    """Capture photo with retry logic, returns (success, filename)"""
    if not setup_photo_directory():
        return False, None
        
    for attempt in range(1, MAX_RETRIES + 1):
        try:
            # Generate timestamped filename
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"frame_{timestamp}.jpg"
            filepath = os.path.join(PHOTO_DIR, filename)
            
            logging.info(f"Capture attempt {attempt}/{MAX_RETRIES}")
            
            # Build gphoto2 command
            cmd = [
                'gphoto2',
                '--set-config', 'autofocusdrive=1',
                '--capture-image-and-download',
                '--filename', filepath
            ]
            
            # Execute capture command
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=CAPTURE_TIMEOUT)
            
            if result.returncode == 0:
                # Verify file was created
                if os.path.exists(filepath):
                    file_size = os.path.getsize(filepath)
                    logging.info(f"Successfully captured: {filename} ({file_size} bytes)")
                    return True, filename
                else:
                    logging.warning(f"Capture command succeeded but file not found: {filepath}")
            else:
                logging.warning(f"Attempt {attempt} failed - gphoto2 error: {result.stderr.strip()}")
                
        except subprocess.TimeoutExpired:
            logging.warning(f"Attempt {attempt} failed - capture timed out after {CAPTURE_TIMEOUT}s")
        except Exception as e:
            logging.warning(f"Attempt {attempt} failed - exception: {e}")
            
        # Wait before retry (except on last attempt)
        if attempt < MAX_RETRIES:
            time.sleep(2)
    
    logging.error(f"All {MAX_RETRIES} capture attempts failed")
    return False, None

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
            success, filename = capture_photo()
            if success:
                logging.info(f"Photo captured successfully: {filename}")
            else:
                logging.error("Photo capture failed after all retry attempts")
        except Exception as e:
            logging.error(f"Unexpected error during photo capture: {e}")

# Setup interrupt on both rising and falling edges
GPIO.add_event_detect(TRIGGER_PIN, GPIO.BOTH, callback=gpio_callback)

# Initial camera check
if not check_camera():
    logging.error("Camera not ready - exiting")
    GPIO.cleanup()
    exit(1)

logging.info(f"Monitoring GPIO pin {TRIGGER_PIN} for triggers...")
logging.info(f"Photos will be saved to: {PHOTO_DIR}")

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    logging.info("Stopping GPIO monitor...")
finally:
    GPIO.cleanup()