#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "WiFi.h"
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include "icons.h"
#include <Wire.h>
#include <U8g2lib.h>

#define GPS_TX_PIN 34 
#define GPS_RX_PIN 35 
#define GPIO_PIN 26

TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

int uniqueSSIDs = 0; 
String discoveredSSIDs; 

int currentPage = 1;

unsigned long lastButtonPress = 0;
const unsigned long buttonInterval = 250;


void appendFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Appending to file: %s\n", path);

    // Open the file for reading and check for existing content
    File file = fs.open(path, FILE_READ);
    if (!file) {
        Serial.println("Failed to open file for reading");
    } else {
        // Read existing content and store it in a string
        String existingContent;
        while (file.available()) {
            existingContent += (char)file.read();
        }
        file.close();

        // Extract SSID from the message
        String newSSID = String(message);
        newSSID = newSSID.substring(newSSID.indexOf(": ") + 2, newSSID.indexOf(","));

        // Check if the new SSID already exists in the file
        if (existingContent.indexOf(newSSID) != -1 || discoveredSSIDs.indexOf(newSSID) != -1) {
            Serial.println("SSID already exists. Skipping append.");
            return;
        } else {
            uniqueSSIDs++; // Increment unique SSID count
            discoveredSSIDs += newSSID + ','; // Add new SSID to discovered list
        }
    }

    // If SSID doesn't exist, open the file for appending and write new data
    file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    if (file.print(message)) {
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}


void setup() {
  
    Serial.begin(115200);

    pinMode(GPIO_PIN, INPUT_PULLUP);
    
    gpsSerial.begin(9600);
    delay(1000); 

    u8g2.begin();
    u8g2.setBitmapMode(1);
    u8g2.setFlipMode(1);

    if (!SD.begin()) {
        Serial.println("Card Mount Failed");
        return;
    }
}

String getCapabilities(int networkIndex) {
    wifi_auth_mode_t authMode = WiFi.encryptionType(networkIndex);
    switch (authMode) {
        case WIFI_AUTH_OPEN:
            return "Open";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
        case WIFI_AUTH_WPA2_PSK:
            return "WPA";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WPA2 Enterprise";
        default:
            return "Unknown";
    }
}


void loop() {
    unsigned long currentMillis = millis();

    if (digitalRead(GPIO_PIN) == LOW && currentMillis - lastButtonPress >= buttonInterval) { 
        currentPage = (currentPage % 2) + 1; 
        lastButtonPress = currentMillis; 
    }

    switch (currentPage) {
        case 1:
            statsPage1();
            break;
        case 2:
            statsPage2();
            break;
    }

    Serial.println(WiFi.scanNetworks());
  
    int numNetworks = WiFi.scanNetworks();
    if (numNetworks == 0) {
        Serial.println("No networks found");
    } else {
        Serial.printf("%d networks found\n", numNetworks);
        while (gpsSerial.available() > 0) {
            if (gps.encode(gpsSerial.read())) {
                if (gps.location.isValid()) {
                    Serial.print("Latitude: ");
                    Serial.println(gps.location.lat(), 6);
                    Serial.print("Longitude: ");
                    Serial.println(gps.location.lng(), 6);
                }
                if (gps.date.isValid()) {
                    Serial.print("Date: ");
                    Serial.print(gps.date.year());
                    Serial.print("-");
                    Serial.print(gps.date.month());
                    Serial.print("-");
                    Serial.println(gps.date.day());
                }
                if (gps.time.isValid()) {
                    Serial.print("Time: ");
                    Serial.print(gps.time.hour());
                    Serial.print(":");
                    Serial.print(gps.time.minute());
                    Serial.print(":");
                    Serial.print(gps.time.second());
                    Serial.print(".");
                    Serial.println(gps.time.centisecond());
                }
            }
        }

        for (int i = 0; i < numNetworks; ++i) {
            String capabilities = getCapabilities(i);
            String networkInfo = WiFi.BSSIDstr(i) + "," + WiFi.SSID(i) + "," + capabilities + "," + String(WiFi.RSSI(i)) + "," + String(WiFi.channel(i));
            
            if (gps.location.isValid()) {
                networkInfo += "," + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
            } else {
                networkInfo += ",,,";
            }
            
            networkInfo += ",0,0,0\n"; // Altitude, Accuracy, and Type are set to 0
            
            if (gps.location.isValid()) {
            appendFile(SD, "/wifi_networks.csv", networkInfo.c_str());
           }
        }
    }
}



void statsPage1() {

  //if (gps.location.isValid()) {
    
    u8g2.clearBuffer();
  
    u8g2.setFont(u8g2_font_7x14_tn);
    u8g2.setCursor(90, 32);
    u8g2.print(WiFi.scanNetworks());
    u8g2.drawXBMP(70, 17, 16, 16, wifi);

    u8g2.setFont(u8g2_font_7x14_tn);
    u8g2.setCursor(90, 12);
    u8g2.print(gps.satellites.value());
    u8g2.drawXBMP(70, -3, 16, 16, satellite);

        int Networks = WiFi.scanNetworks();
        
        switch (Networks) {
            case 0:
                u8g2.drawXBMP(0, -10, 128, 45, lonely);
                break;
            case 10 ... 20:
                u8g2.drawXBMP(0, -10, 128, 45, normal);
                break;
            case 21 ... 30:
                u8g2.drawXBMP(0, -10, 128, 45, smart);
                break;
            case 31 ... 40:
                u8g2.drawXBMP(0, -10, 128, 45, cool);
                break;
            default:
                u8g2.drawXBMP(0, -10, 128, 45, normal);
                break;
  } 
    u8g2.sendBuffer();

   //} else {

   // u8g2.drawXBMP(0, 0, 128, 45, antena);
   // u8g2.sendBuffer();
  //}
}


void statsPage2() {
    u8g2.setFont(u8g2_font_5x7_tf);

    int yPos = 15; 
    int numNetworks = WiFi.scanNetworks();
    if (numNetworks == 0) {
        Serial.println("No networks found");
        return;
    }
    
    u8g2.firstPage();
    do {
        for (int i = 0; i < numNetworks; ++i) {
            String ssid = WiFi.SSID(i);

            u8g2.setCursor(0, yPos + i * 10); 
            u8g2.print(ssid.substring(0, 7)); 
            
            String info = "|" + String(WiFi.encryptionType(i));
            u8g2.print(info); 
            
            if (u8g2.getMaxCharHeight() * (i + 1) > u8g2.getHeight()) {
                for (int j = 0; j < u8g2.getMaxCharHeight() * (i + 1) - u8g2.getHeight(); j++) {
                    u8g2.sendBuffer();
                    delay(100); 
                    u8g2.clearBuffer();
                    for (int k = 0; k < numNetworks; ++k) {
                        u8g2.setCursor(0, -j + yPos + k * 10); 
                        u8g2.print(WiFi.SSID(k).substring(0, 7)); 
                        u8g2.print("|");
                        u8g2.print(WiFi.RSSI(k));
                        u8g2.print(" dBm|");
                        u8g2.print(WiFi.channel(k)); 
                        u8g2.print("|");
                        if (WiFi.encryptionType(k) == WIFI_AUTH_OPEN) {
                            u8g2.print("Open");
                        } else if (WiFi.encryptionType(k) == WIFI_AUTH_WEP) {
                            u8g2.print("WEP");
                        } else if (WiFi.encryptionType(k) == WIFI_AUTH_WPA_PSK || WiFi.encryptionType(k) == WIFI_AUTH_WPA2_PSK) {
                            u8g2.print("WPA");
                        } else if (WiFi.encryptionType(k) == WIFI_AUTH_WPA2_ENTERPRISE) {
                            u8g2.print("WPA2 Enterprise");
                        } else {
                            u8g2.print("Unknown");
                        }
                    }
                }
            }
        }
    } while (u8g2.nextPage());
}
