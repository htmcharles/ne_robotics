# Distributed Vision-Control System (Face-Locked Servo)

## 1. Integrated Situation

### 1.1 Project Description

BENAX Technologies Ltd is developing an AI-powered camera control system for live presentations, lectures, performances, and video-conferencing environments. The goal is to automatically follow one designated speaker in real time.

Unlike generic face-tracking systems that react to any visible face, this system is designed to lock onto one pre-enrolled identity and continue tracking that same person even when other faces appear in the frame.

This project integrates computer vision, machine learning, MQTT networking, embedded control, and servo actuation into one working end-to-end system.

### 1.2 Actual System Configuration Used in This Project

- Windows project Python: `venv310`
- Python version: `3.10.9`
- Vision node runtime: `C:\NE\robotics\Face_recognition_with_Arcface\venv310\Scripts\python.exe`
- MQTT namespace: `vision/student73`
- ESP8266 servo signal pin: physical `D4`, coded as `GPIO2`
- Backend dashboard port: `8080`
- Backend WebSocket port: `9002`

## 2. System Overview

### 2.1 High-Level Operation

1. A PC captures live video from a camera.
2. The vision pipeline detects faces, extracts embeddings, and identifies the enrolled speaker.
3. If the enrolled speaker is present, the system computes horizontal tracking error.
4. Motion commands are published over MQTT.
5. An ESP8266 subscribes to the MQTT topic and drives a servo motor.
6. A Node.js backend relays MQTT movement events to a browser dashboard through WebSocket.

### 2.2 Main Components

- **Vision Node (PC)**: face detection, recognition, identity lock, tracking, command publishing
- **MQTT Broker**: message transport between the PC and ESP8266/backend
- **ESP8266 Controller**: subscribes to movement commands and controls the servo
- **Servo Motor**: rotates the camera horizontally
- **Web Dashboard**: displays live status, lock state, target, and snapshot

## 3. Implemented Features

### 3.1 Face Enrollment

The system supports enrollment of a target person so a reusable facial profile can be saved locally for later identification.

Implemented by:

- `src/enroll.py`
- `data/db/face_db.npz`

### 3.2 Single-Identity Face Recognition

The system compares live embeddings against the stored face database and tracks only the intended person. Other faces are ignored even if they are visible in the frame.

Implemented by:

- `src/recognize.py`
- `src/vision_node.py`
- `src/face_locking.py`

### 3.3 Face Lock and Re-Acquisition

Once the target is found, the system enters a locked tracking mode. If the target disappears, the system publishes `NO_FACE` and the embedded controller falls back into search behavior until the target is seen again.

### 3.4 Tracking and Command Generation

The face center is compared with the frame center. A deadband is used to reduce jitter:

- `MOVE_LEFT`
- `MOVE_RIGHT`
- `CENTERED`
- `NO_FACE`

### 3.5 MQTT-Based Embedded Motor Control

The PC publishes tracking commands and heartbeat messages through MQTT. The ESP8266 subscribes to the movement topic and adjusts the servo position.

Actual configured topics:

- `vision/student73/movement`
- `vision/student73/heartbeat`

### 3.6 Dashboard Monitoring

The backend receives MQTT movement messages and forwards them to the dashboard using WebSocket. The dashboard visualizes:

- connection state
- tracking state
- active target
- confidence
- latest face snapshot
- event log

### 3.7 Logging and Evidence

The system records operational evidence through:

- live MQTT payloads
- dashboard event console
- action history files from face locking
- backend console output

## 4. Hardware Requirements

### 4.1 Vision System

- USB camera

Used for:

- real-time face detection
- face recognition
- face tracking

### 4.2 Motion and Actuation

- SG90 or equivalent servo motor

Used for:

- horizontal rotation
- camera positioning

### 4.3 Embedded Controller

- ESP8266 / NodeMCU

Used for:

- MQTT subscription
- command parsing
- servo signal generation

### 4.4 Servo Wiring Detail

For this project configuration:

- Servo signal pin: physical `D4`
- Arduino code pin value: `2` (`GPIO2`)
- Servo power and grounding must be connected correctly for stable operation

### 4.5 Computing Platform

- Desktop or laptop

Used for:

- running the AI pipeline
- sending MQTT commands
- serving the monitoring dashboard

## 5. Software Requirements

### 5.1 Python Environment

- Python `3.10.9`
- Virtual environment: `venv310`

Recommended interpreter:

```powershell
.\venv310\Scripts\python.exe
```

### 5.2 Python Libraries

- OpenCV
- NumPy
- `paho-mqtt`
- ONNX Runtime
- MediaPipe-related facial landmark processing used by the project modules

### 5.3 Node.js Backend

- Node.js
- `mqtt`
- `ws`

### 5.4 Embedded Library

For the Arduino ESP8266 sketch:

- `PubSubClient`
- `Servo`
- `ESP8266WiFi`

### 5.5 MQTT Broker

Example broker options:

- local Mosquitto
- VPS-hosted Mosquitto

## 6. Actual Project Structure

```text
Face_recognition_with_Arcface/
├── src/
│   ├── vision_node.py
│   ├── face_locking.py
│   ├── recognize.py
│   ├── haar_5pt.py
│   ├── detect.py
│   ├── align.py
│   ├── embed.py
│   └── enroll.py
├── backend/
│   ├── server.js
│   └── package.json
├── dashboard/
│   └── index.html
├── esp8266/
│   ├── main.py
│   ├── boot.py
│   └── vision_servo/
│       ├── vision_servo.ino
│       └── new.ino
├── data/
│   └── db/
│       └── face_db.npz
├── models/
│   └── embedder_arcface.onnx
├── docs/
│   └── diagrams/
└── README.md
```

## 7. Implementation Procedure

### 7.1 Enrollment

1. Capture multiple images of the target person.
2. Generate an embedding template.
3. Save the face database locally.

Example:

```powershell
.\venv310\Scripts\python.exe -m src.enroll --name hatuma
```

### 7.2 Recognition and Lock

1. Detect faces in the live frame.
2. Extract embeddings.
3. Compare against enrolled identities.
4. Select only the authorized target.
5. Maintain lock while the target remains visible.

### 7.3 Tracking and Motion Command Pipeline

1. Determine the horizontal center of the target face.
2. Compare it with the frame center.
3. Apply deadband logic.
4. Publish movement status through MQTT.

### 7.4 Embedded Motor Control

1. ESP8266 connects to Wi-Fi.
2. ESP8266 subscribes to `vision/student73/movement`.
3. Incoming messages are parsed.
4. Servo angle is adjusted left or right.
5. If `NO_FACE` persists, the controller enters a sweep/search pattern.

### 7.5 Monitoring and Validation

1. Backend subscribes to the movement topic.
2. Backend relays updates to the browser through WebSocket.
3. Dashboard shows live state and snapshot evidence.

## 8. Windows Run Instructions

### 8.1 Backend

```powershell
cd backend
npm start
```

### 8.2 Vision Node Using VPS Broker

```powershell
$env:MPLCONFIGDIR="$PWD\\.tmp\\matplotlib"
New-Item -ItemType Directory -Force -Path $env:MPLCONFIGDIR | Out-Null
.\venv310\Scripts\python.exe src\vision_node.py --broker 157.173.101.159 --name hatuma
```

### 8.3 Vision Node Using Local Broker

```powershell
$env:MPLCONFIGDIR="$PWD\\.tmp\\matplotlib"
New-Item -ItemType Directory -Force -Path $env:MPLCONFIGDIR | Out-Null
.\venv310\Scripts\python.exe src\vision_node.py --broker 10.206.87.243 --name hatuma
```

### 8.4 Dashboard

Open:

```text
http://localhost:8080
```

## 9. MQTT Topics

- `vision/student73/movement`
- `vision/student73/heartbeat`

## 10. Validation Scenarios

The system should be demonstrated under the following conditions:

1. Only the enrolled speaker is visible
2. Multiple faces are visible
3. The speaker moves left and right
4. The speaker becomes temporarily occluded
5. The speaker leaves the frame and re-enters

Expected behavior:

- only the enrolled speaker is tracked
- other faces are ignored
- servo follows horizontal motion
- search mode activates when the target is lost
- lock is re-established when the target returns

## 11. Diagrams and Documentation Assets

The documentation diagram files are stored in:

- [docs/flow-diagram.md](C:\NE\robotics\Face_recognition_with_Arcface\docs\flow-diagram.md)
- [docs/system-architecture.md](C:\NE\robotics\Face_recognition_with_Arcface\docs\system-architecture.md)

## 12. Notes to Assessors

### Time Allowed

- 6 hours

### Materials

- FalconEye V1 HD Camera Board
- 2-DOF mechanism
- Servo motor
- ESP8266
- Micro-USB cables
- Male-to-female jumper wires
- USB hub

## 13. Conclusion

This project delivers a working distributed face-locked servo tracking system that combines:

- ArcFace-based recognition
- identity-specific lock logic
- MQTT communication
- ESP8266 servo control
- dashboard monitoring
- Windows-based Python `3.10.9` execution in `venv310`

The system is configured for the isolated namespace `vision/student73` and uses servo signal pin `D4` on the board, represented in Arduino code as `GPIO2`, matching the current implementation.
