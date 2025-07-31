#!/usr/bin/env python3
"""
test_ground_connection.py - Test if ground connection is the issue
"""

import RPi.GPIO as GPIO
import time

# Test pin
TEST_PIN = 18

GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

print("Ground Connection Test")
print("=====================")

# Test with pull-up (your current setup)
GPIO.setup(TEST_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
state_pullup = GPIO.input(TEST_PIN)
print(f"GPIO{TEST_PIN} with pull-up: {'HIGH' if state_pullup else 'LOW'}")

# Test with pull-down
GPIO.setup(TEST_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
state_pulldown = GPIO.input(TEST_PIN)
print(f"GPIO{TEST_PIN} with pull-down: {'HIGH' if state_pulldown else 'LOW'}")

# Test floating (no pull resistor)
GPIO.setup(TEST_PIN, GPIO.IN, pull_up_down=GPIO.PUD_OFF)
state_floating = GPIO.input(TEST_PIN)
print(f"GPIO{TEST_PIN} floating: {'HIGH' if state_floating else 'LOW'}")

print("\nInterpretation:")
if state_pullup == state_pulldown == state_floating:
    if state_pullup:
        print("- Pin is being driven HIGH by external source")
        print("- Ground connection might be working")
    else:
        print("- Pin is being driven LOW by external source")
        print("- Ground connection might be working")
else:
    print("- Pin state changes with pull resistor setting")
    print("- This suggests NO external signal or BAD ground connection")
    print("- CHECK YOUR GROUND CONNECTION!")

print(f"\nExpected behavior with proper connection:")
print(f"- Pull-up: HIGH (external signal can pull LOW)")
print(f"- Pull-down: LOW (external signal can drive HIGH)")
print(f"- Floating: Unpredictable (depends on external signal)")

GPIO.cleanup()