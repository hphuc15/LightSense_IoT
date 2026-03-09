# 2SL Incoming

`2SL_Incoming` is the data acquisition project of Sensor Lab.

The system measures **ambient light intensity** using a **BH1750 light sensor** and sends the collected data to a backend server over **Wi-Fi using HTTP**.

This repository contains firmware, hardware design, documentation, and database resources required for the **incoming data pipeline** of the Sensor Lab system.

---

# System Overview

The device performs the following workflow:

1. Measure ambient light intensity using the **BH1750 sensor**
2. Connect to a Wi-Fi network
3. Send sensor data to the backend server via HTTP
4. Store and process the data in the database

## Architecture

![System Architecture](https://github.com/hphuc15/2SL_Incoming/docs/images/system-architecture.png)
---

# Repository Structure

```
2SL_Incoming
├── firmware/      # Embedded firmware for the sensor node
├── hardware/      # PCB design and schematic files
├── docs/          # Project documents and datasheets
├── database/      # Database schema and related resources
└── README.md
```

---

# Firmware

Contains the embedded software running on the **ESP32**.

Responsibilities:

* Reading data from the **BH1750 light sensor**
* Managing Wi-Fi connectivity
* Sending HTTP requests to the backend server
* Handling device-level logic

The firmware uses the Wi-Fi management component:

* https://github.com/hphuc15/WiFiManager

---

# Hardware

Contains the hardware design of the sensor node.

The PCB integrates the following components:

* **ESP32 SoC**
* **BH1750 sensor**

The board is responsible for:

* Measuring ambient light intensity
* Processing sensor data on the ESP32
* Sending data to the backend server via Wi-Fi

Hardware resources include: 

* PCB layout
* Schematic diagrams
* Hardware integration documentation

---

# Documentation

The `docs` directory contains supporting documentation related to the project.

Included documents:

**Sensor Lab - Đề tài đầu vào nhóm IoT.pdf**

Project specification describing the requirements and objectives of the Sensor Lab incoming data system.

**bh1750fvi-e-186247.pdf**

Official datasheet of the **BH1750 digital light intensity sensor**.

**window-mariadb-problem.md**

Technical documentation describing an issue encountered when configuring **MariaDB remote client access** during backend development.

The document explains how to enable remote connections by adjusting:

* `bind-address`
* `skip-networking`
* user privileges (`GRANT`)
* firewall configuration for port `3306`

---

# Backend Server

Sensor data is sent to a backend server implemented using the **Drogon web framework**.

Demo server repository:

https://github.com/hphuc15/drogon-server-2sl

The backend server is responsible for:

* Receiving sensor data via HTTP
* Processing incoming requests
* Storing data into the database

---

# Development Notes

The firmware follows a **modular architecture** that separates responsibilities into different components:

* Sensor driver
* Wi-Fi management
* Communication layer
* Application logic

The repository follows the **Conventional Commits** specification for commit messages.

---

# Purpose

This repository serves as the **incoming data module of Sensor Lab**, providing a complete pipeline from:

```
Sensor measurement → Network transmission → Backend ingestion
```

The project is intended for:

* Embedded systems experimentation
* IoT data acquisition
* Sensor network development
