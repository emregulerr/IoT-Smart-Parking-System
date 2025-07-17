#include <Servo.h> // Servo library is added to the project.
#include <SPI.h>   // SPI library is added to the project.
#include <RFID.h>  // RFID library is added to the project.

// Pin definitions for the RFID card.
#define SS_PIN 53
#define RST_PIN 49

// Pin definitions for LEDs used in the entry system.
#define kLed 7 // Red LED
#define yLed 8 // Green LED

// Variables for WiFi connection.
#define ssid "YOUR_WIFI_SSID"         // WiFi name to connect to.
#define pass "YOUR_WIFI_PASSWORD"     // WiFi password.
#define server "your-domain.com"      // Website to connect to.

// Change this to a long, random, and secret string
// This key must match the one in control.php
String apiKey = "YOUR_SECRET_API_KEY";

String veri = "";             // Variable for data to be sent via WiFi.
String uyeler = "";           // Variable for the member list received from the database via WiFi.
String uye = "";              // Variable to hold the RFID card number in quotes.
String postVeri = "";         // Variable for the HTTP POST REQUEST to be sent via WiFi.
String tmpResp = "";          // Variable to hold responses from the WiFi module (via serial port).
String espKomutu = "";        // Variable to hold commands to be sent to the WiFi module (ESP).
String kart = "";             // Variable for the subscriber card number.
String uri = "";              // URI of the web file on the site where we will send data.

// Variables to hold the occupied/empty status of parking spaces.
String park1 = "1"; // 1 indicates empty
String park2 = "1";
String park3 = "1";
String park4 = "1";

// Time variables for checking parking space sensors.
long kontSuresi = 15000; // Check interval in milliseconds
long sonKontrol = 0;   // Last check time
unsigned long suan;      // Current time

RFID rfid(SS_PIN, RST_PIN);   // RFID object created.

// Servo objects created.
Servo p1;
Servo p2;
Servo p3;
Servo p4;
Servo p5; // Entry barrier servo

void setup() {  // Code block that runs once when the system starts:
  // Sensor pins defined as input.
  pinMode(3, INPUT);
  pinMode(4, INPUT);
  pinMode(5, INPUT);
  pinMode(6, INPUT);

  // LED pins for the entry system defined as output.
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);

  // Turn off entry LEDs on startup.
  digitalWrite(7, LOW);
  digitalWrite(8, LOW);

  SPI.begin();  // SPI (serial communication) initiated.
  rfid.init();  // RFID initialized.

  // Servos attached to their respective pins.
  p1.attach(10);
  p2.attach(11);
  p3.attach(12);
  p4.attach(13);
  p5.attach(9);

  // Servos set to initial position.
  p1.write(0);
  p2.write(0);
  p3.write(0);
  p4.write(0);
  p5.write(0);

  Serial.begin(115200);   // Serial communication started for Arduino-PC communication.
  Serial3.begin(115200);  // Serial communication started for Arduino-ESP communication.
  Serial3.println("AT+RST");  // Reset the WiFi module.
  delay(1000);            // Wait for 1 sec.

  do {
    while (Serial3.available()) {
      Serial3.read(); // Clear any previous responses from ESP.
    }
    Serial.print("...");
    Serial3.println("AT"); // Send connection test command to ESP.
    delay(1000);  // Wait 1 second for communication with ESP.
  } while (!Serial3.find("OK")); // Keep trying until connection is successful.
  Serial.println("\nConnected to ESP.");

  Serial3.println("AT+CWMODE=1"); // Set ESP WiFi mode to STA. This allows ESP to connect to other networks.
  delay(1000);

  do {
    while (Serial3.available()) {
      Serial3.read(); // Clear any previous responses from ESP.
    }
    Serial.print("...");
    espKomutu = String("AT+CWJAP=\"") + ssid + "\",\"" + pass + "\"";
    Serial3.println(espKomutu);  // Connect to the network.
    delay(3000);  // Wait 3 seconds for connection.
  } while (!Serial3.find("OK"));  // Keep trying until connection is successful.
  Serial.println("\nConnected to the network.");
  uri = "/control.php"; // Set the page to which data will be sent.
  uyelereBak(); // Fetch the members from the database on startup.
}

void loop() { // Code block that runs continuously while the system is on:
  sensor_oku(); // Check the occupied/empty status of parking spaces.
  if (park1 == "1" || park2 == "1" || park3 == "1" || park4 == "1") { // If any parking space is empty...
    digitalWrite(kLed, LOW); // Do not turn on the red LED at the entrance.
    giris_kontrol();  // Is any RFID card being read?
  } else { // If all parking spaces are full...
    digitalWrite(kLed, HIGH); // Turn on the red LED at the entrance (indicates parking is full).
  }
  suan = millis();  // Get the current time.
  if (suan - sonKontrol >= kontSuresi) {  // If the specified check interval has passed since the last check...
    park_durumu_guncelle(); // Send the parking space status to the database.
    sonKontrol = suan;  // Update the last check time.
  }
}

void sensor_oku() { // Function to check sensor status and assign to string variables.
  park1 = String(digitalRead(3));
  park2 = String(digitalRead(4));
  park3 = String(digitalRead(5));
  park4 = String(digitalRead(6));
}

void park_durumu_guncelle() { // Function to send parking status to the website and update reservation status based on the response.
  veri = "park1=" + park1 + "&park2=" + park2 + "&park3=" + park3 + "&park4=" + park4 + "&cihaz=" + apiKey;
  tmpResp = veri_gonder(veri);
  if (tmpResp.indexOf("REZ1") > 0) {
    p1.write(90);
  } else {
    p1.write(0);
  }
  if (tmpResp.indexOf("REZ2") > 0) {
    p2.write(90);
  } else {
    p2.write(0);
  }
  if (tmpResp.indexOf("REZ3") > 0) {
    p3.write(90);
  } else {
    p3.write(0);
  }
  if (tmpResp.indexOf("REZ4") > 0) {
    p4.write(90);
  } else {
    p4.write(0);
  }
}

String veri_gonder(String veri) { // Function to send data to the website.
  do {
    while (Serial3.available()) {
      Serial3.read(); // Clear any previous responses from ESP.
    }
    Serial.print("...");
    espKomutu = String("AT+CIPSTART=\"TCP\",\"") + server + "\",80";
    Serial3.println(espKomutu);  // Connect to the site via TCP.
    delay(1000);  // Wait 1 second for connection.
  } while (!Serial3.find("OK"));  // Keep trying until connection is successful.
  Serial.println("\nConnected to the site.");
  tmpResp = "";
  Serial.println("\n");
  Serial.println(veri);
  postVeri =
    "POST " + uri + " HTTP/1.0\r\n" +
    "Host: " + server + "\r\n" +
    "Accept: *" + "/" + "*\r\n" +
    "Content-Length: " + veri.length() + "\r\n" +
    "Content-Type: application/x-www-form-urlencoded\r\n" +
    "\r\n" + veri;
  while (Serial3.available()) {
    Serial3.read(); // Clear any previous responses from ESP.
  }
  espKomutu = String("AT+CIPSEND=") + postVeri.length();
  Serial3.println(espKomutu); // Prepare to send data to the site with the data size.
  delay(500);   // Wait 0.5 seconds for the operation.
  if (Serial3.find(">")) { // Check if ready to send data, then send.
    Serial.println("Sending data...");
    Serial3.print(postVeri);
    if (Serial3.find("SEND OK")) {
      Serial.println("Data sent.");
      while (Serial3.available()) {
        tmpResp += Serial3.readString();
      }
    } else {
      tmpResp = "Send failed (2)";
    }
  } else {
    tmpResp = "Send failed (1)";
  }
  Serial3.println("AT+CIPCLOSE"); // Close the site connection.
  Serial.println(tmpResp);
  return tmpResp;
}

void uyelereBak() { // Function to get the member list from the site.
  veri =  "park1=" + park1 + "&park2=" + park2 + "&park3=" + park3 + "&park4=" + park4 + "&istek=uye&cihaz=" + apiKey;
  do {
    while (Serial3.available()) {
      Serial3.read(); // Clear any previous responses from ESP.
    }
    Serial.print("...");
    espKomutu = String("AT+CIPSTART=\"TCP\",\"") + server + "\",80";
    Serial3.println(espKomutu);  // Connect to the site via TCP.
    delay(1000);  // Wait 1 second for connection.
  } while (!Serial3.find("OK"));  // Keep trying until connection is successful.
  Serial.println("\nConnected to the site.");
  tmpResp = "";
  Serial.println("\n");
  Serial.println(veri);
  postVeri =
    "POST " + uri + " HTTP/1.0\r\n" +
    "Host: " + server + "\r\n" +
    "Accept: *" + "/" + "*\r\n" +
    "Content-Length: " + veri.length() + "\r\n" +
    "Content-Type: application/x-www-form-urlencoded\r\n" +
    "\r\n" + veri;
  while (Serial3.available()) {
    Serial3.read(); // Clear any previous responses from ESP.
  }
  espKomutu = String("AT+CIPSEND=") + postVeri.length();
  Serial3.println(espKomutu); // Prepare to send data to the site with the data size.
  delay(500);   // Wait 0.5 seconds for the operation.
  if (Serial3.find(">")) { // Check if ready to send data, then send.
    Serial.println("Sending member list request...");
    Serial3.print(postVeri);
    if (Serial3.find("SEND OK")) {
      Serial.println("Member list request sent.");
      while (Serial3.available()) {
        uyeler += Serial3.readString();
      }
    } else {
      Serial.println("Send failed (2)");
    }
  } else {
    Serial.println("Send failed (1)");
  }
  Serial3.println("AT+CIPCLOSE"); // Close the site connection.
  Serial.println(uyeler);
}

void giris_kontrol() { // Function to check if an RFID card is being read and, if so, run the member check function.
  if (rfid.isCard()) {
    if (rfid.readCardSerial()) {
      kart = String(rfid.serNum[0]);
      kart += String(rfid.serNum[1]);
      kart += String(rfid.serNum[2]);
      kart += String(rfid.serNum[3]);
      kart += String(rfid.serNum[4]);
      uye_kontrol();
    }
    kart = "";
  }
}

void uye_kontrol() { // Function to check if the scanned RFID card is in the member list.
  uye = "'" + kart + "'";
  if (uyeler.indexOf(uye) > 0) { // If the member exists in the list;
    digitalWrite(yLed, HIGH); // Turn on the green LED.
    p5.write(90); // Lift the barrier.
    delay(3000);
    p5.write(0); // Lower the barrier.
    digitalWrite(yLed, LOW); // Turn off the green LED.
    veri = "park1=" + park1 + "&park2=" + park2 + "&park3=" + park3 + "&park4=" + park4 + "&cihaz=" + apiKey + "&uye=" + kart;
    tmpResp = veri_gonder(veri);
  } else { // If such a card is not registered in the system;
    for (int i = 1; i <= 3; i++) { // Blink the red LED 3 times as a warning.
      digitalWrite(kLed, HIGH);
      delay(100);
      digitalWrite(kLed, LOW);
      delay(100);
    }
  }
}
