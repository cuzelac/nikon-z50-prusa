#!/usr/bin/env python3
"""
gpio_debug_monitor.py - Monitor ALL GPIO pins for activity
This script helps debug which GPIO pins are receiving signals
"""

import RPi.GPIO as GPIO
import time
import signal
import sys
from datetime import datetime
import argparse

# Available GPIO pins (BCM numbering) - excludes power/ground pins
AVAILABLE_PINS = [2, 3, 4, 14, 15, 17, 18, 27, 22, 23, 24, 10, 9, 25, 11, 8, 7, 5, 6, 12, 13, 19, 16, 26, 20, 21]

# GPIO to Physical pin mapping for reference
GPIO_TO_PHYSICAL = {
    2: 3, 3: 5, 4: 7, 14: 8, 15: 10, 17: 11, 18: 12, 27: 13,
    22: 15, 23: 16, 24: 18, 10: 19, 9: 21, 25: 22, 11: 23, 8: 24,
    7: 26, 5: 29, 6: 31, 12: 32, 13: 33, 19: 35, 16: 36, 26: 37, 20: 38, 21: 40
}

class GPIOMonitor:
    def __init__(self, pins_to_monitor=None, show_initial=True, poll_mode=False, poll_time=0.1):
        self.pins_to_monitor = pins_to_monitor or AVAILABLE_PINS
        self.show_initial = show_initial
        self.poll_mode = poll_mode
        self.poll_time = poll_time
        self.pin_states = {}
        self.activity_count = {}
        self.running = True
        
        # Setup signal handler for clean exit
        signal.signal(signal.SIGINT, self.signal_handler)
        
        # Initialize GPIO
        GPIO.setmode(GPIO.BCM)
        GPIO.setwarnings(False)
        
    def setup_pins(self):
        """Setup all pins for input with pull-up resistors"""
        print(f"Setting up {len(self.pins_to_monitor)} GPIO pins for monitoring...")
        print("Pin mapping (BCM -> Physical pin):")
        
        for pin in self.pins_to_monitor:
            try:
                GPIO.setup(pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)
                initial_state = GPIO.input(pin)
                self.pin_states[pin] = initial_state
                self.activity_count[pin] = 0
                
                phys_pin = GPIO_TO_PHYSICAL.get(pin, '?')
                print(f"  GPIO{pin:2d} (pin {phys_pin:2}) -> {'HIGH' if initial_state else 'LOW'}")
                
                if not self.poll_mode:
                    # Setup interrupt-based detection
                    GPIO.add_event_detect(pin, GPIO.BOTH, callback=self.gpio_callback, bouncetime=50)
                    
            except Exception as e:
                print(f"Failed to setup GPIO{pin}: {e}")
                
        print(f"\nMonitoring started at {datetime.now()}")
        if self.poll_mode:
            poll_ms = self.poll_time * 1000
            print(f"Mode: Polling (checking every {poll_ms:.1f}ms)")
        else:
            print("Mode: Interrupt-driven (immediate detection)")
        print("Press Ctrl+C to stop\n")
        
    def gpio_callback(self, channel):
        """Callback for GPIO state changes (interrupt mode)"""
        current_state = GPIO.input(channel)
        previous_state = self.pin_states.get(channel, current_state)
        
        if current_state != previous_state:
            self.pin_states[channel] = current_state
            self.activity_count[channel] += 1
            
            state_str = "HIGH" if current_state else "LOW"
            transition = "LOW->HIGH" if current_state else "HIGH->LOW"
            phys_pin = GPIO_TO_PHYSICAL.get(channel, '?')
            
            print(f"{datetime.now().strftime('%H:%M:%S.%f')[:-3]} | "
                  f"GPIO{channel:2d} (pin {phys_pin:2}) | {transition:9} | Now: {state_str} | "
                  f"Count: {self.activity_count[channel]}")
            
    def poll_pins(self):
        """Poll all pins for changes (polling mode)"""
        while self.running:
            for pin in self.pins_to_monitor:
                try:
                    current_state = GPIO.input(pin)
                    previous_state = self.pin_states.get(pin, current_state)
                    
                    if current_state != previous_state:
                        self.pin_states[pin] = current_state
                        self.activity_count[pin] += 1
                        
                        state_str = "HIGH" if current_state else "LOW"
                        transition = "LOW->HIGH" if current_state else "HIGH->LOW"
                        phys_pin = GPIO_TO_PHYSICAL.get(pin, '?')
                        
                        print(f"{datetime.now().strftime('%H:%M:%S.%f')[:-3]} | "
                              f"GPIO{pin:2d} (pin {phys_pin:2}) | {transition:9} | Now: {state_str} | "
                              f"Count: {self.activity_count[pin]}")
                        
                except Exception as e:
                    print(f"Error reading GPIO{pin}: {e}")
                    
            time.sleep(self.poll_time)  # Poll at configured interval
            
    def run_interrupt_mode(self):
        """Run in interrupt mode"""
        try:
            while self.running:
                time.sleep(1)
                # Print activity summary every 10 seconds
                if int(time.time()) % 10 == 0:
                    active_pins = [pin for pin, count in self.activity_count.items() if count > 0]
                    if active_pins:
                        print(f"[{datetime.now().strftime('%H:%M:%S')}] Active pins: {active_pins}")
                    time.sleep(1)  # Avoid multiple prints in same second
        except:
            pass
            
    def signal_handler(self, signum, frame):
        """Handle Ctrl+C gracefully"""
        print(f"\n\nStopping monitor at {datetime.now()}")
        self.running = False
        self.cleanup()
        
    def cleanup(self):
        """Print summary and cleanup GPIO"""
        print("\n" + "="*60)
        print("ACTIVITY SUMMARY:")
        print("="*60)
        
        active_pins = [(pin, count) for pin, count in self.activity_count.items() if count > 0]
        
        if active_pins:
            print("Pins with activity:")
            for pin, count in sorted(active_pins, key=lambda x: x[1], reverse=True):
                phys_pin = GPIO_TO_PHYSICAL.get(pin, '?')
                final_state = "HIGH" if self.pin_states[pin] else "LOW"
                print(f"  GPIO{pin:2d} (pin {phys_pin:2}): {count:2d} changes, final state: {final_state}")
        else:
            print("No activity detected on any monitored pins.")
            print("Possible issues:")
            print("  - No signal being sent")
            print("  - Wrong pin connected")
            print("  - Signal voltage too low/high")
            print("  - Bad connection")
            
        print("\nFinal pin states:")
        for pin in sorted(self.pins_to_monitor):
            phys_pin = GPIO_TO_PHYSICAL.get(pin, '?')
            state = "HIGH" if self.pin_states[pin] else "LOW"
            print(f"  GPIO{pin:2d} (pin {phys_pin:2}): {state}")
            
        GPIO.cleanup()
        sys.exit(0)

def main():
    parser = argparse.ArgumentParser(description="Monitor GPIO pins for activity")
    parser.add_argument("--pins", nargs="+", type=int, 
                       help="Specific GPIO pins to monitor (default: all available)")
    parser.add_argument("--poll", action="store_true", 
                       help="Use polling mode instead of interrupts")
    parser.add_argument("--poll-time", type=float, default=0.1,
                       help="Polling interval in seconds (default: 0.1)")
    parser.add_argument("--no-initial", action="store_true",
                       help="Don't show initial pin states")
    
    args = parser.parse_args()
    
    pins = args.pins if args.pins else AVAILABLE_PINS
    
    # Validate pins
    valid_pins = [pin for pin in pins if pin in AVAILABLE_PINS]
    if len(valid_pins) != len(pins):
        invalid = [pin for pin in pins if pin not in AVAILABLE_PINS]
        print(f"Warning: Invalid pins ignored: {invalid}")
    
    if not valid_pins:
        print("No valid pins to monitor!")
        sys.exit(1)
        
    monitor = GPIOMonitor(
        pins_to_monitor=valid_pins,
        show_initial=not args.no_initial,
        poll_mode=args.poll,
        poll_time=args.poll_time
    )
    
    monitor.setup_pins()
    
    if monitor.poll_mode:
        monitor.poll_pins()
    else:
        monitor.run_interrupt_mode()

if __name__ == "__main__":
    main()