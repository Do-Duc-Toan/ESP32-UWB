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

const char *ssid = "iPhone of Ton";
const char *password = "abc1234567";
const char *host = "192.168.1.208";
WiFiClient client;

struct MyLink *uwb_data;
int index_num = 0;
long runtime = 0;
String all_json = "";

// Define the addresses for the two anchors
const int anchor1_address = 0x1782;  // Replace with actual Anchor 1 address
const int anchor2_address = 0x1783;  // Replace with actual Anchor 2 address
const int anchor3_address = 0x1784; 
const int anchor4_address = 0x1785;

// To store ranges from multiple anchors
float anchorRanges[4] = {0, 0, 0, 0};  // Assuming we have two anchors for simplicity
int anchorCount = 0;

void setup()
{
    Serial.begin(115200);

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
    Serial.println("Anchor_1,Anchor_2,Anchor_3,Anchor_4");  // Header for two anchors
}

void loop()
{
    DW1000Ranging.loop();
}

void newRange()
{
    // Get the short address of the distant device (anchor)
    int current_anchor_address = DW1000Ranging.getDistantDevice()->getShortAddress();

    // Check which anchor the distance belongs to and assign accordingly
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

    // Print ranges when we have data from two anchors
    if (anchorCount == 4)
    {
        // Print distances for both anchors, setting 0 if no range is available
        Serial.print(anchorRanges[0] > 0 ? anchorRanges[0] : 0);  // Anchor 1
        Serial.print(",");
        Serial.print(anchorRanges[1] > 0 ? anchorRanges[1] : 0);  // Anchor 2
        Serial.print(",");
        Serial.print(anchorRanges[2] > 0 ? anchorRanges[2] : 0);  //Anchor 3
        Serial.print(",");
        Serial.println(anchorRanges[3] > 0 ? anchorRanges[3] : 0);  //Anchor 4
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
