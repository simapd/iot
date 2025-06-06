// Basic demo for accelerometer readings from Adafruit MPU6050 with MQTT

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

// WiFi Configuration
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT Configuration
const char* mqtt_server = "172.190.255.213";
const int mqtt_port = 1883;
const char* mqtt_user = "";
const char* mqtt_password = "";

const char* TOPIC_MOVEMENT = "iot/measurement/movement";
const char* TOPIC_RAIN = "iot/measurement/rain";
const char* TOPIC_MOISTURE = "iot/measurement/moisture";

const String AREA_ID = "b138ns1z29jrop4q35o51cao";
const String SENSOR_ID = "j6nmu6qwjgo6h7k29ababv02";
const String API_BASE_URL = "http://172.190.255.213:1880";

#define RAIN_ANALOG 35
#define SOIL_MOISTURE_ANALOG 32

Adafruit_MPU6050 mpu;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

enum SensorType {
  MOVEMENT,
  RAIN,
  SOIL_MOISTURE
};

enum MovementLevel {
  MOVEMENT_NONE,
  MOVEMENT_LOW,
  MOVEMENT_MEDIUM,
  MOVEMENT_HIGH,
  MOVEMENT_CRITICAL
};

enum RainLevel {
  RAIN_NONE,
  RAIN_LOW,
  RAIN_MEDIUM,
  RAIN_HIGH,
  RAIN_CRITICAL
};

enum SoilMoistureLevel {
  MOISTURE_NONE,
  MOISTURE_LOW,
  MOISTURE_MEDIUM,
  MOISTURE_HIGH,
  MOISTURE_CRITICAL
};

enum RiskLevel {
  RISK_LOW,
  RISK_MEDIUM,
  RISK_HIGH,
  RISK_CRITICAL
};

double GRAVITY_ACC = 9.8;
double DETECTION_DURATION_MS = 200;

double DETECTION_THRESHOLD = 0.01 * GRAVITY_ACC;
double GYRO_THRESHOLD = 0.2;
unsigned long movementStartTime = 0;
bool movementOngoing = false;
bool movementDetected = false;
MovementLevel lastMovementLevel;

unsigned long rainStartTime = 0;
bool rainOngoing = false;
bool rainDetected = false;
RainLevel lastRainLevel;

unsigned long moistureStartTime = 0;
bool moistureOngoing = false;
bool moistureDetected = false;
SoilMoistureLevel lastSoilMoistureLevel;

unsigned long lastTime;

bool sensorValidated = false;

struct MovementData {
  double accelX, accelY, accelZ;
  double gyroX, gyroY, gyroZ;
  double accelMagnitude;
  double gyroMagnitude;
  MovementLevel movementLevel;
};

struct RainData {
  RainLevel rainLevel;
  unsigned int sensorValue;
};

struct MoistureData {
  SoilMoistureLevel soilMoistureLevel;
  unsigned int sensorValue;
};

RiskLevel calculateRiskLevel(SensorType type, void* data) {
  switch(type) {
    case MOVEMENT: {
      MovementData* m = (MovementData*)data;
      switch(m->movementLevel) {
        case MOVEMENT_LOW: return RISK_LOW;
        case MOVEMENT_MEDIUM: return RISK_MEDIUM;
        case MOVEMENT_HIGH: return RISK_HIGH;
        case MOVEMENT_CRITICAL: return RISK_CRITICAL;
        default: return RISK_LOW;
      }
    }
    case RAIN: {
      RainData* r = (RainData*)data;
      switch(r->rainLevel) {
        case RAIN_LOW: return RISK_LOW;
        case RAIN_MEDIUM: return RISK_MEDIUM;
        case RAIN_HIGH: return RISK_HIGH;
        case RAIN_CRITICAL: return RISK_CRITICAL;
        default: return RISK_LOW;
      }
    }
    case SOIL_MOISTURE: {
      MoistureData* sm = (MoistureData*)data;
      switch(sm->soilMoistureLevel) {
        case MOISTURE_LOW: return RISK_LOW;
        case MOISTURE_MEDIUM: return RISK_MEDIUM;
        case MOISTURE_HIGH: return RISK_HIGH;
        case MOISTURE_CRITICAL: return RISK_CRITICAL;
        default: return RISK_LOW;
      }
    }
    default:
      return RISK_LOW;
  }
}

StaticJsonDocument<512> getMqttPayload(SensorType type, void* data) {
  StaticJsonDocument<512> doc;
  doc["sensorId"] = SENSOR_ID;
  doc["areaId"] = AREA_ID;
  doc["type"] = sensorTypeToString(type);

  switch(type) {
    case MOVEMENT: {
      MovementData* m = (MovementData*)data;
      JsonObject value = doc.createNestedObject("value");

      JsonObject accel = value.createNestedObject("acceleration");
      accel["x"] = m->accelX;
      accel["y"] = m->accelY;
      accel["z"] = m->accelZ;
      accel["magnitude"] = m->accelMagnitude;

      JsonObject gyro = value.createNestedObject("rotation");
      gyro["x"] = m->gyroX;
      gyro["y"] = m->gyroY;
      gyro["z"] = m->gyroZ;
      gyro["magnitude"] = m->gyroMagnitude;

      break;
    }
    case RAIN: {
      RainData* r = (RainData*)data;
      JsonObject value = doc.createNestedObject("value");
      value["rainLevel"] = r->sensorValue;
      break;
    }
    case SOIL_MOISTURE: {
      MoistureData* sm = (MoistureData*)data;
      JsonObject value = doc.createNestedObject("value");
      value["moistureLevel"] = sm->sensorValue;
      break;
    }
    default:
      doc["value"] = "Unknown sensor type";
  }

  RiskLevel riskLevel = calculateRiskLevel(type, data);
  doc["riskLevel"] = riskLevelToString(riskLevel);

  return doc;
}

const char* sensorTypeToString(SensorType type) {
  switch(type) {
    case MOVEMENT: return "MOVEMENT";
    case RAIN: return "RAIN";
    case SOIL_MOISTURE: return "SOIL_MOISTURE";
    default: return "UNKNOWN";
  }
}

const char* rainLevelToString(RainLevel rainLevel) {
  switch(rainLevel) {
    case RAIN_LOW: return "LOW";
    case RAIN_MEDIUM: return "MEDIUM";
    case RAIN_HIGH: return "HIGH";
    case RAIN_CRITICAL: return "CRITICAL";
    default: return "UNKNOWN";
  }
}

const char* movementLevelToString(MovementLevel movementLevel) {
  switch(movementLevel) {
    case MOVEMENT_LOW: return "LOW";
    case MOVEMENT_MEDIUM: return "MEDIUM";
    case MOVEMENT_HIGH: return "HIGH";
    case MOVEMENT_CRITICAL: return "CRITICAL";
    default: return "UNKNOWN";
  }
}

const char* soilMoistureLevelToString(SoilMoistureLevel soilMoistureLevel) {
  switch(soilMoistureLevel) {
    case MOISTURE_LOW: return "LOW";
    case MOISTURE_MEDIUM: return "MEDIUM";
    case MOISTURE_HIGH: return "HIGH";
    case MOISTURE_CRITICAL: return "CRITICAL";
    default: return "UNKNOWN";
  }
}

const char* riskLevelToString(RiskLevel riskLevel) {
  switch(riskLevel) {
    case RISK_LOW: return "LOW";
    case RISK_MEDIUM: return "MEDIUM";
    case RISK_HIGH: return "HIGH";
    case RISK_CRITICAL: return "CRITICAL";
    default: return "LOW";
  }
}

bool validateSensor() {
  HTTPClient http;
  
  String url = API_BASE_URL + "/sensors/validate" + "?" + "areaId=" + AREA_ID + "&sensorId=" + SENSOR_ID;
  
  Serial.println("Validating sensor with API...");
  Serial.println("URL: " + url);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("ngrok-skip-browser-warning", "true");
  http.addHeader("User-Agent", "ESP32-Sensor/1.0");
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("Sensor validated successfully!");
    Serial.println("Response: " + response);
    http.end();
    return true;
  } else {
    Serial.print("Error validating sensor. Code: ");
    Serial.println(httpResponseCode);
    if (httpResponseCode == 404) {
      Serial.println("ERROR: Sensor not found in the specified area!");
      Serial.println("Check if:");
      Serial.println("- Area ID is correct: " + AREA_ID);
      Serial.println("- Sensor ID is correct: " + SENSOR_ID);
      Serial.println("- Sensor is registered in this specific area");
    } else {
      Serial.println("ERROR: Failed to communicate with API!");
    }
    http.end();
    return false;
  }
}

void connectWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Criar um ID único para o cliente
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    // Tentar conectar
    if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void publishMQTT(const char* topic, StaticJsonDocument<512>& payload) {
  if (!mqttClient.connected()) {
    connectMQTT();
  }

  if (payload.overflowed()) {
    Serial.println("ERROR: JSON document buffer overflow! Message too large.");
    Serial.print("Topic: ");
    Serial.println(topic);
    return;
  }
  
  String jsonString;
  serializeJson(payload, jsonString);
  
  Serial.print("Publishing to topic: ");
  Serial.println(topic);
  Serial.print("Payload: ");
  Serial.println(jsonString);
  
  if (mqttClient.publish(topic, jsonString.c_str())) {
    Serial.println("Message published successfully");
  } else {
    Serial.println("Failed to publish message");
  }
}

void setup(void) {
  Serial.begin(115200);
  
  Serial.println("=== STARTING ESP32 SENSOR ===");
  Serial.println("Area ID: " + AREA_ID);
  Serial.println("Sensor ID: " + SENSOR_ID);
  
  connectWiFi();
  
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setBufferSize(1024);
  
  if (!validateSensor()) {
    Serial.println("=== STOPPING EXECUTION ===");
    Serial.println("Sensor was not validated. Please check:");
    Serial.println("1. If the area exists in the system");
    Serial.println("2. If the sensor is registered in this specific area");
    Serial.println("3. If both AREA_ID and SENSOR_ID are correct");
    Serial.println("4. If the API is accessible");
    
    while(true) {
      delay(10000);
      Serial.println("ESP32 stopped. Sensor not validated.");
    }
  }
  
  Serial.println("=== SENSOR VALIDATED ===");
  Serial.println("Sensor belongs to area: " + AREA_ID);
  Serial.println("Starting sensors...");
  
  connectMQTT();
  
  sensorValidated = true;
  
  setupMovementSensor();
  setupRainSensor();
  setupSoilMoistureSensor();
  
  Serial.println("=== SYSTEM READY ===");
}

void setupMovementSensor() {
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    return;
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
}

void setupRainSensor() {
  pinMode(RAIN_ANALOG, INPUT);
  analogReadResolution(10);
}

void setupSoilMoistureSensor() {
  pinMode(SOIL_MOISTURE_ANALOG, INPUT);
  analogReadResolution(10);
}

void loop() {
  if (!sensorValidated) {
    return;
  }
  
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();
  
  detectMovement();
  detectRain();
  detectSoilMoisture();
}

MovementLevel classifyMovement(double accelMagnitude, double gyroMagnitude) {
  if (accelMagnitude < 0.01 * GRAVITY_ACC && gyroMagnitude < 0.2) {
    return MOVEMENT_LOW;
  } else if (accelMagnitude < 0.03 * GRAVITY_ACC && gyroMagnitude < 0.5) {
    return MOVEMENT_MEDIUM;
  } else if (accelMagnitude < 0.1 * GRAVITY_ACC && gyroMagnitude < 1.0) {
    return MOVEMENT_HIGH;
  } else {
    return MOVEMENT_CRITICAL;
  }
}

void detectMovement() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  double accelMagnitude = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));
  double gyroMagnitude = sqrt(pow(g.gyro.x, 2) + pow(g.gyro.y, 2) + pow(g.gyro.z, 2));

  bool isMoving = accelMagnitude >= DETECTION_THRESHOLD && gyroMagnitude > GYRO_THRESHOLD;

  unsigned long currentTime = millis();

  if (isMoving) {
    MovementLevel movementLevel = classifyMovement(accelMagnitude, gyroMagnitude);

    if (movementLevel != lastMovementLevel) {
      movementDetected = false;
    }

    if (!movementOngoing){
      movementStartTime = currentTime;
      movementOngoing = true;
    } else if (!movementDetected && (currentTime - movementStartTime >= DETECTION_DURATION_MS)){  
      MovementData movementData = {
        a.acceleration.x, a.acceleration.y, a.acceleration.z,
        g.gyro.x, g.gyro.y, g.gyro.z,
        accelMagnitude, gyroMagnitude,
        movementLevel
      };

      StaticJsonDocument<512> payload = getMqttPayload(MOVEMENT, &movementData);
      publishMQTT(TOPIC_MOVEMENT, payload);
      movementDetected = true;
    }
    
    lastMovementLevel = movementLevel;
  }  else {
    movementOngoing = false;
    movementDetected = false;
    lastMovementLevel = MOVEMENT_NONE;
  }
}

void detectRain() {
  unsigned int rainReading = analogRead(RAIN_ANALOG);

  if (rainReading <= 150) {
    lastRainLevel = RAIN_NONE;
    rainOngoing = false;
    rainDetected = false;
    return;
  }

  bool isRaining = true;
  RainLevel rainLevel;
  if (rainReading > 900) {
    rainLevel = RAIN_CRITICAL;
  } else if(rainReading > 700) {
    rainLevel = RAIN_HIGH;
  } else if(rainReading > 400) {
    rainLevel = RAIN_MEDIUM;
  } else if(rainReading > 150) {
    rainLevel = RAIN_LOW;
  }

  unsigned long currentTime = millis();

  if (isRaining) {
    if (rainLevel != lastRainLevel) {
      rainDetected = false;
    }

    if (!rainOngoing){
      rainStartTime = currentTime;
      rainOngoing = true;
    } else if (!rainDetected && (currentTime - rainStartTime >= DETECTION_DURATION_MS)){
      RainData rainData = {
        rainLevel,
        rainReading
      };

      StaticJsonDocument<512> payload = getMqttPayload(RAIN, &rainData);
      publishMQTT(TOPIC_RAIN, payload);
      rainDetected = true;
      lastRainLevel = rainLevel;
    }
  }  else {
    rainOngoing = false;
    rainDetected = false;
    lastRainLevel = RAIN_NONE;
  }
}

void detectSoilMoisture() {
  unsigned int moistureReading = analogRead(SOIL_MOISTURE_ANALOG);

  if (moistureReading <= 150) {
    lastSoilMoistureLevel = MOISTURE_NONE;
    moistureOngoing = false;
    moistureDetected = false;
    return;
  }

  bool isMoistured = true;
  SoilMoistureLevel soilMoistureLevel;
  if (moistureReading > 900) {
    soilMoistureLevel = MOISTURE_CRITICAL;
  } else if(moistureReading > 700) {
    soilMoistureLevel = MOISTURE_HIGH;
  } else if(moistureReading > 400) {
    soilMoistureLevel = MOISTURE_MEDIUM;
  } else if(moistureReading > 150) {
    soilMoistureLevel = MOISTURE_LOW;
  }

  unsigned long currentTime = millis();

  if (isMoistured) {
    if (soilMoistureLevel != lastSoilMoistureLevel) {
      moistureDetected = false;
    }

    if (!moistureOngoing){
      moistureStartTime = currentTime;
      moistureOngoing = true;
    } else if (!moistureDetected && (currentTime - moistureStartTime >= DETECTION_DURATION_MS)){
      MoistureData moistureData = {
        soilMoistureLevel,
        moistureReading
      };

      StaticJsonDocument<512> payload = getMqttPayload(SOIL_MOISTURE, &moistureData);
      publishMQTT(TOPIC_MOISTURE, payload);
      moistureDetected = true;
      lastSoilMoistureLevel = soilMoistureLevel;
    }
  }  else {
    moistureOngoing = false;
    moistureDetected = false;
    lastSoilMoistureLevel = MOISTURE_NONE;
  }
} 
