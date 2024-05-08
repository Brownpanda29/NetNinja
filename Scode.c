#include <WiFi.h>
#include <esp_wifi.h>
#include <SPIFFS.h>

// Function prototypes
void setupWiFi();
void enableMonitorMode(uint8_t channel, bool is2_4GHz);
void disableMonitorMode(bool is2_4GHz);
void packetSniffer(void* buf, wifi_promiscuous_pkt_type_t type);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  setupWiFi();
}

void loop() {
  //main code logic here
}

void setupWiFi() {
  WiFi.mode(WIFI_OFF);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(packetSniffer);
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin("YOUR_NETWORK_SSID", "YOUR_NETWORK_PASSWORD");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void enableMonitorMode(uint8_t channel, bool is2_4GHz) {
  wifi_country_t country;
  memset(&country, 0, sizeof(country));
  country.cc[0] = 'U'; // Set your country code here
  country.cc[1] = 'S';
  country.schan = 1;
  country.nchan = 11;

  wifi_second_chan_t second_channel = is2_4GHz ? WIFI_SECOND_CHAN_NONE : WIFI_SECOND_CHAN_ABOVE;

  wifi_promiscuous_filter_t filter;
  filter.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL;

  esp_wifi_set_country(&country);
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

void disableMonitorMode(bool is2_4GHz) {
  esp_wifi_set_promiscuous(false);
  WiFi.mode(WIFI_MODE_STA);
}



// Define a global variable to track the number of captured handshakes
int handshakeCount = 0;

void packetSniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
  // Check if the packet is a management frame (handshake)
  if (type == WIFI_PKT_MGMT) {
    // Increment the handshake count
    handshakeCount++;

    // Convert buffer to packet structure
    wifi_promiscuous_pkt_t* packet = (wifi_promiscuous_pkt_t*)buf;

    // Get the frame control field from the packet
    wifi_ieee80211_frame_ctrl_t frameCtrl = packet->rx_ctrl.frame_ctrl;

    // Check if it's a handshake packet (management frame type)
    if (frameCtrl.subtype == WIFI_PKT_MGMT && frameCtrl.type == WIFI_PKT_MGMT &&
        (packet->payload[0] == 0x08 || packet->payload[0] == 0x05)) {
      // Ask the user for the directory path to save the handshake files
      Serial.println("Enter the directory path to save handshake files:");
      while (Serial.available() == 0) {
        // Wait for user input
      }
      String directoryPath = Serial.readStringUntil('\n');

      // Save the captured handshake packet to a file in SPIFFS
      String fileName = "/handshake_" + String(handshakeCount) + ".cap";
      File handshakeFile = SPIFFS.open(directoryPath + fileName, "w");
      if (handshakeFile) {
        // Write the packet data to the file
        handshakeFile.write((const uint8_t*)packet->payload, packet->rx_ctrl.sig_len);
        handshakeFile.close();
        Serial.println("Handshake file saved successfully.");
      } else {
        Serial.println("Error opening handshake file.");
      }
    }
  }
}
