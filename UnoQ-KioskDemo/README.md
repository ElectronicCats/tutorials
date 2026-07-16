# Arduino UNO Q Kiosk Demo with GIGA Display

> A reference implementation demonstrating how to build a kiosk-style Human-Machine Interface (HMI) on the Arduino UNO Q using an Arduino GIGA Display connected through a custom carrier PCB.

---

# Why this project?

The Arduino UNO Q combines a Linux-capable Qualcomm® processor (MPU) with an STM32 microcontroller (MCU). This architecture enables developers to build modern embedded applications with rich graphical interfaces while maintaining the real-time reliability of traditional Arduino sketches.

This project is a **proof of concept** demonstrating collaborative work across hardware, kernel-level drivers, and user space software:

1. **Hardware:** A custom carrier PCB that routes the DSI video and I²C touch lines from the UNO Q to an Arduino GIGA Display.
2. **Drivers:** Custom Linux display and touch panel drivers to make the GIGA Display fully operational on the UNO Q Debian environment.
3. **Application:** A minimalist, full-screen kiosk UI.

As a demo, it reads real-time temperature and humidity from a Modulino sensor. Each metric occupies a full-screen card (`100vh`), requiring users to use touch scrolling to toggle between views. It serves as an expandable template for lightweight HMI systems using Arduino App Lab.

### Visual Flow (Touch/Swipe Vertical Navigation)

```text
    ▲  [Swipe Up / Down]
    ▼  (Native CSS Snap-Scrolling)

   +--------------------------+
   |                          |
   |       TEMPERATURE        |
   |                          |
   |         24.8°C           | 
   |                          |
   +--------------------------+  ◄--- Card 1 (100vh)
   +--------------------------+  ◄--- Card 2 (100vh)
   |                          |
   |         HUMIDITY         |
   |                          |
   |          54.8%           | 
   |                          |
   +--------------------------+

```

---

# Features

* **Full-screen Kiosk UI:** Clean, large-scale typography optimized for embedded touch displays.
* **Hardware Integration:** Showcases Electronic Cats Adapter Shield (a custom carrier PCB) routing for DSI video output and I²C touch interfaces.
* **Asynchronous Dual-Core Flow:** Seamless RPC (Remote Procedure Call) event notification from the STM32 MCU to the Qualcomm Linux MPU.
* **Ultra-Lightweight Frontend:** Avoids bulky charting libraries or complex CSS wrappers; uses native CSS Snap-Scrolling (`scroll-snap-type`) for card transitions.
* **Web-based UI:** Implemented natively via WebUI - HTML Brick and Python.
* **Compatible with Arduino App Lab.**

---

# Hardware Overview

This demonstration requires the following hardware components:

* Arduino UNO Q
* Electronic Cats Adapter Shield
* Arduino GIGA Display
* MODULINO® Thermo (or equivalent I²C climate sensor)
* USB-C cable for power

The Adapter Shield bridges the high-speed **DSI (Digital Serial Interface)** for video processing and the **I²C bus** required by the display's capacitive touch controller.

---

# System Architecture

```text
                    Arduino UNO Q

       +--------------------------------------+
       | Qualcomm MPU (Debian Linux)          |
       |                                      |
       | Python Backend (main.py)             |
       | WebUI Brick Server                   |
       | HTML Kiosk Dashboard (index.html)    |
       +------------------+-------------------+
                          |
             Bridge Communication / RPC 
                          |
       +------------------+-------------------+
       | STM32 Microcontroller (MCU)          |
       |                                      |
       | Arduino Sketch (sketch.ino)          |
       | Reads MODULINO Thermo via I2C        |
       +------------------+-------------------+
                          |
                  Electronic Cats Adapter Shield
                          |
             Arduino GIGA Display Touch

```

---

# Technologies Used

This project leverages:

* **Arduino App Lab** & WebUI (HTML Brick)
* **Arduino_Modulino** & **Arduino_RouterBridge** libraries
* **Python 3** (Backend Core)
* **HTML5 / CSS3 / JavaScript** (Native Kiosk Frontend)
* **Socket.IO** (Local, real-time WebSocket communication)
* **Arduino Sketch (C++)** (Microcontroller loop)
* **Debian Linux** (Embedded Kernel OS)

### References & Credits

During development, the official **Home Climate Monitoring** example included with Arduino App Lab was used as an architectural baseline to understand the inter-processor communication bridge. The front-end and back-end logic were heavily refactored and streamlined specifically to fulfill this kiosk display proof of concept.

---

# Prerequisites

Before deploying this application, ensure that the underlying custom hardware and hardware abstraction layers are fully configured:

## Hardware

* Electronic Cats Adapter Shield connected to the UNO Q.
* Arduino GIGA Display mounted onto the carrier.
* MODULINO Thermo connected via the Wire1 I²C port.

## Software & Drivers

* Arduino App Lab environment.
* Formatted Debian environment on the UNO Q with the custom touch and display panel drivers pre-loaded.

Component tracking repositories:

| Component | Repository / Resource |
| --- | --- |
| Electronic Cats Adapter Shield | https://github.com/ElectronicCats/tutorials/tree/main/UnoQ-AdapterShield/HW/AdapterShield |
| Linux Display & Touch Drivers | https://github.com/ElectronicCats/tutorials/releases/tag/v1.0_AdapterShield |
| Installation instructions (Linux Display & Touch Drivers) | https://github.com/ElectronicCats/tutorials/tree/main/UnoQ-AdapterShield |


---

# Installation

## Option 1 — Create the application from Arduino App Lab (Recommended)

1. Open **Arduino App Lab**.
2. Create a new empty application project.
3. Add the **WebUI - HTML** Brick block.
4. Replace the source code directories with the following files found in this repository:

```text
assets/libs/socket.io.min.js
assets/index.html

python/main.py

sketch/sketch.ino

```
5. Add the Arduino_Modulino library: App Menu ➔ Add Sketch Library ➔ Type "Arduino_Modulino" in the browser ➔ Click on "Install".
6. Click **Run** to provision the board.

> **Note**
> Redundant template files (like `app.js` and `style.css` from the stock climate example) have been completely removed. The UI is now embedded cleanly into the HTML layout to maximize display render speeds.

---

## Option 2 — Clone this repository

```bash
git clone https://github.com/ElectronicCats/tutorials

```
> **Note**
> You can download the ZIP code as well. Navigate to this repository ➔ Code ➔ Download ZIP.

1. Inside the *tutorials* folder, find the *UnoQ-KioskDemo*. Compress this folder to a ZIP file.
2. Import the ZIP file as an existing project inside your Arduino App Lab instance.
2. Add the Arduino_Modulino library: App Menu ➔ Add Sketch Library ➔ Type "Arduino_Modulino" in the browser ➔ Click on "Install".
3. Click **Run** to provision the board.

---

# Kiosk Configuration Setup

To allow Debian Linux to automatically log in and boot into Chromium full-screen kiosk mode, manually configure the following local files on your Arduino UNO Q terminal.

## 1. Enable System Auto-Login

Open the terminal and modify the LightDM configuration file:

```bash
sudo mkdir /etc/lightdm/lightdm.conf.d
sudo nano /etc/lightdm/lightdm.conf.d/50-autologin.conf

```

Paste the following content, then save and exit (Ctrl+O, Enter, Ctrl+X):

```ini
[Seat:*]
autologin-user=arduino
autologin-user-timeout=0

```

## 2. Configure Chromium Kiosk Autostart

Ensure the autostart directory exists and create the desktop entry file:

```bash
mkdir -p ~/.config/autostart
sudo nano ~/.config/autostart/monitor-kiosko.desktop

```

Paste the following content, then save and exit (Ctrl+O, Enter, Ctrl+X):

```ini
[Desktop Entry]
Type=Application
Name=Monitor Kiosko
Exec=/bin/bash -c 'sleep 8; chromium --kiosk --noerrdialogs --disable-infobars --incognito --check-for-update-interval=31536000 http://127.0.0.1:7000'
X-GNOME-Autostart-enabled=true

```

---

# Configure Automatic Startup

To configure your UNO Q to launch this dashboard immediately upon system startup:

### Method A (GUI)

Inside Arduino App Lab:

```text
App Menu  ➔  Run at startup  ➔  Enable

```

### Method B (CLI Terminal)

Access your UNO Q Linux shell and execute:

```bash
arduino-app-cli app list
arduino-app-cli properties set default <APP_ID>

```

*(Replace `<APP_ID>` with the exact identifier hash generated by the `app list` command).*

---

# Running the Demo

1. Ensure all hardware cables are seated safely.
2. Apply stable power to the Arduino UNO Q via USB-C.
3. Allow Linux to finish its initial initialization scripts.
4. The kiosk server will deploy automatically, launching the full-screen interactive viewport.
5. Swipe/scroll vertically on the screen to alternate between the large-scale **Temperature** and **Humidity** views.

---

# Project Structure

```text
project/
├── assets/
│   ├── libs/
│   │   └── socket.io.min.js
│   └── index.html
├── python/
│   └── main.py
├── sketch/
│   ├── sketch.ino
│   └── sketch.yaml
├── README.md
└── app.yaml

```

---

# Acknowledgements

This ecosystem implementation represents a collaborative engineering effort:

| Contribution | Author / Contributor |
| --- | --- |
| Arduino App Lab Ecosystem | Arduino s.r.l. |
| Home Climate Reference Design | Arduino s.r.l. |
| Adapter Shield Engineering | AndreaZGuz |
| Linux Display Panel Driver | sabas1080/AndreaZGuz |
| Linux Capacitive Touch Driver | sabas1080 |
| Kiosk OS-Level Configurations | sabas1080/victorman-sd |
| Kiosk Dashboard App & Logic Optimization | victorman-sd |

---

# Future Improvements

* Implementation of multi-page layout selectors.
* Integration of industrial fieldbus protocols (Modbus TCP, OPC-UA) on the MCU layer.
* Dark mode adaptive UI adjustments based on photoresistors.
* OTA (Over-the-Air) deployment flows.

---

# License

Please refer to the `LICENSE` file included in this repository.