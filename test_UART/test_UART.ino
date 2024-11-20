// Define the UART0 pins
#define TX_PIN 1  // GPIO 1 (TX0) - You can adjust if needed
#define RX_PIN 3  // GPIO 3 (RX0) - You can adjust if needed

void setup() {
  // Start serial communication with Jetson Nano on UART0 (Serial)
  Serial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);  // Baud rate 115200, 8 data bits, no parity, 1 stop bit

  // For debugging, start communication with the Serial Monitor
  Serial.println("Starting communication with Jetson Nano...");
}

void loop() {
  // Transmit data to Jetson Nano
  String dataToSend = "Hello from ESP32!";
  Serial.println("Sending data: " + dataToSend);
  Serial.write(dataToSend.c_str());  // Transmit data
  
  delay(1000);  // Wait for 1 second before sending the next message
}
