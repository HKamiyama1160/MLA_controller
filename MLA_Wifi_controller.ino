
// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebSrv.h>

#define   IN1_PIN    14
#define   IN2_PIN    12
#define   PWM_PIN    13
#define   LED_IND    4
#define   CW_PIN     5
#define   CCW_PIN    16

const int output = PWM_PIN;

// Replace with your network credentials
//const char* ssid = "REPLACE_WITH_YOUR_SSID";
//const char* password = "REPLACE_WITH_YOUR_PASSWORD";

String sliderValue = "0";

const char* PARAM_INPUT = "value";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP Web Server</title>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.3rem;}
    p {font-size: 1.9rem;}
    body {max-width: 400px; margin:0px auto; padding-bottom: 25px;}
    .slider { -webkit-appearance: none; margin: 14px; width: 360px; height: 25px; background: #FFD65C;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; background: #003249; cursor: pointer;}
    .slider::-moz-range-thumb { width: 35px; height: 35px; background: #003249; cursor: pointer; }
      body { font-family: Arial; text-align: center; margin:0px auto; padding-top: 30px;}
      .button {
        padding: 10px 20px;
        font-size: 24px;
        text-align: center;
        outline: none;
        color: #fff;
        background-color: #2f4468;
        border: none;
        border-radius: 5px;
        box-shadow: 0 6px #999;
        cursor: pointer;
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(0,0,0,0);
      }
      .button:hover {background-color: #8b008b}
      .button:active {
        background-color: #8b008b;
        box-shadow: 0 4px #666;
        transform: translateY(2px);
      }
  </style>
</head>
<body>
  <h2>MLA Controller</h2>
  <p><span id="textSliderValue">%SLIDERVALUE%</span></p>
  <p><input type="range" onchange="updateSliderPWM(this)" id="pwmSlider" min="0" max="100" value="%SLIDERVALUE%" step="10" class="slider"></p>
  <p><button class="button" onmousedown="toggleCheckbox('fon');" ontouchstart="toggleCheckbox('fon');" onmouseup="toggleCheckbox('foff');" ontouchend="toggleCheckbox('foff');">Forward</button></p>
  <p><button class="button" onmousedown="toggleCheckbox('bon');" ontouchstart="toggleCheckbox('bon');" onmouseup="toggleCheckbox('boff');" ontouchend="toggleCheckbox('boff');">Backward</button></p>
<script>
function updateSliderPWM(element) {
  var sliderValue = document.getElementById("pwmSlider").value;
  document.getElementById("textSliderValue").innerHTML = sliderValue;
  console.log(sliderValue);
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/slider?value="+sliderValue, true);
  xhr.send();
}
   function toggleCheckbox(x) {
     var xhr = new XMLHttpRequest();
     xhr.open("GET", "/" + x, true);
     xhr.send();
   }
</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if (var == "SLIDERVALUE"){
    return sliderValue;
  }
  return String();
}

void setup(){
	
  pinMode(LED_IND, OUTPUT);
  pinMode(CW_PIN, OUTPUT);
  pinMode(CCW_PIN, OUTPUT);
  digitalWrite(LED_IND, LOW);
  digitalWrite(CW_PIN, LOW);
  digitalWrite(CCW_PIN, LOW);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
//  pinMode(PWM_PIN, OUTPUT);
  motor_stop();
  analogWriteFreq(20000);
  analogWriteRange(100);

  // Serial port for debugging purposes
  Serial.begin(115200);

  analogWrite(output, sliderValue.toInt());

  // Connect to Wi-Fi
  Serial.println("");
		Serial.println(ssid);
		WiFi.begin(ssid, password);
  }
	
	//  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  digitalWrite(LED_IND, HIGH);

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
  server.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      sliderValue = inputMessage;
      analogWrite(output, sliderValue.toInt());
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });
	
  // Receive an HTTP GET request
  server.on("/fon", HTTP_GET, [] (AsyncWebServerRequest *request) {
    forward();
    request->send(200, "text/plain", "ok");
  });

  // Receive an HTTP GET request
  server.on("/foff", HTTP_GET, [] (AsyncWebServerRequest *request) {
	 motor_stop();
    request->send(200, "text/plain", "ok");
  });

  // Receive an HTTP GET request
  server.on("/bon", HTTP_GET, [] (AsyncWebServerRequest *request) {
    backward();
    request->send(200, "text/plain", "ok");
  });

  // Receive an HTTP GET request
  server.on("/boff", HTTP_GET, [] (AsyncWebServerRequest *request) {
	 motor_stop();
    request->send(200, "text/plain", "ok");
  });
  
  // Start server
  server.begin();
}

void motor_stop() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(CW_PIN, LOW);
  digitalWrite(CCW_PIN, LOW);
}

void forward() {
  motor_stop();
  delay(100);
  digitalWrite(CW_PIN, HIGH);
  digitalWrite(CCW_PIN, LOW);
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
}

void backward() {
	motor_stop();
	delay(100);
  digitalWrite(CW_PIN, LOW);
  digitalWrite(CCW_PIN, HIGH);
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, HIGH);
}
  
void loop() {
  
}
