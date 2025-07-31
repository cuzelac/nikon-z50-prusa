#!/usr/bin/env python3
"""
simulate_prusa.py - Simulate Prusa GPIO signals for testing
Use another GPIO pin to simulate the Prusa hackerboard output
"""

import RPi.GPIO as GPIO
import time
import argparse

# Configuration
OUTPUT_PIN = 23  # Use GPIO23 (pin 16) to simulate Prusa output
PULSE_DURATION = 0.1  # How long the trigger pulse lasts
LAYER_INTERVAL = 5.0  # Simulate 5 seconds per "layer"

def setup_gpio():
    """Setup GPIO for output simulation"""
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(OUTPUT_PIN, GPIO.OUT)
    GPIO.output(OUTPUT_PIN, GPIO.HIGH)  # Start HIGH (idle state)
    print(f"GPIO{OUTPUT_PIN} (pin 16) configured as output")
    print("Connect a jumper wire from pin 16 to pin 12 to test")

def simulate_layer_complete():
    """Simulate a layer completion signal (HIGH to LOW pulse)"""
    print(f"Simulating layer complete - pulling GPIO{OUTPUT_PIN} LOW")
    GPIO.output(OUTPUT_PIN, GPIO.LOW)
    time.sleep(PULSE_DURATION)
    GPIO.output(OUTPUT_PIN, GPIO.HIGH)
    print(f"Signal complete - GPIO{OUTPUT_PIN} back to HIGH")

def continuous_simulation(layers=10):
    """Run continuous layer simulation"""
    print(f"Starting continuous simulation for {layers} layers")
    print(f"Layer interval: {LAYER_INTERVAL}s, Pulse duration: {PULSE_DURATION}s")
    print("Press Ctrl+C to stop early")
    
    try:
        for layer in range(1, layers + 1):
            print(f"\n--- Layer {layer} ---")
            print(f"Printing layer {layer}... (waiting {LAYER_INTERVAL}s)")
            time.sleep(LAYER_INTERVAL)
            simulate_layer_complete()
            
        print(f"\nSimulation complete! {layers} layers processed.")
        
    except KeyboardInterrupt:
        print("\nSimulation stopped by user")
    finally:
        GPIO.cleanup()

def single_pulse():
    """Send a single test pulse"""
    print("Sending single test pulse...")
    simulate_layer_complete()
    GPIO.cleanup()
    print("Single pulse complete")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Simulate Prusa GPIO signals")
    parser.add_argument("--single", action="store_true", help="Send single pulse")
    parser.add_argument("--layers", type=int, default=10, help="Number of layers to simulate")
    parser.add_argument("--interval", type=float, default=5.0, help="Seconds between layers")
    
    args = parser.parse_args()
    
    LAYER_INTERVAL = args.interval
    
    setup_gpio()
    
    if args.single:
        single_pulse()
    else:
        continuous_simulation(args.layers)