#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

const int TRIG_PINS[] = {1, 42, 40, 38, 36, 45, 47, 20, 4, 6, 15, 17, 8, 46, 10, 11, 13};
const int ECHO_PINS[] = {2, 41, 39, 37, 35, 0, 48, 21, 19, 5, 7, 16, 18, 3, 9, 12, 14};

const int NUM_SENSORS = sizeof(TRIG_PINS) / sizeof(TRIG_PINS[0]);

// WiFi credentials
const char *ssid = "Wokwi-GUEST"; // Replace with your SSID
const char *password = ""; // Replace with your password

// URL for sending JSON data
const char *url = "https://service-proposal.vercel.app/update-status-iot-s3"; // Replace with your URL

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Set all TRIG pins as OUTPUT and ECHO pins as INPUT
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(TRIG_PINS[i], OUTPUT);
    pinMode(ECHO_PINS[i], INPUT);
  }
}

// Function to measure distance
long measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  return duration * 0.034 / 2; // Convert to centimeters
}

// Function to send JSON data via POST request
void sendJsonData(int statusArray[]) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setReuse(true); // Aktifkan keep-alive
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // Construct JSON string manually
    String jsonString = "{ \"statusArray\": [";
    for (int i = 0; i < NUM_SENSORS; i++) {
      jsonString += String(statusArray[i]);
      if (i < NUM_SENSORS - 1) {
        jsonString += ", "; // Add a comma between values
      }
    }
    jsonString += "] }";

    // Send the POST request
    int httpResponseCode = http.POST(jsonString);

    // Check the response
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response Code: " + String(httpResponseCode));
      Serial.println("Response Body: " + response);
    } else {
      Serial.println("Error on sending POST: " + String(httpResponseCode));
    }

    // Close connection
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

void loop() {
  int statusArray[NUM_SENSORS];

  // Read and store data for each sensor
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    long distance = measureDistance(TRIG_PINS[i], ECHO_PINS[i]);

    if (distance == 0 || distance > 400)
    {
      statusArray[i] = 2; // Sensor tidak terhubung
    }
    else
    {
      statusArray[i] = (distance < 5) ? 1 : 0; // 1 jika < 5cm, 0 jika >= 5cm
    }
  }

  // Print the JSON-like structure to Serial
  String jsonOutput = "{\n    \"statusArray\": [";
  for (int i = 0; i < NUM_SENSORS; i++) {
    jsonOutput += String(statusArray[i]);
    if (i < NUM_SENSORS - 1) {
      jsonOutput += ", "; // Add a comma between values
    }
  }
  jsonOutput += "]\n}";

  Serial.println(jsonOutput); // Print the complete JSON structure

  // Send the status array as JSON to the specified URL
  sendJsonData(statusArray);

  delay(1000); // Delay between readings
}