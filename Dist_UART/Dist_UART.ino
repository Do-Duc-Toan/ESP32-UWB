#include <SPI.h>
#include <DW1000Ranging.h>
#include <WiFi.h>
#include "link.h"

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4
#define PIN_RST 27
#define PIN_IRQ 34

// Define UART0 pins for communication with Jetson Nano (these are default for ESP32)
#define UART0_TX_PIN 1
#define UART0_RX_PIN 3

const char *ssid = "MINH HUE";
const char *password = "minhtoan";
const char *host = "192.168.1.208";
WiFiClient client;

struct MyLink *uwb_data;
int index_num = 0;
long runtime = 0;
String all_json = "";

// Define the addresses for the anchors
const int anchor1_address = 0x1782;
const int anchor2_address = 0x1783;
const int anchor3_address = 0x1784;
const int anchor4_address = 0x1785;

float anchorRanges[4] = {0, 0, 0, 0};  // Ranges from four anchors
int anchorCount = 0;

void setup()
{
    Serial.begin(115200);  // For debug output to serial monitor

    // Initialize UART0 for communication with Jetson Nano
    Serial1.begin(115200, SERIAL_8N1, UART0_RX_PIN, UART0_TX_PIN);  // UART0 initialization

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected");
    Serial.print("IP Address:");
    Serial.println(WiFi.localIP());

    if (client.connect(host, 8080))
    {
        Serial.println("Success");
        client.print(String("GET /") + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Connection: close\r\n" +
                     "\r\n");
    }

    delay(1000);

    // Init the configuration
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, DW_CS, PIN_IRQ);

    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);

    // We start the module as a tag
    DW1000Ranging.startAsTag("7D:00:22:EA:82:60:3B:9C", DW1000.MODE_LONGDATA_RANGE_LOWPOWER);

    uwb_data = init_link();

    // Print header for CSV format
    Serial.println("Anchor_1,Anchor_2,Anchor_3,Anchor_4");
    Serial1.println("Anchor_1,Anchor_2,Anchor_3,Anchor_4");  // Also transmit header over UART0
}

void loop()
{
    DW1000Ranging.loop();
}

void newRange()
{
    int current_anchor_address = DW1000Ranging.getDistantDevice()->getShortAddress();
    float range = DW1000Ranging.getDistantDevice()->getRange();

    if (current_anchor_address == anchor1_address)
    {
        anchorRanges[0] = range;
    }
    else if (current_anchor_address == anchor2_address)
    {
        anchorRanges[1] = range;
    }
    else if (current_anchor_address == anchor3_address)
    {
        anchorRanges[2] = range;
    }
    else if (current_anchor_address == anchor4_address)
    {
        anchorRanges[3] = range;
    }

    anchorCount++;

    if (anchorCount == 4)
    {
        // Print ranges to serial monitor and transmit via UART0 to Jetson Nano
        String rangesOutput = String(anchorRanges[0] > 0 ? anchorRanges[0] : 0) + "," +
                              String(anchorRanges[1] > 0 ? anchorRanges[1] : 0) + "," +
                              String(anchorRanges[2] > 0 ? anchorRanges[2] : 0) + "," +
                              String(anchorRanges[3] > 0 ? anchorRanges[3] : 0);

        Serial.println(rangesOutput);  // Output to serial monitor
        Serial1.println(rangesOutput);  // Transmit via UART0 to Jetson Nano

        // Reset anchor count after printing
        anchorCount = 0;
    }

    fresh_link(uwb_data, DW1000Ranging.getDistantDevice()->getShortAddress(), range, DW1000Ranging.getDistantDevice()->getRXPower());
}

void newDevice(DW1000Device *device)
{
    add_link(uwb_data, device->getShortAddress());
}

void inactiveDevice(DW1000Device *device)
{
    delete_link(uwb_data, device->getShortAddress());
}

void send_udp(String *msg_json)
{
    if (client.connected())
    {
        client.print(*msg_json);
        Serial.println("UDP send");
    }
}
