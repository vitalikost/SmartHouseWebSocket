#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <WebSocketsServer.h>
#include <Hash.h>
 
String str = "";
boolean conf = false;
 
String html_header = "<html>\
 <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\
 <head>\
   <title>ESP8266 Settings</title>\
   <style>\
     body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
   </style>\
 </head>";
 
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        
        // send message to client
        //webSocket.sendTXT(num, "Connected");
            }
            break;
        case WStype_TEXT:
            Serial.printf("[%u] get Text: %s\n", num, payload);

            // send message to client
            // webSocket.sendTXT(num, "message here");

            // send data to all connected clients
             webSocket.broadcastTXT(payload);
            break;
        case WStype_BIN:
            Serial.printf("[%u] get binary length: %u\n", num, length);
            hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    }

}

void setup(void)
{
    byte len_ssid, len_pass;
   
    delay(1000);
    Serial.begin(115200);
    Serial.println("start");  


    EEPROM.begin(98);
    len_ssid = EEPROM.read(96);
    len_pass = EEPROM.read(97);
    if(len_pass > 64) len_pass = 0;
     if((len_ssid < 33) && (len_ssid != 0)){
        // Режим STATION
          WiFi.mode( WIFI_STA);
          unsigned char* buf_ssid = new unsigned char[32];
          unsigned char* buf_pass = new unsigned char[64];
          for(byte i = 0; i < len_ssid; i++) buf_ssid[i] = char(EEPROM.read(i));
          buf_ssid[len_ssid] = '\x0';
          const char *ssid  = (const char*)buf_ssid;
          for(byte i = 0; i < len_pass; i++) buf_pass[i] = char(EEPROM.read(i + 32));
          const char *pass  = (const char*)buf_pass;
          buf_pass[len_pass] = '\x0';
         
          Serial.print("SSID: ");
          Serial.print(ssid);
          Serial.print("   ");
          Serial.print("Password: ");
          Serial.println(pass);
         
          WiFi.begin(ssid, pass);
          // Wait for connection
          int start = 0;
          while ( WiFi.status() != WL_CONNECTED ) {
            delay ( 500 );
            Serial.print ( "." );
            start = start +1;
            if(start > 100 )
            {
              
              start_server();
              break;
            }
            
          }
          
          if(WiFi.status() == WL_CONNECTED)
          {
            Serial.println();
            Serial.print("Connected to ");
            Serial.println(ssid);
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());    
            //---------------------------
            WiFi.softAP("ESP-Server", "");
            delay(2000);      
            IPAddress myIP = WiFi.softAPIP();
            Serial.print("AP IP address: ");
            Serial.println(myIP);
            server.on("/", handleRootStatus);            
            server.begin();
            Serial.println("HTTP server started"); 
          }

    }
    else // Режим SoftAP
      {
        start_server();
      }  
  
  
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
 
Serial.println("stop");     
}
void loop() {
  server.handleClient();
  webSocket.loop();
}
 
void handleRootStatus() {
  String str = "";
  String str_ip = "";
  IPAddress ip = WiFi.localIP();
  str_ip = String(ip[0])+"." +String(ip[1])+"."+String(ip[2])+"."+String(ip[3]);
  str += html_header;
  str += "<body>\
      <H1>Server Smart House</H1>\
      <p>ip -"+str_ip+     
 "</body>\
</html>";
server.send ( 200, "text/html", str );
}

void handleRoot() {
  String str = "";
  str += html_header;
  str += "<body>\
   <form method=\"POST\" action=\"ok\">\
     <input name=\"ssid\"> WIFI Net</br>\
     <input name=\"pswd\"> Password</br></br>\
     <input type=SUBMIT value=\"Save settings\">\
   </form>\
 </body>\
</html>";
server.send ( 200, "text/html", str );
}
 
void handleOk(){
  String ssid_ap;
  String pass_ap;
  unsigned char* buf = new unsigned char[64];
 
  String str = "";
  str += html_header;
  str += "<body>";
 
  EEPROM.begin(98);
 
  ssid_ap = server.arg(0);
  pass_ap = server.arg(1);
 
  if(ssid_ap != ""){
    EEPROM.write(96,ssid_ap.length());
    EEPROM.write(97,pass_ap.length());
    ssid_ap.getBytes(buf, ssid_ap.length() + 1);
    for(byte i = 0; i < ssid_ap.length(); i++)
      EEPROM.write(i, buf[i]);
    pass_ap.getBytes(buf, pass_ap.length() + 1);
    for(byte i = 0; i < pass_ap.length(); i++)
      EEPROM.write(i + 32, buf[i]);
    EEPROM.commit();
    EEPROM.end();
   
    str +="Configuration saved in FLASH</br>\
   Changes applied after reboot</p></br></br>\
   <a href=\"/\">Return</a> to settings page</br>";
  }
  else {
    str += "No WIFI Net</br>\
   <a href=\"/\">Return</a> to settings page</br>";
  }
  str += "</body></html>";
  server.send ( 200, "text/html", str );
}

void start_server()
{
       const char *ssid_ap = "ESPap";
       
        WiFi.mode(WIFI_AP);
       
        Serial.print("Configuring access point...");
    /* You can remove the password parameter if you want the AP to be open. */
    WiFi.softAP(ssid_ap);
 
     delay(2000);
    Serial.println("done");
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    server.on("/", handleRoot);
        server.on("/ok", handleOk);
    server.begin();
    Serial.println("HTTP server started"); 
}
 
