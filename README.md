# Nikon Z50 Timelapse Capture with Raspberry Pi & Prusa GPIO

This project enables automated timelapse photography with a Nikon Z50, triggered by a Prusa 3D printer's GPIO board, using a Raspberry Pi and gPhoto2.

---

## Features
- Capture a photo every time a print layer completes
- Robust logging and retry logic
- Works with Prusa GPIO board or any GPIO trigger
- Compatible with Nikon Z50 (and many other cameras)
- Can be tested on native Linux or WSL2 (with USB pass-through)

---

## Hardware Requirements
- Raspberry Pi (any model with USB)
- Nikon Z50 (or compatible camera)
- USB cable (camera to Pi)
- Prusa GPIO Hackerboard (optional, for printer integration)
- (Optional) Relay or optocoupler for electrical isolation

---

## Software Requirements
- Raspbian OS or any Debian-based Linux
- gPhoto2 (`sudo apt-get install gphoto2`)
- Python 3 (for advanced GPIO monitoring)
- RPi.GPIO or gpiozero (for Python GPIO scripts)

---

## Setup

### 1. Connect the Camera
- Set the Nikon Z50 USB mode to **PTP** (not Mass Storage)
- Connect the camera to the Pi via USB

### 2. Install Dependencies
```bash
sudo apt-get update
sudo apt-get install gphoto2 python3-rpi.gpio
```

### 3. Deploy Scripts
- Place `timelapse_capture.sh` and your chosen GPIO monitor script (`gpio_monitor.sh` or `gpio_monitor.py`) in `/home/pi/`
- Make them executable:
```bash
chmod +x /home/pi/timelapse_capture.sh /home/pi/gpio_monitor.sh /home/pi/gpio_monitor.py
```

### 4. Test Camera
```bash
gphoto2 --auto-detect
gphoto2 --summary
gphoto2 --capture-image-and-download
```

### 5. Test Script
```bash
/home/pi/timelapse_capture.sh
```

### 6. Set Up GPIO Trigger
- Wire the Prusa GPIO output pin to Raspberry Pi GPIO 18 (physical pin 12)
- Use the shell or Python monitor script to trigger photo capture on pin change

---

## Usage

### Start the GPIO Monitor
**Shell script:**
```bash
./gpio_monitor.sh
```
**Python script:**
```bash
python3 gpio_monitor.py
```

### Timelapse Capture Script
- The script will create `/home/pi/timelapse/` and save images as `frame_YYYYMMDD_HHMMSS.jpg`
- Logs are written to `/home/pi/capture.log`

---

## WSL2 (Windows Subsystem for Linux) Notes
- WSL2 can access USB devices using [usbipd-win](https://github.com/dorssel/usbipd-win)
- Install usbipd-win on Windows and USB/IP tools in WSL2
- Attach the camera to WSL2 using PowerShell:
  ```powershell
  usbipd list
  usbipd attach --busid <BUSID> --wsl
  ```
- In WSL2, verify with `lsusb` and test with gPhoto2
- Not all USB devices are supported; camera must be in PTP mode

---

## Troubleshooting

### Camera Not Detected
- Ensure camera is in **PTP** mode
- Use a good quality USB cable
- Try different USB ports
- Run `gphoto2 --auto-detect` and check output
- On WSL2, make sure the device is attached to WSL and not mounted in Windows

### Permission Issues
- Add your user to the `plugdev` group:
  ```bash
  sudo usermod -a -G plugdev $USER
  ```
- Reboot or log out/in after adding to group

### gPhoto2 Errors
- Try running as root (`sudo gphoto2 ...`) for testing
- Check camera battery and memory card
- If you see `ERROR: No camera detected`, check cable and camera mode

### GPIO Not Triggering
- Double-check wiring (Pi GND to GPIO board GND, GPIO18 to output pin)
- Use `gpio readall` or `raspi-gpio get 18` to check pin state
- Add debug prints/logging to your monitor script

### Script Not Running Automatically
- Use `systemd` to run the monitor script as a service (see below)

---

## Running as a Service (Optional)
Create `/etc/systemd/system/gpio-camera.service`:
```ini
[Unit]
Description=GPIO Camera Trigger Service
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi
ExecStart=/usr/bin/python3 /home/pi/gpio_monitor.py
Restart=always

[Install]
WantedBy=multi-user.target
```
Enable and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable gpio-camera.service
sudo systemctl start gpio-camera.service
```

---

## Credits
- gPhoto2 project: http://gphoto.org/
- Prusa Research for GPIO board
- [usbipd-win](https://github.com/dorssel/usbipd-win) for WSL2 USB support

---

## License
MIT or public domain. Use at your own risk.