#!/bin/bash
# test_gpio.sh - Manual GPIO testing script

echo "GPIO Test Script for Prusa Timelapse"
echo "===================================="

# Check if running as root (needed for gpio commands)
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root: sudo ./test_gpio.sh"
    exit 1
fi

echo "1. Checking GPIO pin 18 current state..."
if command -v raspi-gpio >/dev/null 2>&1; then
    raspi-gpio get 18
elif command -v gpio >/dev/null 2>&1; then
    gpio -g mode 18 in
    gpio -g read 18
else
    echo "Neither raspi-gpio nor gpio command found"
    echo "Install with: sudo apt install wiringpi"
fi

echo ""
echo "2. Monitoring GPIO pin 18 for changes..."
echo "   Connect Prusa signal to Pin 12 (GPIO18)"
echo "   Connect Prusa GND to Pin 14 (GND)"
echo "   Press Ctrl+C to stop"

# Simple monitoring loop
LAST_STATE=-1
while true; do
    if command -v raspi-gpio >/dev/null 2>&1; then
        CURRENT_STATE=$(raspi-gpio get 18 | grep -o "level=[01]" | cut -d= -f2)
    elif command -v gpio >/dev/null 2>&1; then
        CURRENT_STATE=$(gpio -g read 18)
    else
        echo "Cannot read GPIO state"
        break
    fi
    
    if [ "$CURRENT_STATE" != "$LAST_STATE" ]; then
        echo "$(date): GPIO18 changed to $CURRENT_STATE"
        LAST_STATE=$CURRENT_STATE
    fi
    sleep 0.1
done