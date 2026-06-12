#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>

const char* ssid = "Peace And Love Ploclaimers";
const char* password = "loveisthekey";

const char* mqtt_server = "157.173.101.159";
const int mqtt_port = 1883;
const char* client_id = "esp8266_student73";
const char* topic_movement = "vision/student73/movement";
const char* topic_heartbeat = "vision/student73/heartbeat";

// Servo Configuration
Servo myServo;
const int servoPin = 2; // GPIO2 = physical D4 on NodeMCU / ESP8266
int currentAngle = 90;

// --- Search Mode Variables ---
bool isSearching = true;         // Start in search mode by default
unsigned long lastSweepTime = 0;
int sweepStep = 2;

// --- Watchdog Timer Variables ---
unsigned long lastFaceDetectTime = 0;
const unsigned long FACE_TIMEOUT = 2000; // 2 seconds without a face triggers a search

WiFiClient espClient;
PubSubClient client(espClient);

const char* wifiStatusToText(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS: return "IDLE";
    case WL_NO_SSID_AVAIL: return "NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "SCAN_COMPLETED";
    case WL_CONNECTED: return "CONNECTED";
    case WL_CONNECT_FAILED: return "CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "CONNECTION_LOST";
    case WL_DISCONNECTED: return "DISCONNECTED";
    default: return "UNKNOWN";
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.println("=== ESP8266 Vision Servo Boot ===");
  Serial.print("Connecting to WiFi SSID: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    if (millis() - wifiStart > 15000) {
      Serial.println();
      Serial.print("WiFi connect timeout. Status: ");
      Serial.print((int)WiFi.status());
      Serial.print(" (");
      Serial.print(wifiStatusToText(WiFi.status()));
      Serial.println(")");
      Serial.println("Check 2.4GHz WiFi, password, signal, or hotspot availability.");
      wifiStart = millis();
    }
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("ESP IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signal RSSI: ");
  Serial.println(WiFi.RSSI());
}

void moveServo(int delta) {
  int previousAngle = currentAngle;
  currentAngle += delta;
  if (currentAngle < 0) currentAngle = 0;
  if (currentAngle > 180) currentAngle = 180;
  myServo.write(currentAngle);

  Serial.print("Servo move: ");
  Serial.print(previousAngle);
  Serial.print(" -> ");
  Serial.println(currentAngle);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println();
  Serial.print("MQTT message on ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  // Parse the commands and update the Watchdog Timer
  if (message.indexOf("MOVE_LEFT") >= 0) {
    isSearching = false;
    lastFaceDetectTime = millis(); // Reset the timer!
    Serial.println("Command: MOVE_LEFT");
    moveServo(-3);
  }
  else if (message.indexOf("MOVE_RIGHT") >= 0) {
    isSearching = false;
    lastFaceDetectTime = millis(); // Reset the timer!
    Serial.println("Command: MOVE_RIGHT");
    moveServo(3);
  }
  else if (message.indexOf("CENTERED") >= 0) {
    isSearching = false;
    lastFaceDetectTime = millis(); // Reset the timer!
    Serial.println("Command: CENTERED");
  }
  else if (message.indexOf("NO_FACE") >= 0) {
    isSearching = true;  // Explicit command to start searching
    Serial.println("Command: NO_FACE -> search mode enabled");
  } else {
    Serial.println("Command ignored: no known movement keyword found");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println();
    Serial.print("Attempting MQTT connection to ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.println(mqtt_port);
    Serial.print("Using client id: ");
    Serial.println(client_id);

    if (client.connect(client_id)) {
      Serial.println("MQTT connected");
      bool subscribed = client.subscribe(topic_movement);
      Serial.print("Subscribed to ");
      Serial.print(topic_movement);
      Serial.print(" => ");
      Serial.println(subscribed ? "OK" : "FAILED");
    } else {
      Serial.print("MQTT failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.print("Servo signal pin: GPIO");
  Serial.println(servoPin);
  myServo.attach(servoPin);
  myServo.write(currentAngle);
  Serial.print("Initial servo angle: ");
  Serial.println(currentAngle);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  Serial.print("MQTT movement topic: ");
  Serial.println(topic_movement);
  Serial.print("MQTT heartbeat topic: ");
  Serial.println(topic_heartbeat);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();

  // --- WATCHDOG TIMER ---
  // If we aren't currently searching, but it's been more than 2 seconds
  // since we last saw a face, force the system back into search mode.
  if (!isSearching && (now - lastFaceDetectTime > FACE_TIMEOUT)) {
    Serial.println("Face lost! Watchdog triggered. Starting search...");
    isSearching = true;
  }

  // --- NON-BLOCKING SEARCH SWEEP ---
  if (isSearching) {
    if (now - lastSweepTime > 30) {
      lastSweepTime = now;
      currentAngle += sweepStep;

      if (currentAngle >= 180) {
        currentAngle = 180;
        sweepStep = -2;
      } else if (currentAngle <= 0) {
        currentAngle = 0;
        sweepStep = 2;
      }
      myServo.write(currentAngle);
    }
  }

  // --- SYSTEM HEARTBEAT ---
  static unsigned long lastHeartbeat = 0;
  if (now - lastHeartbeat > 5000) {
    lastHeartbeat = now;
    String heartbeat = "{\"node\": \"esp8266\", \"status\": \"ONLINE\"}";
    bool heartbeatOk = client.publish(topic_heartbeat, heartbeat.c_str());
    Serial.print("Heartbeat publish: ");
    Serial.println(heartbeatOk ? "OK" : "FAILED");
  }
}