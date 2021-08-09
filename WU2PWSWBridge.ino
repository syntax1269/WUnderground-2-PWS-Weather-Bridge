

/*
 * Designed for use with ESP01 ESP8266 with 1mb flash or more.
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include <ArduinoJson.h>  // https://arduinojson.org/v6/example/parser/

#include "uptime.h" // calculats uptime and handles roll-over of mills 

//----- OTA ---------
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
//-----------------------

//----- Variables --------
char* ver = ("2.5");
char* LngPrdName = "";
char* localESPname = "WU2PWSWBridge1M_";
char* strApplicationName = "WU2PWSWBridge "; //The name of your application or the application generating the event.
const char* passphrase = "123456789"; //Default SSID pass for initial connection.
String string_IP="";//temporarily stores a IP group number
int ip_group=0;//sets what number set of the IP address we are collecting. 5 in total
bool wifistat; // Current status of WiFi
String myssid; // Stores SSID to connect to
int rssi;  //WiFi Signal strength
unsigned int APModeLoopCnt = 0;
bool boolSoftAPEnabled = false;


//----- Weather Configs----------
bool checkssl = false;  // If you need to check the fingerprint set to true
String WUJSON; //JSON Retrived from Weather Underground
WiFiClientSecure client;// For HTTPS requests
#define WU_HOST "api.weather.com" // Just the base of the URL you want to connect to
#define WU_HOST_FINGERPRINT "BD 02 01 B4 24 50 33 63 90 7E AF B3 C3 4E 68 A1 37 2E 23 33"  //needs updating every so offten
  String WU_APIKEY = "";
  String WU_STATIONID = "";
#define PWSUPDATE_HOST "pwsupdate.pwsweather.com" // PWS Weather Update Host
#define pwsupdate_fingerprint "9F EC 59 DB 66 89 9C 87 52 21 B0 F6 AC 71 11 B5 67 E3 D4 59" //needs updating every so offten
String PWSW_APIKEY = "";
String PWSW_STATIONID = "";
String buildURL = "";
String PWSWResult = "";

//----- Device Configs -------
#define timechkwifi 10000 // time to check wifi status (10 seconds)
 ESP8266WebServer server(80); // Webserver port
 WiFiUDP udp;  // A UDP instance to let us send and receive packets over UDP

bool loadFromSpiffs(String path){
  String dataType = "text/plain";
//  if(path.endsWith("/")) path += "index.htm";
  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
//  else if(path.endsWith(".html")) dataType = "text/html";
//  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js")) dataType = "application/javascript";
//  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
//  else if(path.endsWith(".xml")) dataType = "text/xml";
//  else if(path.endsWith(".pdf")) dataType = "application/pdf";
//  else if(path.endsWith(".zip")) dataType = "application/zip";
  File dataFile = SPIFFS.open(path.c_str(), "r");
  if (server.hasArg("download")) dataType = "application/octet-stream";
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
  }
  dataFile.close();
  return true;
}


void handleWebRequests() {
  if(loadFromSpiffs(server.uri())) return;
  String msg = "File Not Detected\n\n";
  msg += "URI: ";
  msg += server.uri();
  msg += "\nMethod: ";
  msg += (server.method() == HTTP_GET)?"GET":"POST";
  msg += "\nArguments: ";
  msg += server.args();
  msg += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    msg += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
    msg += "File Not Detected\n\n";
  server.send(404, "text/plain", msg);
}

// +-+-+-***********************************************************-+-+-+
// +-+-+-***********************************************************-+-+-+
// +-+-+-***********************************************************-+-+-+


void initalizeDevices() { // Place all devices that need to initalized here they will be run inline


} // END init Devices

// +-+-+-***********************************************************-+-+-+
// +-+-+-***********************************************************-+-+-+
// +-+-+-***********************************************************-+-+-+

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  EEPROM.begin(512);
  delay(500);

  pinMode(LED_BUILTIN, OUTPUT);
  configTime(-5 * 3600, 1 * 3600, "10.9.50.1", "pool.ntp.org", "time.nist.gov");
  String esid;
  for (int i = 0; i < 32; ++i)
    {
     esid += char(EEPROM.read(i));
    }
      myssid = esid;
  String epass = "";
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }

  string_IP="";
  for (int i = 97; i < 116; ++i)
    {
      string_IP += char(EEPROM.read(i));
    }

   int Parts[4] = {0,0,0,0};
   int Part = 0;

  //Reading IP information out of memmory
    int ip_Info[5];//holds all converted IP integer information
    for (int n = 97; n < 116; ++n)
    {
     if(char(EEPROM.read(n))!='.'&& char(EEPROM.read(n))!=';'){//
      string_IP += char(EEPROM.read(n));
     }else if(char(EEPROM.read(n))=='.'){//if(char(EEPROM.read(n))=='.')
        ip_Info[ip_group]=ip_convert(string_IP);
        ip_group++;
     }else if(char(EEPROM.read(n))==';'){
        ip_Info[ip_group]=ip_convert(string_IP);
        ip_group++;
        n=117;
     }
  } 

    ip_Info[ip_group]=ip_convert(string_IP); 
  
   //configuring conection parameters, and connecting to the WiFi router
  IPAddress  eeIP(ip_Info[0], ip_Info[1], ip_Info[2], ip_Info[3]); // defining ip address

   String eMask;
   for (int i = 117; i < 136; ++i)
    {
      eMask += char(EEPROM.read(i));
    }
//Reading IP information out of memmory
   string_IP="";
   ip_group=0;
   memset(ip_Info, 0, sizeof(ip_Info));//holds all converted IP integer information
    for (int n = 117; n < 136; ++n)
    {
     if(char(EEPROM.read(n))!='.'&& char(EEPROM.read(n))!=';'){//
      string_IP += char(EEPROM.read(n));
     }else if(char(EEPROM.read(n))=='.'){//if(char(EEPROM.read(n))=='.')
        ip_Info[ip_group]=ip_convert(string_IP);
        ip_group++;
     }else if(char(EEPROM.read(n))==';'){
        ip_Info[ip_group]=ip_convert(string_IP);
        ip_group++;
        n=137;
     }
  } 

//configuring conection parameters, and connecting to the WiFi router
    IPAddress  eeMask(ip_Info[0], ip_Info[1], ip_Info[2], ip_Info[3]); // defining ip address
       
   String eGW;
   string_IP="";
   ip_group=0;
   memset(ip_Info, 0, sizeof(ip_Info));
   for (int i = 137; i < 156; ++i)
    {
      eGW += char(EEPROM.read(i));
    }

//Reading IP information out of memmory
   string_IP="";
   ip_group=0;
   memset(ip_Info, 0, sizeof(ip_Info));//holds all converted IP integer information
    for (int n = 137; n < 156; ++n)
    {
     if(char(EEPROM.read(n))!='.'&& char(EEPROM.read(n))!=';'){//
      string_IP += char(EEPROM.read(n));
     }else if(char(EEPROM.read(n))=='.'){
        ip_Info[ip_group]=ip_convert(string_IP);
        ip_group++;
     }else if(char(EEPROM.read(n))==';'){
        ip_Info[ip_group]=ip_convert(string_IP);
        ip_group++;
        n=157;
     }
  } 

    ip_Info[ip_group]=ip_convert(string_IP);
//configuring conection parameters, and connecting to the WiFi router
    IPAddress  eeGW(ip_Info[0], ip_Info[1], ip_Info[2], ip_Info[3]); // defining ip address

   String eDNS;
   string_IP="";
   ip_group=0;
   memset(ip_Info, 0, sizeof(ip_Info));
   for (int i = 157; i < 176; ++i)
    {
      eDNS += char(EEPROM.read(i));
    }
//Reading IP information out of memmory
   string_IP="";
   ip_group=0;
   memset(ip_Info, 0, sizeof(ip_Info));//holds all converted IP integer information
    for (int n = 157; n < 176; ++n)
    {
     if(char(EEPROM.read(n))!='.'&& char(EEPROM.read(n))!=';'){//
      string_IP += char(EEPROM.read(n));
     }else if(char(EEPROM.read(n))=='.'){
        ip_Info[ip_group]=ip_convert(string_IP);
        ip_group++;
     }else if(char(EEPROM.read(n))==';'){
        ip_Info[ip_group]=ip_convert(string_IP);
        ip_group++;
        n=177;
     }
  } 

    ip_Info[ip_group]=ip_convert(string_IP);
//configuring conection parameters, and connecting to the WiFi router
    IPAddress  eeDNS(ip_Info[0], ip_Info[1], ip_Info[2], ip_Info[3]); // defining ip address

  String eDHCP = "";
      eDHCP = char(EEPROM.read(200));

   for (int i = 201; i < 233; ++i)
    {
      WU_APIKEY += char(EEPROM.read(i));
    }


   for (int i = 234; i < 266; ++i)
    {
       PWSW_APIKEY += char(EEPROM.read(i));
    }

    for (int i = 267; i < 277; ++i)
    {
      WU_STATIONID += char(EEPROM.read(i));
    }

    for (int i = 278; i < 288; ++i)
      {
        PWSW_STATIONID += char(EEPROM.read(i));
      }
   
  localESPname = "WU2PWSWBridge1M_";

  strcat(localESPname,WiFi.macAddress().substring(12,14).c_str()); // Append MAC Address to Local Host Name
  strcat(localESPname,WiFi.macAddress().substring(15,17).c_str()); // Append MAC Address to Local Host Name
Serial.println(localESPname);
initalizeDevices();


  if ( esid.length() > 1 ) {
      // add DHCP check here 
      if ( eDHCP == "N" ) {
          WiFi.hostname(String(localESPname));
        WiFi.begin(esid.c_str(), epass.c_str());
          WiFi.config(eeIP, eeGW, eeMask, eeDNS);  
        } else {
            WiFi.hostname(String(localESPname));
          WiFi.begin(esid.c_str(), epass.c_str());
          }

  ArduinoOTA.setHostname(localESPname);
      
      if (testWifi()) {
        wifistat = true;

//----------------- OTA ----------------------

 // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
  });

  ArduinoOTA.begin();
  launchWeb(0);
  return;
      } 
  }
setupAP();

} // setup END


bool testWifi(void) {
  unsigned long counter = 0;
  unsigned long progress = (counter * 100) / 20;
  unsigned long start = millis();
  while ( counter < 20 ) {
    if (WiFi.status() == WL_CONNECTED) { return true; } 
    unsigned long start = millis();
    while( millis() - start < 500){ // let go by without impacting other cpu functions
       yield();
     }
  counter++; 
  progress = (counter * 100) / 20;
  Serial.print(String(progress));
  Serial.print("%  ");
    }
  Serial.println("");
  Serial.println(F("Connect timed out! Starting ESP in AP mode."));
  return false;
  wifistat = false;
}

void launchWeb(int webtype) {

    if (wifistat) {
       Serial.print("ESP Configured IP: ");Serial.println(WiFi.localIP());
       WiFi.mode(WIFI_STA);
       WiFi.enableAP(false);
               if (MDNS.begin(localESPname)) {             // Start the mDNS responder
      MDNS.addService("http", "tcp", 80);
     }
       MDNS.update();
       if (checkssl){
    client.setFingerprint(WU_HOST_FINGERPRINT);
       makeHTTPRequest();
    client.setFingerprint(pwsupdate_fingerprint);
      SendDataToPWSWeather ();
       } else {
         
        client.setInsecure();
        makeHTTPRequest();
        SendDataToPWSWeather ();
       }
            }
            else {
                WiFi.mode(WIFI_AP);
                WiFi.enableAP(true);
                MDNS.update();
                }

        createWebServer(webtype);
        // Start the server
        server.begin();
}

void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  unsigned long start = millis();
  while( millis() - start < 200){ // let go by without impacting other cpu functions
   yield();
   }
    if (wifistat) {
    }
    else {
        WiFi.softAP(localESPname, passphrase);
    }
  launchWeb(1);
  boolSoftAPEnabled = true;
}

void createWebServer(int webtype)
{
  if ( webtype == 1 ) {

    server.on("/", []() {
        String cnt;
        int n = WiFi.scanNetworks();
        cnt = "<!DOCTYPE HTML>\r\n<html>Hello from " + String(strApplicationName) + " " + " IP Address: ";
        cnt += WiFi.localIP().toString();
        cnt += "<p>";
        cnt += "MAC Address: ";
        cnt += WiFi.macAddress();
        cnt += "<br>Currently connected to WiFi network: ";
        cnt += myssid;
        cnt += "<p>";
   cnt += "<ol>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      cnt += "<li><form method='get' action='setting'><input type='hidden' id='ssid' name='ssid' value='";
      cnt += WiFi.SSID(i);
      cnt += "'>" +  WiFi.SSID(i) + " (";
      cnt += WiFi.RSSI(i);
      cnt += ")";
      cnt += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
      cnt += "Password: <input type='text' name='pass' length=64><input type='submit' value='Connect'></form></li>";
    }
  cnt += "</ol>";
        cnt += "</p><form method='get' action='setting'><label>Hidden WiFi SSID: </label><input name='ssid' length=32> Password: <input name='pass' length=64><input type='submit' value='Connect'></form>";
        cnt += "</html>";
        server.send(200, "text/html", cnt);  
    });
    server.on("/setting", []() {
      String cnt;
        String qsid = server.arg("ssid");
        String qpass = server.arg("pass");
        if (qsid.length() > 0 && qpass.length() > 0) {
//          Serial.println(F("clearing eeprom"));
          for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
          for (int i = 0; i < qsid.length(); ++i)
            {
              EEPROM.write(i, qsid[i]);
            }
          for (int i = 0; i < qpass.length(); ++i)
            {
              EEPROM.write(32+i, qpass[i]);
            }
          EEPROM.commit();
          
         cnt = "<!DOCTYPE HTML>\r\n<html>";
         cnt += "<meta http-equiv=refresh content=60;URL='http://";
         cnt += localESPname;
         cnt += ".local";
         cnt += "/'><html>";
         cnt += "{\"Success\":\"WiFi settings saved to memory... ESP will reboot in 5 seconds and connect into new wifi\"}";
         cnt += "<br> This page will refresh in 1 minute. Please wait...";
         cnt += "<br>If this page doesn't refresh, you can try to access the page http://";
         cnt += localESPname;
         cnt += ".local/";
         cnt += "</html>";
        } 
        server.send(200, "text/html", cnt);
        unsigned long start = millis();
        while( millis() - start < 2*1000){ // let go by without impacting other cpu functions
             yield();
         }
        ESP.restart(); 
    });
  } else if (webtype == 0) {

    server.onNotFound(handleWebRequests); //Set setver all paths are not found so we can handle as per URI

server.on("/", []()  {
  String cnt="";
cnt = "<!DOCTYPE HTML>\r\n<html>";
cnt += "<meta http-equiv=refresh content=15>";
cnt += LngPrdName;
cnt += "<br>";
cnt += "<br>WiFi MAC Address: ";

cnt += WiFi.macAddress();
cnt += "<br>Current WiFi network: <b>";
cnt += myssid;
cnt += "</b>";
cnt += " <br>Current IP Address: " + WiFi.localIP().toString();
cnt += "<br> Current RSSI: ";
cnt += rssi;
cnt += " dB";
cnt += "<br>Up time: ";
cnt += uptime::getDays();
cnt += " days, ";
cnt += uptime::getHours();
cnt += " hours, ";
cnt += uptime::getMinutes();
cnt += " minutes, ";
cnt += uptime::getSeconds();
cnt += " seconds";
cnt += "<br><br>This page can also be accessed by the URL: ";
cnt += "<br><a href=http://";
cnt += localESPname;
cnt += ".local/>http://";
cnt += localESPname;
cnt += ".local/</a>";
cnt += "<br><br>Current Local Time/Date: ";
    time_t now = time(NULL);
cnt += ctime(&now);
cnt += "<br><hr><hr><br>";
  DynamicJsonDocument doc(950);
  DeserializationError error = deserializeJson(doc, WUJSON);

cnt += "Current Sensor Status: ";
cnt += "<br>";
  // Test if parsing succeeds.
  if (error) {
     cnt += "No data retrived from WeatherUnderground.<br>";
     cnt += "deserializeJson() failed: ";
     cnt += error.f_str();
     cnt += "<br>";
    } else {
cnt += "WU UTC Time: ";
String valUTC = doc["observations"][0]["obsTimeUtc"];
      valUTC.replace("T", " - ");
      valUTC.replace("Z", "");
cnt += valUTC;
cnt += "<br>Temperature: ";
cnt += double(doc["observations"][0]["imperial"]["temp"]);

cnt += "<br>Wind Direction: ";
cnt += int(doc["observations"][0]["winddir"]);
cnt += " ";
cnt += getHeading(int(doc["observations"][0]["winddir"]));

cnt += "<br>Wind Speed: ";
cnt += int(doc["observations"][0]["imperial"]["windSpeed"]);
cnt += " MPH";

cnt += "<br>Wind Gust: ";
cnt += int(doc["observations"][0]["imperial"]["windGust"]);
cnt += " MPH";

cnt += "<br>Hourly rain in inches: ";
cnt += double(doc["observations"][0]["imperial"]["precipRate"]);

cnt += "<br>Daily rain in inches: ";
cnt += double(doc["observations"][0]["imperial"]["precipTotal"]);

cnt += "<br>Barometric pressure: ";
cnt += double(doc["observations"][0]["imperial"]["pressure"]);

cnt += "<br>Humidity: ";
cnt += int(doc["observations"][0]["humidity"]);
cnt += "%";

cnt += "<br>Dew point: ";
cnt += double(doc["observations"][0]["imperial"]["dewpt"]);

cnt += "<br>UV: ";
cnt += int(doc["observations"][0]["uv"]);
    }


    cnt += "<br><hr>JSON received from WU:<br>";
    cnt += WUJSON;
    cnt += "<br><br>URL Sent to PWS Weather:<br>";
    cnt += buildURL;
    cnt += "<br><br>Response JSON from PWS Weather:<br>";
    cnt += PWSWResult;
    cnt+= "<br><hr>";
    cnt += "<br><br><form method=get action=/setting><button type=submit>Config WiFi</button></form>";
    cnt += "<br><form method=get action=/settingIP><button type=submit>Config IP Address</button></form>";
    cnt += "<br><form method=get action=/setwuapikey><button type=submit>Config Weather Underground</button></form>";
    cnt += "<br><form method=get action=/setpwswapikey><button type=submit>Config PWS Weather</button></form>";
    cnt += "<br><form method=get action=/rebootesp><button type=submit>Reboot Sensor</button></form>";
    cnt += "</p></html>";

server.send(200, "text/html", cnt);
});

   server.on("/rebootesp", []()  {
    String cnt;
          cnt = "<!DOCTYPE HTML>\r\n<html>";
          cnt += "<meta http-equiv=refresh content=45;URL='http://";
          cnt += localESPname;
          cnt += ".local";
          cnt += "/'><html><h2 style=\"border:5px solid Green;\">...REBOOTING SENSOR...</h2></html>";
      server.send(200, "text/html", cnt);
      unsigned long start = millis();
      while( millis() - start < 2*1000){ // let 2 seconds go by without impacting other cpu functions
           yield();
      }
      ESP.restart();
   });



   server.on("/setwuapikey", []() {
    String cnt;
           cnt = "<!DOCTYPE HTML>\r\n<html>" + String(strApplicationName) + "<br><br> Weather Underground Station ID and API KEY";
        cnt += "<p><br>Current WiFi network: <b>";
        cnt += myssid;
        cnt += "</b></p><form method='get' action='commitwuapikey'>";
        cnt += "<label>WU Station ID: </label><input name='WUSTAID' length=22 value=";
        cnt += WU_STATIONID;
        cnt += "><br><label>WU API KEY: </label><input name='WUAPIKEY' length=32 value=";
        cnt += WU_APIKEY;       
        cnt += "><br><input type='submit' value='Commit'></form></p></html>";
        server.send(200, "text/html", cnt);  
    }); 

   server.on("/commitwuapikey", []() {
    String cnt;
    String qwuapikey = server.arg("WUAPIKEY");
    String qwustaid = server.arg("WUSTAID");
       if (WU_STATIONID != qwustaid){
              for (int i = 0; i < qwustaid.length(); ++i)
            {
              EEPROM.write(267+i, qwustaid[i]);
            }
          EEPROM.commit();
          WU_STATIONID = qwustaid;
       }
      if (WU_APIKEY != qwuapikey){
              for (int i = 0; i < qwuapikey.length(); ++i)
            {
              EEPROM.write(201+i, qwuapikey[i]);
            }
          EEPROM.commit();
          WU_APIKEY = qwuapikey;
      }
         cnt = "<!DOCTYPE HTML>\r\n<html>";
         cnt += "<meta http-equiv=refresh content=8;URL='http://";
         cnt += localESPname;
         cnt += ".local";
         cnt += "/'><html><br>Success! WU STATION ID saved to memory...";
         cnt += "<br>WU STATION ID: ";
         cnt += WU_STATIONID;         
         cnt += "<br>Success! WU API KEY saved to memory...";
         cnt += "<br>WU API KEY: ";
         cnt += WU_APIKEY;
         cnt += "</html>";
   server.send(200, "text/html", cnt); 
    });

    
   server.on("/setpwswapikey", []() {
    String cnt;
           cnt = "<!DOCTYPE HTML>\r\n<html>" + String(strApplicationName) + " PWS WEATHER API KEY";
        cnt += "<p><br>Current WiFi network: <b>";
        cnt += myssid;
        cnt += "</b></p><form method='get' action='commitpwswapikey'>";
        cnt += "<label>PWS Weather Station ID: </label><input name='PWSWSTATIONID' length=12 value=";
        cnt += PWSW_STATIONID;        
        cnt += "><br><label>API KEY: </label><input name='PWSWAPIKEY' length=42 value=";
        cnt += PWSW_APIKEY;
        cnt += "><br><input type='submit' value='Commit'></form></p></html>";
        server.send(200, "text/html", cnt);  
    }); 


   server.on("/commitpwswapikey", []() {
    String cnt;
    String qPWSWSTATIONID = server.arg("PWSWSTATIONID");
       if (PWSW_STATIONID != qPWSWSTATIONID){
              for (int i = 0; i < qPWSWSTATIONID.length(); ++i)
            {
              EEPROM.write(278+i, qPWSWSTATIONID[i]);
            }
          EEPROM.commit();
          PWSW_STATIONID = qPWSWSTATIONID;
       }
    
    String qpwswapikey = server.arg("PWSWAPIKEY");
       if (PWSW_APIKEY != qpwswapikey){
              for (int i = 0; i < qpwswapikey.length(); ++i)
            {
              EEPROM.write(234+i, qpwswapikey[i]);
            }
          EEPROM.commit();
          PWSW_APIKEY = qpwswapikey;
       }
         cnt = "<!DOCTYPE HTML>\r\n<html>";
         cnt += "<meta http-equiv=refresh content=8;URL='http://";
         cnt += localESPname;
         cnt += ".local";
         cnt += "/'><html>";
         cnt += "<br>Success! PWS Weather Station ID saved to memory...";
         cnt += "<br>PWS Weather Station ID: ";
         cnt += PWSW_STATIONID;
         cnt += "<br>Success! PWS Weather API KEY saved to memory...";
         cnt += "<br>PWS Weather API KEY: ";
         cnt += PWSW_APIKEY;
         cnt += "</html>";
   server.send(200, "text/html", cnt); 
    });
    
   server.on("/settingIP", []() {
    String cnt;
        cnt = "<!DOCTYPE HTML>\r\n<html>" + String(strApplicationName) + " IP setting";
        cnt += "<p>";
        cnt += "<br>Current WiFi network: <b>";
        cnt += myssid;
        cnt += "</b>";
        cnt += "</p><form method='get' action='commitIPsetting'>";
        cnt += "<br><input type=\"checkbox\" name=\"DHCP\" value=\"Y\" checked> Enable DHCP<br>";
        cnt += "<label>IP: </label><input name='IP' length=32 value=";
        cnt += WiFi.localIP().toString();
        cnt += ">";
        cnt += "<br>Gateway: <input name='GW' length=16 value=";
        cnt += WiFi.gatewayIP().toString();
        cnt += ">";
        cnt += "<br>Netmask: <input name='MASK' length=16 value=";
        cnt += WiFi.subnetMask().toString();
        cnt += ">";
        cnt += "<br>DNS: <input name='DNS' length=16 value=";
        cnt += WiFi.dnsIP().toString();
        cnt += ">";
        cnt += "<br><input type='submit' value='Commit'></form>";
        cnt += "</p></html>";
        server.send(200, "text/html", cnt);  
    });     

server.on("/commitIPsetting", []() {
        String qIP = server.arg("IP");
        String qGW = server.arg("GW");
        String qMASK = server.arg("MASK");
        String qDNS = server.arg("DNS");    
        String qDHCP = server.arg("DHCP");
        for (int i = 97; i < 200; ++i) { EEPROM.write(i, 0); }
          for (int i = 0; i < qIP.length(); ++i)
            {
              EEPROM.write(97+i, qIP[i]);
            }
EEPROM.commit();
          for (int i = 0; i < qMASK.length(); ++i)
            {
              EEPROM.write(117+i, qMASK[i]);
            }
EEPROM.commit();
          for (int i = 0; i < qGW.length(); ++i)
            {
              EEPROM.write(137+i, qGW[i]);
            }
EEPROM.commit();
          for (int i = 0; i < qDNS.length(); ++i)
            {
              EEPROM.write(157+i, qDNS[i]);
            }
EEPROM.commit();
 
              if (qDHCP == "E"){
                for (int i = 0; i < qDHCP.length(); ++i)
            {
              EEPROM.write(200+i, qDHCP[i]);
            }
                } else {
                  qDHCP = "N";
                for (int i = 0; i < qDHCP.length(); ++i)
            {
              EEPROM.write(200+i, qDHCP[i]);
                  }}
          EEPROM.commit();
          String cnt;
         cnt = "<!DOCTYPE HTML>\r\n<html>";
         cnt += "<meta http-equiv=refresh content=8;URL='http://";
         cnt += qIP;
         cnt += "/'><html>";
         cnt += "Success! IP settings saved to memory... ESP will reboot in 5 seconds and connect into new wifi";
         cnt += "";
         cnt += "</html>";
         
         server.send(200, "text/html", cnt);
         unsigned long start = millis();
         while( millis() - start < 2*1000){ // let 2 seconds go by without impacting other cpu functions
             yield();
         }
          ESP.restart(); 
    });


server.on("/settingcommit", []() {
  String cnt;
        String qsid = server.arg("ssid");
        String qpass = server.arg("pass");
        
        if (qsid.length() > 0 && qpass.length() > 0) {
          for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
          for (int i = 0; i < qsid.length(); ++i)
            {
              EEPROM.write(i, qsid[i]);
            }
          for (int i = 0; i < qpass.length(); ++i)
            {
              EEPROM.write(32+i, qpass[i]);
            }    
          EEPROM.commit();
         String cnt;
         cnt = "<!DOCTYPE HTML>\r\n<html>";
         cnt += "<meta http-equiv=refresh content=8;URL='http://";
         cnt += localESPname;
         cnt += ".local/";
         cnt += "/'><html>";
         cnt += "{\"Success\":\"WiFi settings saved to memory... Rebooting\"}";
         cnt += "";
         cnt += "</html>";
        } 
        server.send(200, "text/html", cnt);
          
         unsigned long start = millis();
         while( millis() - start < 2*1000){ // let 2 seconds go by without impacting other cpu functions
             yield();
         }
          ESP.restart();
    });

server.on("/setting", []() {
  String cnt;
  int n = WiFi.scanNetworks();
        cnt = "<!DOCTYPE HTML>\r\n<html>Hello from " + String(strApplicationName);
        cnt += WiFi.localIP().toString();
        cnt += "<p>";
        cnt += "Currently connected to WiFi network: <b>";
        cnt += myssid;
        cnt += "</b><p>";        
        cnt += "<ol>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      cnt += "<li><form method='get' action='settingcommit'><input type='hidden' id='ssid' name='ssid' value='";
      cnt += WiFi.SSID(i);
      if (myssid == WiFi.SSID(i) ){
         cnt += "'><b>" +  WiFi.SSID(i) + " (";
         cnt += WiFi.RSSI(i);
         cnt += ")</b>";
      } else {
         cnt += "'>" +  WiFi.SSID(i) + " (";
         cnt += WiFi.RSSI(i);
         cnt += ")";
      }
        cnt += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
        cnt += "Password: <input type='text' name='pass' length=64><input type='submit' value='Connect'></form></li>";
    }
        cnt += "</ol>";
        cnt += "</p><form method='get' action='settingcommit'><label>Hidden WiFi SSID: </label><input name='ssid' length=32> Password: <input name='pass' length=64><input type='submit' value='Connect'></form>";
        cnt += "</html>";
        server.send(200, "text/html", cnt);  
    });
  }

  } // END WEB TYPE 0

void loop() {
    server.handleClient();
    yield(); // Give some time for ESP to process
    ArduinoOTA.handle();
    rssi = WiFi.RSSI();
    uptime::calculateUptime();  
    static int last = 0;
    static int lstwificheck = 0;
    if ((millis() - lstwificheck) > timechkwifi){
      lstwificheck = millis();
      CheckWiFiStatus();
      MDNS.update();
    }
  static int lastupdate;

  if ((millis()-lastupdate) > (5*60*1000)){    //5 Minutes interval
    lastupdate = millis();
    
       if (checkssl){
    client.setFingerprint(WU_HOST_FINGERPRINT);
       makeHTTPRequest();
    client.setFingerprint(pwsupdate_fingerprint);
      SendDataToPWSWeather ();
       } else {
         // If you don't need to check the fingerprint
        client.setInsecure();
        makeHTTPRequest();
        SendDataToPWSWeather ();
       }
      Serial.println(F("---------------"));
      Serial.println(F("5 Minute Update"));
      Serial.println(F("---------------"));
      
  }
      
} // END OF LOOP


String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(", ") +\
  String(ipAddress[1]) + String(", ") +\
  String(ipAddress[2]) + String(", ") +\
  String(ipAddress[3])  ;
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


//creates int out of a string for your IP address
int ip_convert(String convert){
  int number;
  if (convert.toInt()){
      number=convert.toInt();
      string_IP="";
      return number;
     }else{
//      Serial.println("Error! " + convert + " Not an IP address");
      //debugI("Error! " + convert + " Not an IP address");
      uint32_t convert8 = convert.toInt();
  //    debugI("Error! %u Not an IP address", convert8);
      return 400;
     }
 }

void CheckWiFiStatus(){
   if ( boolSoftAPEnabled == true || WiFi.status() == WL_DISCONNECTED ) {
            APModeLoopCnt++;
        }
        if (WiFi.status() == WL_CONNECTED){
        }
        if (WiFi.status() == WL_CONNECTION_LOST){
            APModeLoopCnt++;
         }
        if (APModeLoopCnt >= 30){
           ESP.restart();
        }
}

void makeHTTPRequest() {
digitalWrite(LED_BUILTIN, LOW); // LED ON
  // Opening connection to server (Use 80 as port if HTTP)
  if (!client.connect(WU_HOST, 443))
  {
    Serial.println(F("Connection failed"));
    return;
  }

  // give the esp a breather
  yield();

  // Send HTTP request
  client.print(F("GET "));
  // This is the second half of a request (everything that comes after the base URL)
  String wugetURL = "/v2/pws/observations/current?stationId=";
  wugetURL += WU_STATIONID;
  wugetURL += "&format=json&units=e&numericPrecision=decimal&apiKey=";
  wugetURL += WU_APIKEY;
  client.print(wugetURL);
  client.println(F(" HTTP/1.1"));

  //Headers
  client.print(F("Host: "));
  client.println(WU_HOST);

  client.println(F("Cache-Control: no-cache"));

  if (client.println() == 0)
  {
    Serial.println(F("Failed to send request"));
    return;
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0)
  {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders))
  {
    Serial.println(F("Invalid response"));
    return;
  }

  // This is probably not needed for most, but I had issues
  // with the Tindie api where sometimes there were random
  // characters coming back before the body of the response.
  // This will cause no harm to leave it in
  // peek() will look at the character, but not take it off the queue
  while (client.available() && client.peek() != '{')
  {
    char c = 0;
    client.readBytes(&c, 1);
    Serial.print(c);
    Serial.println("BAD");
  }

  // While the client is still availble read each
  // byte and print to the serial monitor
  WUJSON = "";
  while (client.available()) {
    char c = 0;
    client.readBytes(&c, 1);
    if (c == 44){
      Serial.println();
    }
    WUJSON += c; 
    Serial.print(c);
  }
  Serial.println();
  Serial.println("-----");
  Serial.println(WUJSON);
    Serial.println("-----");

      Serial.println("----====-----");
      for (int wx=0; wx<26; wx++) {
        String result = "";
        result = getValue(WUJSON, ',', wx);

  Serial.print(wx);
  Serial.print(">");
  Serial.println(result);
      }
      digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off
}


void SendDataToPWSWeather (){
  digitalWrite(LED_BUILTIN, LOW); // LED ON
  buildURL = "";
  PWSWResult = "";
    // Opening connection to server (Use 80 as port if HTTP)
  if (!client.connect(PWSUPDATE_HOST, 443))
  {
    Serial.println(F("Connection failed"));
    return;
  }

  // give the esp a breather
  yield();
  Serial.println("WUJSON: ");
  Serial.println(WUJSON);

  DynamicJsonDocument doc(960);
  DeserializationError error = deserializeJson(doc, WUJSON);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
String valUTC = doc["observations"][0]["obsTimeUtc"];
      Serial.print("rawWUJSON[\"obsTimeUtc\"] = ");
      valUTC.replace("T", "+");
      valUTC.replace("Z", "");
    Serial.println(valUTC);

    buildURL = "/api/v1/submitwx?ID=";
    buildURL += PWSW_STATIONID;
    buildURL += "&PASSWORD=";
    buildURL += PWSW_APIKEY;
    buildURL += "&dateutc=";
    buildURL += valUTC;
 int valWINDIR = doc["observations"][0]["winddir"];
    Serial.print("rawWUJSON[\"winddir\"] = ");
    Serial.println(valWINDIR); 
    buildURL += "&winddir=";
    buildURL += valWINDIR;
 int valWINDSPD = doc["observations"][0]["imperial"]["windSpeed"];
    Serial.print("rawWUJSON[\"windSpeed\"] = ");
    Serial.println(valWINDSPD); 
    buildURL += "&windspeedmph=";
    buildURL += valWINDSPD;
 int valWINDGST = doc["observations"][0]["imperial"]["windGust"];
    Serial.print("rawWUJSON[\"windGust\"] = ");
    Serial.println(valWINDGST); 
    buildURL += "&windgustmph=";
    buildURL += valWINDGST;
 double valTEMP = doc["observations"][0]["imperial"]["temp"];
    Serial.print("rawWUJSON[\"temp\"] = ");
    Serial.println(valTEMP);    
    buildURL += "&tempf=";
    buildURL += valTEMP;
 double valRAININ = doc["observations"][0]["imperial"]["precipRate"];
    Serial.print("rawWUJSON[\"valRAININ\"] = ");
    Serial.println(valRAININ);  
    buildURL += "&rainin=";
    buildURL += valRAININ;
 double val24hRAININ = doc["observations"][0]["imperial"]["precipTotal"];
    Serial.print("rawWUJSON[\"val24hRAININ\"] = ");
    Serial.println(val24hRAININ); 
    buildURL += "&dailyrainin=";
    buildURL += val24hRAININ;
 double valBAR = doc["observations"][0]["imperial"]["pressure"];
    Serial.print("rawWUJSON[\"pressure\"] = ");
    Serial.println(valBAR); 
    buildURL += "&baromin=";
    buildURL += valBAR;
 double valDEW = doc["observations"][0]["imperial"]["dewpt"];
    Serial.print("rawWUJSON[\"dewpt\"] = ");
    Serial.println(valDEW); 
    buildURL += "&dewptf=";
    buildURL += valDEW;
 int valHUMDTY = doc["observations"][0]["humidity"];
    Serial.print("rawWUJSON[\"humidity\"] = ");  
    Serial.println(valHUMDTY);
    buildURL += "&humidity=";
    buildURL += valHUMDTY;
 int valUV = doc["observations"][0]["uv"];
    Serial.print("rawWUJSON[\"uv\"] = ");
    Serial.println(valUV);
    buildURL += "&UV=";
    buildURL += valUV;

buildURL += "&softwaretype=WU2PWSWBridge2.5&action=updateraw";
Serial.print("**URL** ");
Serial.println(buildURL);

  // Send HTTP request
  client.print(F("GET "));
  // This is the second half of a request (everything that comes after the base URL)
  client.print(buildURL); 
  client.println(F(" HTTP/1.1"));

  //Headers
  client.print(F("Host: "));
  client.println(PWSUPDATE_HOST);

  client.println(F("Cache-Control: no-cache"));

  if (client.println() == 0)
  {
    Serial.println(F("Failed to send request"));
    return;
  }
  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0)
  {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders))
  {
    Serial.println(F("Invalid response"));
    return;
  }

  // This is probably not needed for most, but I had issues
  // with the Tindie api where sometimes there were random
  // characters coming back before the body of the response.
  // This will cause no harm to leave it in
  // peek() will look at the character, but not take it off the queue
  while (client.available() && client.peek() != '{')
  {
    char c = 0;
    client.readBytes(&c, 1);
    Serial.print(c);
    Serial.println("BAD");
  }

  // While the client is still availble read each
  // byte and print to the serial monitor
  while (client.available()) {
    char c = 0;
    client.readBytes(&c, 1);
    if (c == 44){
      Serial.println();
    }
    PWSWResult += c; 
    Serial.print(c);
  }
  Serial.println();
  Serial.println("-----");
  Serial.println(PWSWResult);
    Serial.println("-----");
    
      Serial.println("-----");
digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off
      }


// Converts compass direction to heading
String getHeading(int direction) {
  String strCompassDir;
if(direction < 22.5){
strCompassDir = "N";
}else if (direction < 67.5) {
strCompassDir = "NE";
}else if (direction < 112.5){
strCompassDir = "E";
}else if (direction < 157.5) {
strCompassDir = "SE";
}else if (direction < 202.5) {
strCompassDir = "S";
}else if (direction < 247.5) {
strCompassDir = "SW";
}else if (direction < 292.5) {
strCompassDir = "W";
}else if (direction < 337.5) {
strCompassDir = "NW";
}else{
strCompassDir = "N";
return strCompassDir;
}

}

