#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "ENTER_YOUR_WIFI_NAME";
const char* password = "ENTER_YOUR_WIFI_PASSWORD";

const char* PARAM_STRING = "inputString";
const char* PARAM_INT = "inputInt";
const char* PARAM_FLOAT = "inputFloat";

// HTML web page to handle 3 input fields (inputString, inputInt, inputFloat)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js"></script>
  <script>
    function submitMessage() {
     var value1 = document.getElementById('input1').value;
     var value2 = document.getElementById('input2').value;
     var value3 = document.getElementById('input3').value;  
     document.getElementById('joint').value =value1+","+value2+","+value3;
     document.getElementById("frm1").submit();
    }
  </script></head><body>
    <input type="text" id="input1">
    <input type="text" id="input2">
    <input type="text" id="input3">
     <form action="/get" id="frm1" target="hidden-form">
    <input type="hidden" id="joint" name="inputString">
    <input type="button" onclick="submitMessage()" value="Submit">
    </form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  Serial.print("&");
  Serial.println(message);
  Serial.println("&*");
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

// Replaces placeholder with stored values
String processor(const String& var){
  //Serial.println(var);
  if(var == "inputString"){
    return readFile(SPIFFS, "/inputString.txt");
  }
  else if(var == "inputInt"){
    return readFile(SPIFFS, "/inputInt.txt");
  }
  else if(var == "inputFloat"){
    return readFile(SPIFFS, "/inputFloat.txt");
  }
  return String();
}

void setup() {
  Serial.begin(115200);
  // Initialize SPIFFS
  #ifdef ESP32
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #else
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET inputString value on <ESP_IP>/get?inputString=<inputMessage>
        Serial.println(request->getParam(0)->name());
    if (request->hasParam(PARAM_STRING)) {
      inputMessage = request->getParam(PARAM_STRING)->value();
      int splitC=inputMessage.indexOf(",");
      String Higher=inputMessage.substring(0,splitC);
      String lower=inputMessage.substring(splitC+1);
      writeFile(SPIFFS, "/inputString.txt", Higher.c_str());
      splitC=lower.indexOf(",");
      String Lower=lower.substring(0,splitC);
      String floatt=lower.substring(splitC+1);
      writeFile(SPIFFS,"/inputInt.txt",Lower.c_str());
      writeFile(SPIFFS, "/inputFloat.txt", floatt.c_str());
    }
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    else if (request->hasParam(PARAM_INT)) {
      inputMessage = request->getParam(PARAM_INT)->value();
      writeFile(SPIFFS, "/inputInt.txt", inputMessage.c_str());
    }
    // GET inputFloat value on <ESP_IP>/get?inputFloat=<inputMessage>
    else if (request->hasParam(PARAM_FLOAT)) {
      inputMessage = request->getParam(PARAM_FLOAT)->value();
      writeFile(SPIFFS, "/inputFloat.txt", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop() {
}
