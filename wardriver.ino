/*
 * Waveshare ESP32-S3 AMOLED 2.06" Wardriver
 * WiFi Network Scanner with SD Card Logging
 * 
 * Hardware: ESP32-S3 Touch AMOLED 2.06" (240x536 ST7789)
 * Features: Real-time WiFi scanning + CSV logging
 */

#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <time.h>

// ============================================================================
// DISPLAY CONFIGURATION - Waveshare ESP32-S3 AMOLED 2.06"
// ============================================================================
#define GFX_BL 48  // Backlight pin

// SPI pins for Waveshare ESP32-S3 AMOLED
#define TFT_CS 12
#define TFT_DC 11
#define TFT_RST 13
#define TFT_SCLK 14
#define TFT_MOSI 47

Arduino_DataBus *bus = new Arduino_SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1);
Arduino_GFX *gfx = new Arduino_ST7789(bus, TFT_RST, 0, true, 240, 536);

// ============================================================================
// SD CARD CONFIGURATION
// ============================================================================
const int SD_CS = 21;  // SD card chip select pin
File logFile;
String logFileName;
bool sdCardReady = false;

// ============================================================================
// DISPLAY VARIABLES
// ============================================================================
int16_t w, h, text_size, banner_height, graph_baseline, graph_height, channel_width, signal_width;

// RSSI RANGE
#define RSSI_CEILING -40
#define RSSI_FLOOR -100

// Channel color mapping
uint16_t channel_color[] = {
    RGB565_RED, RGB565_ORANGE, RGB565_YELLOW, RGB565_GREEN, RGB565_CYAN, RGB565_BLUE, RGB565_MAGENTA,
    RGB565_RED, RGB565_ORANGE, RGB565_YELLOW, RGB565_GREEN, RGB565_CYAN, RGB565_BLUE, RGB565_MAGENTA
};

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nWaveshare ESP32-S3 AMOLED 2.06\" Wardriver");
    Serial.println("================================\n");

    // Initialize display
    initDisplay();

    // Initialize SD card
    initSDCard();

    // Set WiFi to station mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    // Draw initial banner
    gfx->fillScreen(RGB565_BLACK);
    gfx->setTextSize(text_size);
    gfx->setTextColor(RGB565_RED);
    gfx->setCursor(0, 0);
    gfx->print("WiFi");
    gfx->setTextColor(RGB565_WHITE);
    gfx->print(" Analyzer");
    gfx->setTextColor(RGB565_CYAN);
    gfx->setCursor(0, text_size * 4);
    gfx->setTextSize(1);
    gfx->print(sdCardReady ? "SD: OK" : "SD: FAIL");
    gfx->setCursor(0, text_size * 4 + 12);
    gfx->print("Scanning...");
}

// ============================================================================
// INITIALIZE DISPLAY
// ============================================================================
void initDisplay() {
    if (!gfx->begin()) {
        Serial.println("ERROR: Display initialization failed!");
        while (1) delay(1000);
    }

    w = gfx->width();
    h = gfx->height();
    text_size = (h < 200) ? 1 : 2;
    banner_height = text_size * 3 * 4;
    graph_baseline = h - 20;
    graph_height = graph_baseline - banner_height - 30;
    channel_width = w / 17;
    signal_width = channel_width * 2;

    // Backlight
#ifdef GFX_BL
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
#endif

    Serial.printf("Display: %dx%d initialized\n", w, h);
}

// ============================================================================
// INITIALIZE SD CARD
// ============================================================================
void initSDCard() {
    if (!SD.begin(SD_CS)) {
        Serial.println("WARNING: SD card initialization failed. Logging disabled.");
        sdCardReady = false;
        return;
    }

    sdCardReady = true;
    Serial.println("SD card initialized successfully!");

    // Create logs directory if it doesn't exist
    if (!SD.exists("/wardriver_logs")) {
        SD.mkdir("/wardriver_logs");
        Serial.println("Created /wardriver_logs directory");
    }

    // Create filename with timestamp
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    char filename[50];
    strftime(filename, sizeof(filename), "/wardriver_logs/scan_%Y%m%d_%H%M%S.csv", timeinfo);
    logFileName = String(filename);

    // Write CSV header
    logFile = SD.open(logFileName, FILE_WRITE);
    if (logFile) {
        logFile.println("BSSID,SSID,Channel,RSSI,Encryption");
        logFile.close();
        Serial.printf("Log file created: %s\n", logFileName.c_str());
    } else {
        Serial.println("ERROR: Could not create log file");
        sdCardReady = false;
    }
}

// ============================================================================
// LOG NETWORK TO SD CARD
// ============================================================================
void logNetworkToSD(int index, int channel, int rssi, const char *ssid, int encType) {
    if (!sdCardReady) return;

    logFile = SD.open(logFileName, FILE_APPEND);
    if (logFile) {
        char buffer[256];
        uint8_t *bssid = WiFi.BSSID(index);
        const char *encStr = "Unknown";

        if (encType == WIFI_AUTH_OPEN) encStr = "OPEN";
        else if (encType == WIFI_AUTH_WEP) encStr = "WEP";
        else if (encType == WIFI_AUTH_WPA_PSK) encStr = "WPA";
        else if (encType == WIFI_AUTH_WPA2_PSK) encStr = "WPA2";
        else if (encType == WIFI_AUTH_WPA_WPA2_PSK) encStr = "WPA/WPA2";

        snprintf(buffer, sizeof(buffer),
                 "%02X:%02X:%02X:%02X:%02X:%02X,\"%s\",%d,%d,%s",
                 bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
                 ssid, channel, rssi, encStr);

        logFile.println(buffer);
        logFile.close();
    }
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
    uint8_t ap_count_list[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int32_t noise_list[] = {RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR};
    int32_t peak_list[] = {RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR};
    int16_t peak_id_list[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    int32_t channel;
    int16_t idx;
    int32_t rssi;
    uint8_t *bssid;
    String ssid;
    uint16_t color;
    int16_t height, offset, text_width;

    // Perform WiFi scan
    int n = WiFi.scanNetworks(false, true, true, 500);

    // Clear graph area
    gfx->fillRect(0, banner_height, w, h - banner_height, RGB565_BLACK);
    gfx->setTextSize(1);

    if (n == 0) {
        gfx->setTextColor(RGB565_WHITE);
        gfx->setCursor(0, banner_height);
        gfx->println("no networks found");
    } else {
        // Process scan results
        for (int i = 0; i < n; i++) {
            channel = WiFi.channel(i);
            idx = channel - 1;
            rssi = WiFi.RSSI(i);
            bssid = WiFi.BSSID(i);
            ssid = WiFi.SSID(i);

            // Log to SD card
            logNetworkToSD(i, channel, rssi, ssid.c_str(), WiFi.encryptionType(i));

            // Track peak signal per channel
            if (peak_list[idx] < rssi) {
                peak_list[idx] = rssi;
                peak_id_list[idx] = i;
            }

            // Check for duplicate SSID
            bool duplicate_SSID = false;
            for (int j = 0; j < i; j++) {
                if ((WiFi.channel(j) == channel) && matchBssidPrefix(WiFi.BSSID(j), bssid)) {
                    duplicate_SSID = true;
                    break;
                }
            }

            if (!duplicate_SSID) {
                ap_count_list[idx]++;

                // Calculate noise statistics
                int32_t noise = rssi - RSSI_FLOOR;
                noise *= noise;
                if (channel > 4) noise_list[idx - 4] += noise;
                if (channel > 3) noise_list[idx - 3] += noise;
                if (channel > 2) noise_list[idx - 2] += noise;
                if (channel > 1) noise_list[idx - 1] += noise;
                noise_list[idx] += noise;
                if (channel < 14) noise_list[idx + 1] += noise;
                if (channel < 13) noise_list[idx + 2] += noise;
                if (channel < 12) noise_list[idx + 3] += noise;
                if (channel < 11) noise_list[idx + 4] += noise;
            }
        }

        // Draw WiFi visualizations
        for (int i = 0; i < n; i++) {
            channel = WiFi.channel(i);
            idx = channel - 1;
            rssi = WiFi.RSSI(i);
            color = channel_color[idx];
            height = constrain(map(rssi, RSSI_FLOOR, RSSI_CEILING, 1, graph_height), 1, graph_height);
            offset = (channel + 1) * channel_width;

            if (rssi < RSSI_FLOOR) rssi = RSSI_FLOOR;

            // Draw signal bar
            gfx->startWrite();
            gfx->writeEllipseHelper(offset, graph_baseline + 1, signal_width, height, 0b0011, color);
            gfx->endWrite();

            // Draw peak signal SSID label
            if (i == peak_id_list[idx]) {
                String displaySSID = WiFi.SSID(i);
                if (displaySSID.length() == 0) displaySSID = WiFi.BSSIDstr(i);
                text_width = (displaySSID.length() + 6) * 6;

                if (text_width > w) offset = 0;
                else {
                    offset -= signal_width;
                    if ((offset + text_width) > w) offset = w - text_width;
                }

                gfx->setTextColor(color);
                gfx->setCursor(offset, graph_baseline - 10 - height);
                gfx->print(displaySSID);
                gfx->print('(');
                gfx->print(rssi);
                gfx->print(')');
                if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) gfx->print('*');
            }
        }
    }

    // Print statistics
    gfx->setTextColor(RGB565_WHITE);
    gfx->setCursor(0, banner_height);
    gfx->print(n);
    gfx->print(" networks, SD: ");
    gfx->print(sdCardReady ? "OK" : "FAIL");

    // Find and display best channels (lowest noise)
    bool listed_first_channel = false;
    int32_t min_noise = noise_list[0];
    for (channel = 2; channel <= 11; channel++) {
        idx = channel - 1;
        if (noise_list[idx] < min_noise) min_noise = noise_list[idx];
    }

    gfx->setCursor(0, banner_height + 12);
    gfx->print("Low noise: ");
    for (channel = 1; channel <= 11; channel++) {
        idx = channel - 1;
        if (noise_list[idx] == min_noise) {
            if (listed_first_channel) gfx->print(", ");
            gfx->print(channel);
            listed_first_channel = true;
        }
    }

    // Draw channel axis
    gfx->drawFastHLine(0, graph_baseline, gfx->width(), RGB565_WHITE);
    for (channel = 1; channel <= 14; channel++) {
        idx = channel - 1;
        offset = (channel + 1) * channel_width;
        gfx->setTextColor(channel_color[idx]);
        gfx->setCursor(offset - ((channel < 10) ? 3 : 6), graph_baseline + 2);
        gfx->print(channel);
        if (ap_count_list[idx] > 0) {
            gfx->setCursor(offset - ((ap_count_list[idx] < 10) ? 9 : 12), graph_baseline + 8 + 2);
            gfx->print('{');
            gfx->print(ap_count_list[idx]);
            gfx->print('}');
        }
    }

    // Wait before next scan
    delay(3000);
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================
bool matchBssidPrefix(uint8_t *a, uint8_t *b) {
    for (uint8_t i = 0; i < 5; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}
