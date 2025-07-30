#!/bin/bash
# gpio_monitor.sh - Monitor GPIO pin 18 and trigger photo capture script

TRIGGER_PIN=18
SCRIPT_PATH="/home/pi/capture_photo.sh"
LOG_FILE="/home/pi/gpio_monitor.log"

# Export the GPIO pin if not already exported
if [ ! -d /sys/class/gpio/gpio${TRIGGER_PIN} ]; then
    echo $TRIGGER_PIN > /sys/class/gpio/export
fi

echo "in" > /sys/class/gpio/gpio${TRIGGER_PIN}/direction
echo "rising" > /sys/class/gpio/gpio${TRIGGER_PIN}/edge

echo "$(date): GPIO monitor started on pin $TRIGGER_PIN" >> $LOG_FILE

# Trap to cleanup on exit
cleanup() {
    echo $TRIGGER_PIN > /sys/class/gpio/unexport
    echo "$(date): GPIO monitor stopped" >> $LOG_FILE
    exit 0
}
trap cleanup SIGINT SIGTERM

while true; do
    # Wait for GPIO edge detection
    read < /sys/class/gpio/gpio${TRIGGER_PIN}/value
    if [ $(cat /sys/class/gpio/gpio${TRIGGER_PIN}/value) -eq 1 ]; then
        echo "$(date): Trigger detected - capturing photo" >> $LOG_FILE
        $SCRIPT_PATH >> $LOG_FILE 2>&1
        sleep 0.5  # Debounce delay
    fi
    # Wait for pin to go low before next trigger
    while [ $(cat /sys/class/gpio/gpio${TRIGGER_PIN}/value) -eq 1 ]; do
        sleep 0.05
    done
    sleep 0.05
    # Loop continues

done