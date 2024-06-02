# Embedded Systems and Applications Programming Assignments

## Assignment 1: Medibox Simulation

### Project Overview

This project involves simulating a Medibox using an ESP32 microcontroller. The Medibox is designed to help users effectively manage their medication schedules. Key features include setting alarms, fetching time from an NTP server, monitoring temperature and humidity, and providing warnings.

### Requirements

#### Fetching Current Time and Displaying on OLED

- **Fetches time from an NTP server.**
- **Displays current time on an OLED screen.**

#### Menu

- **Allows user to set time zone, set 3 alarms, and disable all alarms.**

#### Alarms

- **Rings alarms at set times with proper indications.**
- **Stops alarms using a push button.**

#### Monitoring

- **Monitors temperature and humidity.**
- **Provides warnings if temperature or humidity exceed healthy limits.**

## Assignment 2: Medibox Enhancement

### Project Overview

This assignment focuses on enhancing the Medibox by adding features such as light intensity monitoring, dynamic regulation of light using a shaded sliding window controlled by a servo motor, and user-adjustable parameters via a Node-RED dashboard.

### Requirements

#### Light Intensity Monitoring

- **Uses two LDRs to monitor light intensity.**
- **Sends data to Node-RED via MQTT.**

#### Shaded Sliding Window

- **Uses a servo motor to adjust the window.**
- **Dynamically regulates light based on a given equation.**

#### Node-RED Dashboard

- **Visualizes light intensity with a plot and gauge.**
- **Sets up sliders and dropdowns for user-adjustable parameters.**

