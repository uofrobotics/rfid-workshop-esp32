#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#include <MFRC522.h>

// Add your WiFi details
const char* ssid = "uofrGuest";
const char* password = "";

#define SS_PIN 5
#define RST_PIN 22
#define LED_PIN 2
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.


// Create an AsyncWebServer object on port 80
AsyncWebServer server(80);

// Function to get the RFID tag UID (assume this is already implemented)
String gettagid() {
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent() ) {
    // Serial.println("No new card");
    return "";
  }
  if ( ! mfrc522.PICC_ReadCardSerial()  ) {
    // Serial.println("cant read card");
    return "";
  }
  //Show UID on serial monitor
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  return content.substring(1);
}

// HTML content for the web page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>U of Robotics RFID Workshop</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 10px;}
  </style>
</head>
<body>
  <h1>U of Robotics RFID Workshop</h2>
  <h2>RFID Tag UID:</24>
  <p id="uid">%UID%</p>
  <div id = "correct-tag"></div>
  <script>
    // Refresh the UID every second
    setInterval(function() {

      fetch('/uid').then(response => response.text()).then(data => {
        current_uid = data;
        if (data == "NONE") document.getElementById('uid').innerText =  "Hold an RFID chip up to the reader.";
        else document.getElementById('uid').innerText = data;
      });

      setCorrectUID();
      scan_for_correct_card();
    }, 100);


    let current_uid = "NONE";
    let correct_uid = "NONE";
    let correct_card_scan_mode = true;

    function setCorrectUID(){
      if (correct_card_scan_mode && current_uid != "NONE") {
        console.log("card set!")
        correct_uid = current_uid;
        correct_card_scan_mode = false;
      }
      console.log("first", current_uid, correct_uid, correct_card_scan_mode);
    }

    function scan_for_correct_card() {
      document.body.style.backgroundColor = "white";
      if(current_uid == "NONE") return;
      else if(current_uid == correct_uid) document.body.style.backgroundColor = "#00ff00";
      else document.body.style.backgroundColor = "#ff0000";
      console.log("second", current_uid, correct_uid, correct_card_scan_mode);
    }

  </script>
</body>
</html>
)rawliteral";

// Replaces placeholders in the HTML with dynamic values
String processor(const String& var) {
  return String();
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522

  // Configure GPIO 2 as an output
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  // Route to handle the UID update request
  server.on("/uid", HTTP_GET, [](AsyncWebServerRequest *request) {
    String uid = gettagid(); // Get the current RFID tag UID
    // if there is no id coming back, scan again to be sure
    if(uid == "") uid = gettagid();
    // if it is stil blank, then there is definitly no card
    if(uid == "") uid = "NONE";
    request->send(200, "text/plain", uid);
  });

  // Start the server
  server.begin();
}

void loop() {
  // Nothing to do here
}