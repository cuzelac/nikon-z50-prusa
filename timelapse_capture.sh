#!/bin/bash
# timelapse_capture.sh - Complete script for timelapse photo capture with logging and retries

PHOTO_DIR="/home/pi/timelapse"
LOG_FILE="/home/pi/capture.log"
MAX_RETRIES=3

mkdir -p "$PHOTO_DIR"
cd "$PHOTO_DIR"

log_message() {
    echo "$(date '+%Y-%m-%d %H:%M:%S'): $1" >> "$LOG_FILE"
}

capture_with_retry() {
    local attempt=1
    while [ $attempt -le $MAX_RETRIES ]; do
        log_message "Capture attempt $attempt"
        # Generate filename with timestamp
        timestamp=$(date +"%Y%m%d_%H%M%S")
        filename="frame_${timestamp}.jpg"
        # Try to capture
        if timeout 30 gphoto2 --capture-image-and-download --filename "$filename" 2>>"$LOG_FILE"; then
            log_message "Successfully captured: $filename"
            echo "$filename"  # Return filename for logging
            return 0
        else
            log_message "Capture attempt $attempt failed"
            attempt=$((attempt + 1))
            sleep 2
        fi
    done
    log_message "All capture attempts failed"
    return 1
}

# Check if camera is connected
if ! gphoto2 --auto-detect | grep -q "usb:"; then
    log_message "ERROR: No camera detected"
    exit 1
fi

# Optionally configure camera settings (uncomment and adjust as needed)
# gphoto2 --set-config iso=400
# gphoto2 --set-config shutterspeed=1/60

# Attempt capture
if capture_with_retry; then
    log_message "Capture sequence completed successfully"
else
    log_message "Capture sequence failed after $MAX_RETRIES attempts"
    exit 1
fi