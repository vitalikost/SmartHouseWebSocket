/*
 * WebSocketClient.ino
 *
 *  Created on: 24.05.2015
 *
 */
#include <ArduinoJson.h>
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <Hash.h>

#include <EEPROM.h>

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define USE_SERIAL Serial

boolean start = true;
long previousMillis = 60000;        // храним время последнего переключения 
long interval = 1000*60*5;          // интервал между проверкой доступности сервера (300 секунд)
long interval_restart = 1000*60*60; // интервал между включение/выключением (60 минут)

DynamicJsonBuffer jsonBuffer;


void runCommand(String command)
  {         String StatusPin = "";
            float OK = false;
            if (command == "ON_1") {
                digitalWrite(D1, HIGH);
                EEPROM.write(0, 1);
                OK = true;    
            }
      
            if (command == "OFF_1") {
                digitalWrite(D1, LOW);  
                EEPROM.write(0, 0);
                OK = true;
            }  

            if (command == "ON_2") {
                digitalWrite(D2, HIGH);
                EEPROM.write(1, 1);
                OK = true;    
            }
      
            if (command == "OFF_2") {
                digitalWrite(D2, LOW);  
                EEPROM.write(1, 0);
                OK = true;
            }  
            
            if(OK)
              EEPROM.commit();
            
            if ((command == "STATUS")or(OK)) {
                if(digitalRead(D1) == HIGH )
                   StatusPin = "[1"; 
                else
                   StatusPin = "[0"; 
                
                if(digitalRead(D2) == HIGH )
                   StatusPin = StatusPin+",1]"; 
                else
                   StatusPin = StatusPin+",0]"; 

                OK = true;
            }  

            

            if(OK){
                 // if(command == "STATUS")                  
                    webSocket.sendTXT("{\"Name\":\""+String(WiFi.macAddress())+"\",\"Status\":"+StatusPin+"}");
                //  else
                //    webSocket.sendTXT("{\"Name\":\"Esp-12e\",\"Status\":\"Ok\"}");
                  
            }else{
                //  char json[] =    "{\"Name\":\""+String(WiFi.macAddress())+"\",\"Status\":\"Not suport command\"}";
                  webSocket.sendTXT("{\"Name\":\""+String(WiFi.macAddress())+"\",\"Status\":\"Not suport command\"}");
            
            }
            
  }


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

	switch(type) {
		case WStype_DISCONNECTED:
			USE_SERIAL.printf("[WSc] Disconnected!\n");
			break;
		case WStype_CONNECTED: {
			USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);

			// send message to server when Connected
			//webSocket.sendTXT("Connected");

      //  char json[] = "{\"Name\":\"Esp-12e\",\"Status\":\"Run\"}";        

      //  webSocket.sendTXT(json);
          runCommand("STATUS");
		}
			break;
		case WStype_TEXT:{
			String text = (char*) payload;
			USE_SERIAL.printf("[WSc] get text: %s\n", payload);

      JsonObject& root = jsonBuffer.parseObject(text);

    if (!root.success()){
        USE_SERIAL.println("Error message");

       // char json[] =  "{\"Name\":\"Esp-12e\",\"Starus\":\"Err\"}";        

      //  webSocket.sendTXT("{\"Name\":\""+String(WiFi.macAddress())+"\",\"Status\":\"Error\"}");
        }
        
    else{
        if (root["Name"] == "Controller"){
          if (root["Client"] == String(WiFi.macAddress()) || root["Client"] == "ALL")
          { 
            String command = root["Command"];
            runCommand(command);
          }
        }          
    }
		}
			break;
		case WStype_BIN:
			USE_SERIAL.printf("[WSc] get binary length: %s\n", length);
			hexdump(payload, length);

			// send data to server
			// webSocket.sendBIN(payload, length);
			break;

    case WStype_ERROR:
			USE_SERIAL.printf("[WSc] Error connection: %s\n", payload);
	
			// send data to server
			// webSocket.sendBIN(payload, length);
			break;
	}

}

void setup() {
	
	EEPROM.begin(2);
	
	// USE_SERIAL.begin(921600);
	USE_SERIAL.begin(115200);

	//Serial.setDebugOutput(true);
	USE_SERIAL.setDebugOutput(true);

	USE_SERIAL.println();
	USE_SERIAL.println();
	USE_SERIAL.println();

	for(uint8_t t = 4; t > 0; t--) {
		USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
		USE_SERIAL.flush();
		delay(1000);
	}

   USE_SERIAL.print("MAC: ");
   USE_SERIAL.println(WiFi.macAddress());
   
  //Читаем состояние пинов после аварийного отключения
  if(EEPROM.read(0) == 1)
    digitalWrite(D1, HIGH);
  else  
	  digitalWrite(D1, LOW);

  if(EEPROM.read(1) == 1)
    digitalWrite(D2, HIGH);
  else  
    digitalWrite(D2, LOW);

  WiFi.mode(WIFI_STA);
	WiFiMulti.addAP("router_it", "0472454294");

	//WiFi.disconnect();
	while(WiFiMulti.run() != WL_CONNECTED) {
		delay(100);
	}


 // USE_SERIAL.println(WiFiMulti.macAddress());
	// server address, port and URL
	webSocket.begin("192.168.5.44", 81, "/");

	// event handler
	webSocket.onEvent(webSocketEvent);

	// use HTTP Basic Authorization this is optional remove if not needed
//	webSocket.setAuthorization("user", "Password");

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);
  //Двухканальное реле
  pinMode(D1, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(D2, OUTPUT);     // Initialize the LED_BUILTIN pin as an output

}

void loop() {

	webSocket.loop();
  // здесь будет код, который будет работать постоянно
  // и который не должен останавливаться на время между переключениями свето
  unsigned long currentMillis = millis();
    //проверяем не прошел ли нужный интервал, если прошел то
  if(currentMillis - previousMillis > interval) {
    // сохраняем время последнего переключения
    previousMillis = currentMillis; 
      runCommand("STATUS");
    // если светодиод не горит, то зажигаем, и наоборот
   
    // устанавливаем состояния выхода, чтобы включить или выключить светодиод
  }

  if(currentMillis > interval_restart) {
      //Перезапускаем ESP 
      ESP.restart();
    
  }
 
}
