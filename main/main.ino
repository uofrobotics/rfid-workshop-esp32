#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#include <MFRC522.h>

// Add your WiFi details
const char* ssid = "SASKTEL0095";
const char* password = "shupe5955";

#define SS_PIN 5
#define RST_PIN 22
#define LED_PIN 2
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.


// Create an AsyncWebServer object on port 80
AsyncWebServer server(80);


String gettagid() {
  String uid = scanTag();
  // if there is no id coming back, scan again to be sure
  if(uid == "") uid = scanTag();
  // if it is stil blank, then there is definitly no card
  if(uid == "") uid = "NONE";
  return uid;
}

String scanTag() {
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
  <style>
    html {
      font-family: Arial;
      display: inline-block;
      text-align: center;
      color: white;
    }
    h2 {
      font-size: 2.0rem;
    }
    p {
      font-size: 3.0rem;
    }
    body {
      margin: 0px auto;
      margin-top: 0px;
      padding-bottom: 10px;
      background-color: #181C14;
      color: #ECDFCC;
      flex-direction: column;
      display: flex;
      justify-content: center;
      align-items: center;
    }
    #correct-tag {
      width: 450px;
      background: black;
      height: 20px;
      margin: auto;
      border-radius: 8px;
    }
    .nav {
      width: 100%;
      height: auto;
      background-color: #3C3D37;
      justify-content: center;
    }
    .correct-tag {
      padding-bottom: 5px;
    }
  </style>
</head>
<body>
  <div class="nav">
    <h1>U of Robotics RFID Workshop</h1>
  </div>
  <p id="uid"></p>
  <div class="corret-tag-wrapper">
    <div id = "correct-tag-info"></div>
    <div id = "correct-tag"></div>
  <div class="corret-tag-wrapper">
  <script>
    let list_of_scanned_cards

    let current_uid = "NONE";
    let correct_uid = "NONE";
    let correct_card_scan_mode = true;
    // Refresh the UID 0.1 seconds
    setInterval(function() {

      correct_card_scan_mode = correct_uid == "NONE";
      
      getUID();
      getCorrectUID();
      displayUID();
      scan_for_correct_card();

    }, 100);//milliseconds

    function displayUID() {
      let uid_element = document.getElementById("uid");
      if(current_uid == "NONE") uid_element.innerText = "Scanning for RFID chip...";
      else uid_element.innerText = "UID: "+ current_uid;
    }

    function getUID(){
      fetch('/uid').then(response => response.text()).then(data => {
        console.log(data);
        current_uid = data;
      });
    }

    function getCorrectUID() {
      fetch('/correct_uid').then(response => response.text()).then(data => {
        correct_uid = data;
      });
    }

    function scan_for_correct_card() {
      let correct_tag_element = document.getElementById("correct-tag");
      let correct_tag_info_element = document.getElementById("correct-tag-info");
      correct_tag_element.style.backgroundColor = "#ffff00";
      correct_tag_info_element.style.color = "#ffff00";

      if(correct_card_scan_mode) {
        correct_tag_info_element.innerText = "Scan a card to act as the key.";
      } else {
        correct_tag_info_element.innerText = "Correct UID: " + correct_uid;
      }

      if(current_uid == "NONE") return;
      else if(current_uid == correct_uid) {
        correct_tag_element.style.backgroundColor = "#00ff00";
        correct_tag_info_element.style.color = "#00ff00";
        correct_tag_info_element.innerText = "Correct card!";
      }
      else {
        correct_tag_element.style.backgroundColor = "#ff0000";
        correct_tag_info_element.style.color = "#ff0000";
        correct_tag_info_element.innerText = "Incorrect card!";
      }
    }

  </script>
</body>
</html>
)rawliteral";

bool set_correct_tag_mode = true;
String correct_uid = "NONE";
String current_uid = "NONE";

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522

  // Configure GPIO 2 as an output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

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
    Serial.println("Sending Website");
    request->send_P(200, "text/html", index_html);
  });

  // Route to handle the UID update request
  server.on("/uid", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", current_uid);
  });

  server.on("/correct_uid", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", correct_uid);
  });

  // Start the server
  server.begin();
}

void loop() {
  if(WiFi.status() != WL_CONNECTED) return;

  current_uid = gettagid();
  digitalWrite(LED_PIN, LOW);
  // user must scan a tag, settting the correct tag to continue
  if(current_uid == "NONE") return;
  if (set_correct_tag_mode){
    correct_uid = current_uid;
    set_correct_tag_mode = false;
  }

  while(correct_uid == current_uid){
    current_uid = gettagid();
    digitalWrite(LED_PIN, HIGH);
  }

  while(current_uid != "NONE" && correct_uid != current_uid) {
    current_uid = gettagid();
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }
}