/**
 * Remote Controller for a Crawler Robot Equipped with a Dot-Matrix Display  
 * 
 * This application controls a crawler robot using ESP-NOW wireless communication.
 * The M5Core2 device serves as a remote controller with a touch-based directional
 * interface displayed on the screen. Touch inputs are converted to direction commands
 * and transmitted to the crawler via ESP-NOW protocol.
 * 
 * Features:
 * - Touch-based directional control (Forward, Backward, Left, Right)
 * 
 * Hardware Requirements:
 * - M5Stack Core2
 */

#include <esp_now.h>
#include <WiFi.h>
#include "constants.h"

/**
 * MAC address of the receiver device (crawler robot)
 */
uint8_t broadcastAddress[] = {0x98, 0xA3, 0x16, 0x61, 0x19, 0xA8};

/**
 * ESP-NOW Message Structure
 */
typedef struct MessageStruct {
  uint8_t direction;
} MessageStruct;

/**
 * ESP-NOW peer information structure
 */
esp_now_peer_info_t peerInfo;

/**
 * Last direction output to serial (for duplicate prevention)
 */
uint8_t lastDirection = DIRECTION_STOP;

/**
 * Callback function invoked when ESP-NOW data transmission completes
 * 
 * @param mac_addr MAC address of the receiver device (6 bytes)
 * @param status Transmission status (ESP_NOW_SEND_SUCCESS or ESP_NOW_SEND_FAIL)
 */
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("ESP-NOW delivery success");
  } else {
    Serial.println("ESP-NOW delivery failed");
  }
}

/**
 * Sends direction command to the crawler via ESP-NOW
 * 
 * @param direction Direction command value (use DIRECTION_* constants)
 *                  0 = Stop, 1 = Forward, 2 = Backward, 3 = Left, 4 = Right
 */
void sendData(uint8_t direction) {
  // Prepare message structure with direction command
  MessageStruct messageData = {
      .direction = direction,
  };

  // Attempt to send data via ESP-NOW to the broadcast address
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&messageData, sizeof(messageData));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  } else {
    Serial.println("Sent with failed");
  }
}

/**
 * Initialize ESP-NOW Communication
 * 
 * @return true if initialization successful, false if any step fails
 */
bool initESPNow() {
  // Set WiFi to station mode (required for ESP-NOW)
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW protocol stack
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return false;
  }

  // Register callback function to monitor transmission status
  esp_now_register_send_cb(OnDataSent);

  // Configure peer information with receiver's MAC address
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);  // Copy MAC address
  peerInfo.channel = 0;                              // Use default WiFi channel
  peerInfo.encrypt = false;                          // Disable encryption for lower latency

  // Add configured peer to ESP-NOW peer list
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return false;
  }

  return true;
}

/**
 * Setup Function
 */
void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  analogSetAttenuation(ADC_11db);

  // Initialize ESP-NOW wireless communication
  if (!initESPNow()) {
    // Critical failure: stop execution to prevent unsafe operation
    while (true) {
      delay(1000);
    }
  }

  Serial.println("Setup complete - Remote Controller ready");
}

/**
 * Main Loop Function
 */
void loop() {  
  uint16_t x = analogRead(A0);
  uint16_t y = analogRead(A1);

  uint8_t currentDirection = DIRECTION_STOP;
  
  // Determine direction based on joystick values
  // X-axis takes priority over Y-axis
  if (x > 3072) {
    currentDirection = DIRECTION_FORWARD;
  } else if (x < 1024) {
    currentDirection = DIRECTION_BACKWARD;
  } else if (y < 1024) {
    currentDirection = DIRECTION_LEFT;
  } else if (y > 3072) {
    currentDirection = DIRECTION_RIGHT;
  } else {
    currentDirection = DIRECTION_STOP;
  }
  
  // Output only when direction changes
  if (currentDirection != lastDirection) {
    sendData(currentDirection);
    lastDirection = currentDirection;
  }

  delay(200);
}
