#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>


#ifndef STASSID
#define STASSID "esp32-1"
#define STAPSK  "almafa18"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);

volatile float distance;
volatile float triggerOff;
#define SW1 D0

DeserializationError readRequest(DynamicJsonDocument doc) {

  Serial.print("body:");
  Serial.println(server.arg("plain"));
  auto error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    server.send(404, "application/json", "{ \"error\": \"Json read failed\"}");
  }
  return error;
}

void sendSwitchStatus() {
    StaticJsonDocument<200> doc;
    char jsonBuffer[512];
    
    doc["switch"] = digitalRead(SW1)?"true":"false";
    serializeJson(doc, jsonBuffer);
    server.send(200, "application/json", jsonBuffer);
}

void sendDistStatus() {
  StaticJsonDocument<200> doc;
  char jsonBuffer[512];
  
  doc["distance"] = distance;
  doc["triggerOff"] = triggerOff;
  serializeJson(doc, jsonBuffer);
  server.send(200, "application/json", jsonBuffer);  
}

void setup() {
  WiFi.persistent(false);
  Serial.begin(115200);
  // put your setup code here, to run once:
  pinMode(SW1, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D3, INPUT);
  digitalWrite(D4,0);

  Serial.print("Setting soft-AP ... ");
  /* boolean result = WiFi.softAP(ssid, password);
  if(result == true)
  {
    Serial.println("Ready");
  }
  else
  {
    Serial.println("Failed!");
  }*/

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", []() {
    server.send(200, "text/html", "<html><head><title>Contorller</title><link href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.5.0/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-9aIt2nRpC12Uk9gS9baDl411NQApFmC26EwAOH8WgZl5MYYxFfc+NcPb1dKGj7Sk\" crossorigin=\"anonymous\"><script src=\"https://code.jquery.com/jquery-3.5.1.min.js\" integrity=\"sha256-9/aliU8dGd2tb6OSsuzixeV4y/faTqgFtohetphbbj0=\" crossorigin=\"anonymous\"></script><script src=\"https://stackpath.bootstrapcdn.com/bootstrap/4.5.0/js/bootstrap.min.js\" integrity=\"sha384-OgVRvuATP1z7JjHLkuOU7Xw704+h835Lr+6QL9UvYjZE3Ipu6Tp75j7Bh/kR0JKI\" crossorigin=\"anonymous\"></script><script>jQuery(\"body\").on(\"click\",\"button\", function(e){ jQuery.post(\"/switchOn\")})</script></head><body><button value=\"switchOn\">On</button><button value=\"switchOn\">Off</button></body></html>");    
  });

  server.on("/switch", HTTP_PUT, []() {
    const char* sw;
    DynamicJsonDocument doc(400);
    DeserializationError error;
    error = readRequest(doc);
    if (!error) {
      sw = doc["switch"];
      Serial.print("Switch value:");
      Serial.print(sw);
      bool st = strcmp(sw,"true")==0;

      digitalWrite(SW1, st?1:0);
      sendSwitchStatus();
    }
  });
  server.on("/switch", HTTP_GET, []() {
    sendSwitchStatus();
  });

  server.on("/dist", HTTP_GET, []() {
    sendDistStatus();
  });
  server.on("/dist", HTTP_PUT, []() {
    const char* off;
    DynamicJsonDocument doc(400);
    DeserializationError error;
    error = readRequest(doc);
    if (!error) {
      off = doc["triggerOff"];
      Serial.print("trigger value:");
      Serial.print(off);
      triggerOff = atof(off);

      sendDistStatus();
    }
  });
  
  server.begin();
  Serial.println("HTTP server started");
  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
unsigned long lastTime = 0;

void loop() {
  unsigned long currentMillis = millis();
  server.handleClient();  
  digitalWrite(D4,1);
  delayMicroseconds(10);
  digitalWrite(D4,0);
  unsigned long diff = pulseIn(D3, HIGH);
  if (diff>0) {
    distance = (diff)/58.0f;
  } else {
    distance =-1.0f;  
  }

  if (distance < triggerOff) {
    digitalWrite(SW1,0);
  }

  if (currentMillis - lastTime >= 10000) {
    lastTime = currentMillis;
    Serial.print("Distance:");
    Serial.println(distance);
  }
}
