# KneeGuard
IoT-enabled smart knee brace for real-time patellar tracking, vibration analysis, and rehabilitation monitoring.

# Real-Time Wearable Smart System for Detecting Patellar Disorientation and Enhancing Patellar Stability

## Overview

This project presents a wearable smart rehabilitation system designed to monitor knee biomechanics in real time and assist in the early detection of patellar instability.

The system combines:

* ESP32 WROOM microcontroller
* MPU6050 Inertial Measurement Unit (IMU)
* PVDF Piezoelectric Film Sensor
* Custom 3D-Printed Knee Brace
* Real-Time Web Monitoring Application

Sensor data is collected directly from the knee brace, processed by the ESP32, and transmitted wirelessly to a web application that visualizes knee movement and vibration patterns. The system alerts users whenever abnormal patellar vibrations or unsafe knee movements are detected.

---

## Features

* Real-time knee angle monitoring
* Patellar vibration measurement using PVDF sensor
* Wireless data transmission over Wi-Fi
* User-specific calibration
* Automatic threshold generation
* Real-time dashboard visualization
* Abnormal vibration alerts
* Lightweight 3D-printed brace integration

---

## Hardware Components

| Component               | Purpose                                 |
| ----------------------- | --------------------------------------- |
| ESP32 WROOM             | Data processing and Wi-Fi communication |
| MPU6050                 | Knee angle measurement                  |
| PVDF Film Sensor        | Patellar vibration detection            |
| 4.7kΩ Resistors         | I2C pull-up resistors                   |
| Custom 3D Printed Brace | Sensor housing and stabilization        |

---

## System Architecture

PVDF Sensor + MPU6050
↓
ESP32 WROOM
↓ Wi-Fi
Web Application
↓
Calibration + Monitoring + Alerts

---

## Working Principle

1. The MPU6050 continuously measures knee flexion and extension angles.
2. The PVDF sensor captures vibration signatures around the patellar region.
3. ESP32 processes the incoming sensor data.
4. Data is transmitted wirelessly to the web application.
5. The application performs:

   * Calibration
   * Threshold mapping
   * Real-time visualization
   * Alert generation
6. Users receive warnings when vibration or angle values exceed safe limits.

---

## Repository Structure

```text
ESP32_Firmware/
WebApp/
Images/
3D_Model/
Documentation/
```

---

## Installation

### ESP32 Firmware

1. Open Arduino IDE.
2. Install ESP32 board support.
3. Install required libraries:

   * Wire.h
   * MPU6050 library
4. Update Wi-Fi credentials.
5. Upload firmware to ESP32.

### Web Application

1. Open the WebApp folder.
2. Launch `index.html`.
3. Connect the ESP32 device.
4. Start monitoring.

---

## Results

The developed system demonstrated:

* Reduced vibration spikes when the brace was worn
* Improved knee movement stability
* Real-time monitoring capability
* Instant abnormal vibration alerts

These findings indicate that the system can assist in monitoring patellar alignment and rehabilitation progress.

---

## Future Improvements

* Machine Learning based instability prediction
* Mobile application support
* Cloud-based monitoring
* Long-term patient analytics
* Automated rehabilitation recommendations

---

## Authors

* Harinisri Ramesh
* Maanasvini Anand
* Kayalvizhi M

Department of Biomedical Engineering
SSN College of Engineering

---

## License

This project is licensed under the MIT License.

