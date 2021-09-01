#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <SmartMatrix_GFX.h>
#include <SmartMatrix3.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Update.h>
#include <esp_wifi.h>

#define BUTTON 33
#define PRESSED LOW
#define NOT_PRESSED HIGH

// ========================== CONFIG START ===================================================

/// SmartMatrix Defines
#define COLOR_DEPTH 24                  // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24
#define kMatrixWidth  64
#define kMatrixHeight 32
const uint8_t kRefreshDepth = 24;       // known working: 24, 36, 48
const uint8_t kDmaBufferRows = 2;       // known working: 2-4, use 2 to save memory, more to keep from dropping frames and automatically lowering refresh rate
const uint8_t kPanelType = SMARTMATRIX_HUB75_32ROW_64COL_MOD8SCAN_P5_2727;
const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_NONE);      // see http://docs.pixelmatix.com/SmartMatrix for options
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);

SMARTMATRIX_ALLOCATE_BUFFERS(matrixLayer, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);

const int defaultBrightness = (10*255)/100;       // dim: 10% brightness

#define mw kMatrixWidth
#define mh kMatrixHeight
#define NUMMATRIX (kMatrixWidth*kMatrixHeight)

CRGB leds[kMatrixWidth*kMatrixHeight];

// Sadly this callback function must be copied around with this init code
void show_callback() {
    for (uint16_t y=0; y<kMatrixHeight; y++) {
	for (uint16_t x=0; x<kMatrixWidth; x++) {
	    CRGB led = leds[x + kMatrixWidth*y];
	    // rgb24 defined in MatrixComnon.h
	    backgroundLayer.drawPixel(x, y, { led.r, led.g, led.b } );
	}
    }
    // This should be zero copy
    // that said, copy or no copy is about the same speed in the end.
    backgroundLayer.swapBuffers(false);
}

SmartMatrix_GFX *matrix = new SmartMatrix_GFX(leds, mw, mh, show_callback);

// ========================== CONFIG END ======================================================

#define LED_BLACK		0

#define LED_RED_VERYLOW 	(3 <<  11)
#define LED_RED_LOW 		  (7 <<  11)
#define LED_RED_MEDIUM 		(15 << 11)
#define LED_RED_HIGH 		  (31 << 11)

#define LED_GREEN_VERYLOW	(1 <<  5)   
#define LED_GREEN_LOW 		(15 << 5)  
#define LED_GREEN_MEDIUM 	(31 << 5)  
#define LED_GREEN_HIGH 		(63 << 5)  

#define LED_BLUE_VERYLOW	3
#define LED_BLUE_LOW 		  7
#define LED_BLUE_MEDIUM 	15
#define LED_BLUE_HIGH 		31

#define LED_ORANGE_VERYLOW	(LED_RED_VERYLOW + LED_GREEN_VERYLOW)
#define LED_ORANGE_LOW		  (LED_RED_LOW     + LED_GREEN_LOW)
#define LED_ORANGE_MEDIUM	  (LED_RED_MEDIUM  + LED_GREEN_MEDIUM)
#define LED_ORANGE_HIGH		  (LED_RED_HIGH    + LED_GREEN_HIGH)

#define LED_PURPLE_VERYLOW	(LED_RED_VERYLOW + LED_BLUE_VERYLOW)
#define LED_PURPLE_LOW		  (LED_RED_LOW     + LED_BLUE_LOW)
#define LED_PURPLE_MEDIUM	  (LED_RED_MEDIUM  + LED_BLUE_MEDIUM)
#define LED_PURPLE_HIGH		  (LED_RED_HIGH    + LED_BLUE_HIGH)

#define LED_CYAN_VERYLOW	  (LED_GREEN_VERYLOW + LED_BLUE_VERYLOW)
#define LED_CYAN_LOW		    (LED_GREEN_LOW     + LED_BLUE_LOW)
#define LED_CYAN_MEDIUM		  (LED_GREEN_MEDIUM  + LED_BLUE_MEDIUM)
#define LED_CYAN_HIGH		    (LED_GREEN_HIGH    + LED_BLUE_HIGH)

#define LED_WHITE_VERYLOW	  (LED_RED_VERYLOW + LED_GREEN_VERYLOW + LED_BLUE_VERYLOW)
#define LED_WHITE_LOW		    (LED_RED_LOW     + LED_GREEN_LOW     + LED_BLUE_LOW)
#define LED_WHITE_MEDIUM	  (LED_RED_MEDIUM  + LED_GREEN_MEDIUM  + LED_BLUE_MEDIUM)
#define LED_WHITE_HIGH		  (LED_RED_HIGH    + LED_GREEN_HIGH    + LED_BLUE_HIGH)

unsigned int free_spaces = 0;
String arrow = "";
String URL = "URL Not Set";
String Brightness = "10";
int oldBright = 0;
String urlRefreshRate = "3";
bool isAPmodeOn = false;
bool shouldReboot = false;
String value_login[2];
bool userFlag = false;

AsyncWebServer server(80);

const char* passwordAP = "metrici@admin";
IPAddress local_IP_AP(109,108,112,114); //decimal for "mlpr"(metrici license plate recognition) in ASCII table
IPAddress gatewayAP(0,0,0,0);
IPAddress subnetAP(255,255,255,0);

IPAddress local_IP_STA, gateway_STA, subnet_STA, primaryDNS;
String v[6];
bool didIsetUpTheDisplay = true;

String strlog = "";

//------------------------- struct circular_buffer
struct ring_buffer
{
    ring_buffer(size_t cap) : buffer(cap) {}

    bool empty() const { return sz == 0 ; }
    bool full() const { return sz == buffer.size() ; }

    void push( String str )
    {
        if(last >= buffer.size()) last = 0 ;
        buffer[last] = str ;
        ++last ;
        if(full()) 
			first = (first+1) %  buffer.size() ;
        else ++sz ;
    }
    void print() const {
		strlog= "";
		if( first < last )
			for( size_t i = first ; i < last ; ++i ) {
				strlog += (buffer[i] + "<br>");
			}	
		else {
			for( size_t i = first ; i < buffer.size() ; ++i ) {
				strlog += (buffer[i] + "<br>");
			}
			for( size_t i = 0 ; i < last ; ++i ) {
				strlog += (buffer[i] + "<br>");
			}
		}
	}

    private:
        std::vector<String> buffer ;
        size_t first = 0 ;
        size_t last = 0 ;
        size_t sz = 0 ;
};
//------------------------- struct circular_buffer
ring_buffer circle(10);

//------------------------- networkScan(String)
void networkScan(String &network) {
  network = "";
  int n = WiFi.scanComplete();
  if(n == -2) {
    WiFi.scanNetworks(true);
    network = "Scanning for available networks...";
  } else if(n) {
    for (int i = 0; i < n; ++i){
      if(i) network += "<br>";
      network += WiFi.SSID(i);
      network += " - ";
      int quality = 2* (WiFi.RSSI(i) + 100);
      if (quality > 100) quality = 100;
      network += (String)quality + "&#37;";
    }
  } // else if(n)
  WiFi.scanDelete();
  if(WiFi.scanComplete() == -2) {
    WiFi.scanNetworks(true);
  }
}

//------------------------- logOutput(String)
void logOutput(String string1) {
	circle.push(string1);	
	Serial.println(string1);	
}
//------------------------- processor()
String processor(const String& var) {
	circle.print();
       if (var == "PH_Controller")
    return String("Display Controller");
  else if (var == "PH_Version")
    return String("v1.2");
  else if (var == "PH_IP_Addr")
    return local_IP_STA.toString();
  else if (var == "PH_SSID")
    return WiFi.SSID();
  else if (var == "PH_Gateway")
    return gateway_STA.toString();
  else if (var == "PH_Subnet")
    return subnet_STA.toString();
  else if (var == "PH_DNS")
    return primaryDNS.toString();
  else if (var == "PLACEHOLDER_URL")
		return String(URL);
  else if (var == "PH_URR")
		return String(urlRefreshRate);
  else if (var == "PH_Bright")
		return String(Brightness);
  else if (var == "PLACEHOLDER_LOGS")
		return String(strlog);
	return String();
}

//------------------------- listAllFiles()
void listAllFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file){
    Serial.print("FILE: ");
    String fileName = file.name();
    Serial.println(fileName);
    file = root.openNextFile();
  }
  file.close();
  root.close();
}

//------------------------- fileReadLines()
void fileReadLines(File file, String x[]) {
  int i = 0;
    while(file.available()){
      String line= file.readStringUntil('\n');
      line.trim();
      x[i] = line;
      i++;
    }    
}

String readString(File s) {
  // Read from file witout yield 
  String ret;
  int c = s.read();
  while (c >= 0) {
    ret += (char)c;
    c = s.read();
  }
  return ret;
}

// Add files to the /files table
String& addDirList(String& HTML) {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  int count = 0;
  while (file) {
    String fileName = file.name();
    File dirFile = SPIFFS.open(fileName, "r");
    size_t filelen = dirFile.size();
    dirFile.close();
    String filename_temp = fileName;
    filename_temp.replace("/", "");
    String directories = "<form action=\"/files\" method=\"POST\">\r\n<tr><td align=\"center\">";
    if(fileName.indexOf("Display") > 0) {
      directories += "<input type=\"hidden\" name = \"filename\" value=\"" + fileName + "\">" + filename_temp;
      directories += "</td>\r\n<td align=\"center\">" + String(filelen, DEC);
      directories += "</td><td align=\"center\"><button style=\"margin-right:20px;\" type=\"submit\" name= \"download\">Download</button><button type=\"submit\" name= \"delete\">Delete</button></td>\r\n";
      directories += "</tr></form>\r\n~directories~";
      HTML.replace("~directories~", directories);
      count++;
    }
    file = root.openNextFile();
  }
  HTML.replace("~directories~", "");
  
  HTML.replace("~count~", String(count, DEC));
  HTML.replace("~total~", String(SPIFFS.totalBytes() / 1024, DEC));
  HTML.replace("~used~", String(SPIFFS.usedBytes() / 1024, DEC));
  HTML.replace("~free~", String((SPIFFS.totalBytes() - SPIFFS.usedBytes()) / 1024, DEC));

  return HTML;
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  if(filename.indexOf(".bin") > 0) {
    if (!index){
      logOutput("The update process has started...");
      // if filename includes spiffs, update the spiffs partition
      int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
        Update.printError(Serial);
      }
    }

    if (Update.write(data, len) != len) {
      Update.printError(Serial);
    }

    if (final) {
      if (filename.indexOf("spiffs") > -1) {
        request->send(200, "text/html", "<div style=\"margin:0 auto; text-align:center; font-family:arial;\">The device entered AP Mode ! Please connect to it.</div>");  
      } else {
        request->send(200, "text/html", "<div style=\"margin:0 auto; text-align:center; font-family:arial;\">Congratulation ! </br> You have successfully updated the device to the latest version. </br>Please wait 10 seconds until the device reboots, then press on the \"Go Home\" button to go back to the main page.</br></br> <form method=\"post\" action=\"http://" + local_IP_STA.toString() + "\"><input type=\"submit\" name=\"goHome\" value=\"Go Home\"/></form></div>");
      }

      if (!Update.end(true)){
        Update.printError(Serial);
      } else {
        logOutput("Update complete");
        Serial.flush();
        ESP.restart();
      }
    }
  } else {
      if(!index){
        logOutput((String)"Started uploading: " + filename);
        // open the file on first call and store the file handle in the request object
        request->_tempFile = SPIFFS.open("/"+filename, "w");
      }
      if(len) {
        // stream the incoming chunk to the opened file
        request->_tempFile.write(data,len);
      }
      if(final){
        logOutput((String)filename + " was successfully uploaded! File size: " + index+len);
        // close the file handle as the upload is now done
        request->_tempFile.close();
        request->redirect("/files");
      }
    }
}

//------------------------- go_right()
void go_right(unsigned int space, String stringSign) {
  // Serial.println("I am positioned on the right side of the Display.");
  matrix->clear();
  matrix->setRotation(0);
  matrix->setTextSize(1);
  matrix->setFont(&FreeSansBold18pt7b);
  matrix->setTextWrap(false);
  matrix->setCursor(12,27);             // cursor for 1 digit

  matrix->setTextColor(LED_GREEN_HIGH);
  if(space == 0) {
    matrix->setTextColor(LED_RED_HIGH);
  } else if (space > 9) {
    matrix->setCursor(2,27);        // cursor for 2 digits
  } else if (space > 99) {
    space = 99;
    matrix->setCursor(2,27);
  }
  matrix->print(space);

  if        (stringSign == ">") {
      matrix->drawTriangle(42,3,42,27,60,15,LED_WHITE_HIGH);
      matrix->fillTriangle(42,3,42,27,60,15,LED_WHITE_HIGH);
  } else if (stringSign == "^") {
      matrix->drawTriangle(41,27,51,3,61,27,LED_WHITE_HIGH);
      matrix->fillTriangle(41,27,51,3,61,27,LED_WHITE_HIGH);
  } else if (stringSign == "V") {
      matrix->drawTriangle(41,3,51,27,61,3,LED_WHITE_HIGH);
      matrix->fillTriangle(41,3,51,27,61,3,LED_WHITE_HIGH);
  } else if(stringSign == "<") {
      matrix->drawTriangle(60,3,60,27,42,15,LED_WHITE_HIGH);
      matrix->fillTriangle(60,3,60,27,42,15,LED_WHITE_HIGH);
  }
  matrix->show();
}

//------------------------- go_left()
void go_left(unsigned int space, String stringSign) {
  // Serial.println("I am positioned on the left side of the Display.");
  matrix->clear();
  matrix->setRotation(0);
  matrix->setTextSize(1);
  matrix->setFont(&FreeSansBold18pt7b);
  matrix->setTextWrap(false);
  matrix->setCursor(33,27);             // cursor for 1 digit
  matrix->setTextColor(LED_GREEN_HIGH);
  if(space == 0) {
    matrix->setTextColor(LED_RED_HIGH);
  } else if (space > 9) {
    matrix->setCursor(24,27);        // cursor for 2 digits
  } else if (space > 99) {
    space = 99;
    matrix->setCursor(24,27);
  }
  matrix->print(space);
  if        (stringSign == "<") {
      matrix->drawTriangle(21,3,21,27,3,15,LED_WHITE_HIGH);
      matrix->fillTriangle(21,3,21,27,3,15,LED_WHITE_HIGH);
  } else if (stringSign == "^") {
      matrix->drawTriangle(2,27,12,3,22,27,LED_WHITE_HIGH);
      matrix->fillTriangle(2,27,12,3,22,27,LED_WHITE_HIGH);
  } else if (stringSign == "V") {
      matrix->drawTriangle(2,3,12,27,22,3,LED_WHITE_HIGH);
      matrix->fillTriangle(2,3,12,27,22,3,LED_WHITE_HIGH);
  } else if(stringSign == ">") {
      matrix->drawTriangle(3,3,3,27,21,15,LED_WHITE_HIGH);
      matrix->fillTriangle(3,3,3,27,21,15,LED_WHITE_HIGH);
  }

  matrix->show();
}

//------------------------- scroll_text()
void scroll_text(String text, uint16_t color, uint8_t ypos) {
  uint16_t text_length = text.length();
  matrix->clear();
  matrix->setTextWrap(false);
  matrix->setRotation(0);
  matrix->setFont(&FreeSans9pt7b);
  matrix->setTextColor(color);
  for(int xpos = mw; xpos > - (mw + text_length*9); xpos--) {
    matrix->clear();
    matrix->setCursor(xpos,ypos);
    matrix->print(text);
    yield();
    matrix->show();
    delay(20);
  }
}

//------------------------- restart_sequence()
void restart_sequence(unsigned int countdown) {
  for(int i = countdown; i>=0; i--) {
    matrix->clear();
    matrix->setFont(NULL);
    matrix->setTextSize(1);
    matrix->setTextColor(LED_RED_HIGH);
    matrix->setCursor(2,1);
    matrix->print("Restarting");
    matrix->setCursor(20,15);
    matrix->print((String)"in " + i);
    matrix->show();
    delay(1000);
  }
  server.reset();
  delay(100);
  matrix->clear();
  delay(100);
  ESP.restart();
}

//------------------------- startSTA()
void startSTA(String x[]){
  if(x[0] != NULL &&      // SSID
     x[0].length() != 0 &&
     x[1] != NULL &&      // Password
     x[1].length() != 0 &&
     x[2] != NULL &&      //Local IP
     x[2].length() != 0 &&
     x[3] != NULL &&      // Gateway
     x[3].length() != 0 &&
     x[4] != NULL &&      // Subnet
     x[4].length() != 0 &&
     x[5] != NULL &&      // DNS
     x[5].length() != 0) {
       local_IP_STA.fromString(x[2]);
       gateway_STA.fromString(x[3]);
       subnet_STA.fromString(x[4]);
       primaryDNS.fromString(x[5]);
       if(!WiFi.config(local_IP_STA, gateway_STA, subnet_STA, primaryDNS))
          logOutput((String)"WARNING: Couldn't configure STATIC IP ! Starting DHCP IP !");
       delay(50);
       WiFi.begin(x[0].c_str(),x[1].c_str());
       WiFi.setSleep(false);
       delay(50);   
       } else if (x[0] != NULL &&      // SSID
             x[0].length() != 0 &&
             x[1] != NULL &&           // Password
             x[1].length() != 0 &&
             x[2] == NULL &&           //Local IP
             x[2].length() == 0 &&
             x[3] == NULL &&           // Gateway
             x[3].length() == 0 &&
             x[4] == NULL &&           // Subnet
             x[4].length() == 0 &&
             x[5] == NULL &&           // DNS
             x[5].length() == 0) {
               WiFi.begin(x[0].c_str(),x[1].c_str());
               WiFi.setSleep(false);
               delay(50);
               }

int k = 0;
while (WiFi.status() != WL_CONNECTED && k<20) {
  k++;
  delay(1000);
  logOutput((String)"Attempt " + k + " - Connecting to WiFi..");
}

if(WiFi.status() != WL_CONNECTED) {
    if(didIsetUpTheDisplay) {
      logOutput((String)"ERROR: (1) Could not acces Wireless Netowrk ! WiFi Router/AP is down, trying again...");
      logOutput((String)"WARNING: Controller will restart in 5 seconds !");
      delay(5000);
      ESP.restart();
    } else {
      scroll_text("(1) Could not access Wireless Network ! WiFi Router/AP is down !",LED_RED_HIGH,20);
      scroll_text("(2) SSID or Password Incorrect ! Press RESET button and enter AP Mode to reconfigure !",LED_RED_HIGH,20);
      restart_sequence(5);
    }
  }
  local_IP_STA = WiFi.localIP();
  gateway_STA = WiFi.gatewayIP();
  subnet_STA = WiFi.subnetMask();
  primaryDNS = WiFi.dnsIP();
  logOutput((String)"Connected to " + x[0] + " with IP addres: " + local_IP_STA.toString());
  logOutput((String)"Gateway: " + gateway_STA.toString());
  logOutput((String)"Subnet: " + subnet_STA.toString());
  logOutput((String)"DNS: " + primaryDNS.toString());
}

//------------------------- startAP()
void startAP() {
  if(SPIFFS.exists("/networkDisplay.txt")) SPIFFS.remove("/networkDisplay.txt");

  String x = "Metrici";
  x.concat(random(100,999));
  char ssidAP[x.length()+1];
  x.toCharArray(ssidAP,sizeof(ssidAP));
  Serial.println((String)"Connect to " + ssidAP);
  logOutput((String)"Starting AP ... ");
  logOutput(WiFi.softAP(ssidAP, passwordAP) ? "Ready" : "ERROR: WiFi.softAP failed ! (Password must be at least 8 characters long )");
  delay(100);
  logOutput((String)"Setting AP configuration ... ");
  logOutput(WiFi.softAPConfig(local_IP_AP, gatewayAP, subnetAP) ? "Ready" : "Failed!");
  delay(100);
  logOutput((String)"Soft-AP IP address: ");
  logOutput(WiFi.softAPIP().toString());
  
  while (WiFi.softAPgetStationNum() == 0) {
    matrix->clear();
    matrix->setFont(NULL);
    matrix->setCursor(1,8);
    matrix->setTextColor(LED_PURPLE_HIGH);
    matrix->print((String)ssidAP);   
    matrix->setTextColor(LED_CYAN_HIGH);
    matrix->setCursor(1,0);
    matrix->print("Connect to");
    matrix->setCursor(1,16);
    matrix->print("Wireless");
    matrix->setCursor(1,24);
    matrix->print("Network"); 
    matrix->show();
    delay(2);
  }

  delay(100);
  matrix->clear();  
  matrix->setCursor(0,0);
  matrix->print("Config page");
  matrix->setCursor(0,8);
  matrix->print("found at:");
  matrix->setCursor(0,16);
  matrix->setTextWrap(true);
  matrix->setTextColor(LED_PURPLE_HIGH);
  matrix->print((String)WiFi.softAPIP().toString());
  matrix->show();

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/favicon.ico", "image/ico");
  });
  server.on("/newMaster.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/newMaster.css", "text/css");
  });
  server.on("/jsMaster.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/jsMaster.js", "text/javascript");
  });
  server.on("/logo.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/logo.png", "image/png");
  });
  server.on("/events_placeholder.html", HTTP_GET, [](AsyncWebServerRequest* request){
    request->send(SPIFFS, "/events_placeholder.html", "text/html", false, processor);
  });

  server.on("/register", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/createUser.html", "text/html", false, processor);
  });
  server.on("/chooseIP", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/chooseIP.html", "text/html", false, processor);
  });
  server.on("/dhcpIP", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/dhcpIP.html", "text/html", false, processor);
  });
  server.on("/staticIP", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/staticIP.html", "text/html", false, processor);
  });
  server.on("/logs", HTTP_GET, [](AsyncWebServerRequest* request){
    request->send(SPIFFS, "/events.html", "text/html", false, processor);
  });

  server.on("/register", HTTP_POST, [](AsyncWebServerRequest * request){
    if(request->hasArg("register")){
      int params = request->params();
      String values_user[3];
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
            logOutput((String)"POST[" + p->name().c_str() + "]: " + p->value().c_str() + "\n");            
            values_user[i] = p->value();
          } else {
              logOutput((String)"GET[" + p->name().c_str() + "]: " + p->value().c_str() + "\n");
            }
      } // for(int i=0;i<params;i++)
      
      if(values_user[0] != NULL && values_user[0].length() > 4 &&
        values_user[1] != NULL && values_user[1].length() > 7) {
            File userWrite = SPIFFS.open("/userDisplay.txt", "w");
            if(!userWrite) logOutput((String)"ERROR: Couldn't open file to write USER credentials !");
            userWrite.println(values_user[0]);  // Username
            userWrite.println(values_user[1]);  // Password
            userWrite.close();
            logOutput("Username and password saved !");          
            request->redirect("/chooseIP");
            // digitalWrite(LED, LOW);
      } else request->redirect("/register");  
    } else if (request->hasArg("skip")) {
      request->redirect("/chooseIP");
    } else if(request->hasArg("import")) {
      request->redirect("/files");
    } else {
      request->redirect("/register");
    }
  }); // server.on("/register", HTTP_POST, [](AsyncWebServerRequest * request)

  server.on("/files", HTTP_ANY, [](AsyncWebServerRequest *request){
    if (request->hasParam("filename", true)) { // Check for files
      if (request->hasArg("download")) { // Download file
        Serial.println("Download Filename: " + request->arg("filename"));
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, request->arg("filename"), String(), true);
        response->addHeader("Server", "ESP Async Web Server");
        request->send(response);
        return;
      } else if(request->hasArg("delete")) { // Delete file
        if (SPIFFS.remove(request->getParam("filename", true)->value())) {
          logOutput((String)request->getParam("filename", true)->value().c_str() + " was deleted !");
        } else {
          logOutput("Could not delete file. Try again !");
        }
        request->redirect("/files");
      }
    } else if(request->hasArg("restart_device")) {
        request->send(200,"text/plain", "The device will reboot shortly !");
        shouldReboot = true;
    }

    String HTML PROGMEM; // HTML code 
    File pageFile = SPIFFS.open("/files.html", "r");
    if (pageFile) {
      HTML = readString(pageFile);
      pageFile.close();
      HTML = addDirList(HTML);
      AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML.c_str(), processor);
      response->addHeader("Server","ESP Async Web Server");
      request->send(response);
    }
  });

  server.on("/staticIP", HTTP_POST, [](AsyncWebServerRequest * request){
    if(request->hasArg("saveStatic")){
      int params = request->params();      
      String values_static[params];

      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
            logOutput((String)"POST[" + p->name().c_str() + "]: " + p->value().c_str() + "\n");
            values_static[i] = p->value();            
          } else {
              logOutput((String)"GET[" + p->name().c_str() + "]: " + p->value().c_str() + "\n");
            }
      } // for(int i=0;i<params;i++)

      if(values_static[0] != NULL &&
        values_static[0].length() != 0 &&
        values_static[1] != NULL &&
        values_static[1].length() != 0 &&
        values_static[2] != NULL &&
        values_static[2].length() != 0 &&
        values_static[3] != NULL &&
        values_static[3].length() != 0 &&
        values_static[4] != NULL &&
        values_static[4].length() != 0 &&
        values_static[5] != NULL &&
        values_static[5].length() != 0) {
            File inputsWrite = SPIFFS.open("/networkDisplay.txt", "w");
            if(!inputsWrite) logOutput((String)"ERROR: Couldn't open file to write Static IP credentials !"); 
            inputsWrite.println(values_static[0]);   // SSID
            inputsWrite.println(values_static[1]);   // Password
            inputsWrite.println(values_static[2]);   // Local IP
            inputsWrite.println(values_static[3]);   // Gateway
            inputsWrite.println(values_static[4]);   // Subnet
            inputsWrite.println(values_static[5]);   // DNS
            inputsWrite.close();
            logOutput("Configuration saved !");
            request->redirect("/logs");
            shouldReboot = true;
      } else {
        request->redirect("/staticIP");
      }
    } else {
      request->redirect("/staticIP");
    }
  }); // server.on("/staticLogin", HTTP_POST, [](AsyncWebServerRequest * request)

  server.on("/dhcpIP", HTTP_POST, [](AsyncWebServerRequest * request){
    if(request->hasArg("saveDHCP")){    
      int params = request->params();
      String values_dhcp[params];

      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
            logOutput((String)"POST[" + p->name().c_str() + "]: " + p->value().c_str() + "\n");            
            values_dhcp[i] = p->value();            
          } else {
              logOutput((String)"GET[" + p->name().c_str() + "]: " + p->value().c_str() + "\n");
            }
      }

      if(values_dhcp[0] != NULL && values_dhcp[0].length() != 0 &&
        values_dhcp[1] != NULL && values_dhcp[1].length() != 0) {
        File networkDisplayWrite = SPIFFS.open("/networkDisplay.txt", "w");
        if(!networkDisplayWrite) logOutput((String)"ERROR: Couldn't open file to write DHCP IP credentials !");
        networkDisplayWrite.println(values_dhcp[0]);  // SSID
        networkDisplayWrite.println(values_dhcp[1]);  // Password
        networkDisplayWrite.close();
        logOutput("Configuration saved !");
        logOutput("Restarting in 5 seconds...");
        request->redirect("/logs");
        shouldReboot = true;
      } else {
        request->redirect("/dhcpIP");
      }
    } else {
      request->redirect("/dhcpIP");
    }  
  }); // server.on("/dhcpLogin", HTTP_POST, [](AsyncWebServerRequest * request)
  
  server.onFileUpload(handleUpload);
  server.onNotFound([](AsyncWebServerRequest *request){ request->redirect("/register"); });
  server.begin(); //-------------------------------------------------------------- server.begin()

} // void startAP()

void setup() {
  enableCore1WDT();
  esp_wifi_set_ps (WIFI_PS_NONE);
  delay(100);
  Serial.begin(115200);
  delay(100);

  matrixLayer.addLayer(&backgroundLayer); 
  matrixLayer.begin();
  matrixLayer.setBrightness(defaultBrightness);
  matrixLayer.setRefreshRate(240);
  backgroundLayer.enableColorCorrection(true);
  backgroundLayer.fillScreen( {0x00, 0x00, 0x00} );
  backgroundLayer.swapBuffers();
  matrix->begin();
  delay(100);

  matrix->setTextWrap(false);
  matrix->setRotation(0);
  matrix->setTextSize(1);
  // pinMode(LED, OUTPUT);
  pinMode(BUTTON,INPUT_PULLUP);
  delay(50);
  // digitalWrite(LED,LOW);
  digitalWrite(BUTTON, NOT_PRESSED);
  delay(50);

  int q = 0;
  if (digitalRead(BUTTON) == PRESSED) {
    q++;
    delay(2000);      
    if (digitalRead(BUTTON) == PRESSED) {
      q++;
    }
  }
  Serial.println((String)"Q == " + q);
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS ! Formatting in progress");
    return;
  }

  if (q == 2) {
    delay(10);
    //--- Blink 2 times
    // digitalWrite(LED, HIGH); delay(200); digitalWrite(LED, LOW); delay(200); digitalWrite(LED, HIGH); delay(200); digitalWrite(LED, LOW); delay(10);
    if(SPIFFS.exists("/networkDisplay.txt")) SPIFFS.remove("/networkDisplay.txt");
    if(SPIFFS.exists("/userDisplay.txt")) SPIFFS.remove("/userDisplay.txt");
    if(SPIFFS.exists("/configDisplay.txt")) SPIFFS.remove("/configDisplay.txt");
    logOutput((String)"WARNING: Reset button was pressed ! AP will start momentarily");
    scroll_text("Reset button was pressed ! AP will start momentarily",LED_RED_HIGH,20); 
  }

  listAllFiles();

  //--- Check if we've made any configuration
  if(!SPIFFS.exists("/configDisplay.txt")) {
    didIsetUpTheDisplay = false;
  }

  //--- Check if /networkDisplay.txt exists. If not then create one
  if(!SPIFFS.exists("/networkDisplay.txt")) {
    File networkCreate = SPIFFS.open("/networkDisplay.txt", "w");
    if(!networkCreate) logOutput((String)"Couldn't create /networkDisplay.txt");
    logOutput("Was /networkDisplay.txt created ?");
    logOutput(SPIFFS.exists("/networkDisplay.txt") ? "Yes" : "No");
    networkCreate.close();
  }

  //--- Check if /networkDisplay.txt is empty (0 bytes) or not (>8)
  //--- println adds /r and /n, which means 2 bytes
  //--- 2 lines = 2 println = 4 bytes
  File networkRead = SPIFFS.open("/networkDisplay.txt");
  if(!networkRead) logOutput((String)"ERROR: Couldn't open networkDisplay !");

  if(networkRead.size() > 8) {
    //--- Read SSID, Password, Local IP, Gateway, Subnet, DNS from file
    //--- and store them in v[]
    fileReadLines(networkRead,v);
    networkRead.close();

    //--- Start ESP in Station Mode using the above credentials
    startSTA(v);

    if(!didIsetUpTheDisplay) {
      matrix->clear();
      matrix->setTextColor(LED_CYAN_HIGH);
      matrix->setFont(NULL);
      matrix->setCursor(1,0);
      matrix->print("Home page");
      matrix->setCursor(0,8);
      matrix->print("found at:");
      matrix->setCursor(0,16);
      matrix->setTextWrap(true);
      matrix->setTextColor(LED_PURPLE_HIGH);
      matrix->print(local_IP_STA.toString());
      matrix->show();
      delay(1000);
      matrix->setTextWrap(false);  
    }
    
    if(SPIFFS.exists("/userDisplay.txt")) {
      File userDisplayRead = SPIFFS.open("/userDisplay.txt", "r");
      if(!userDisplayRead) logOutput((String)"ERROR: Couldn't open userDisplay !");
      fileReadLines(userDisplayRead, value_login);
      if(value_login[0].length() >4 && value_login[0] != NULL &&
        value_login[1].length() >7 && value_login[1] != NULL) {
          userFlag = true;
        }
    }

    if(didIsetUpTheDisplay) {
      String values_config[3];
      File configRead = SPIFFS.open("/configDisplay.txt", "r");
      if(!configRead) logOutput((String)"ERROR: Couldn't open configDisplay to read values !");
      fileReadLines(configRead, values_config);
      if (values_config[0]!= NULL || values_config[0].length() != 0) {
        URL = values_config[0];
      } else {
        values_config[0] = "URL Link: " + URL;
      }
      if (values_config[1]!= NULL || values_config[1].length() != 0) {
        urlRefreshRate = values_config[1];
      } else {
        values_config[1] = "URL Refresh Rate: " + urlRefreshRate;
      }
      if (values_config[2]!= NULL || values_config[2].length() != 0) {
        Brightness = values_config[2];
      } else {
        values_config[2] = "Brightness (%): " + Brightness;
      }

      logOutput("Present Configuration: ");
      for(int k=0; k<3; k++) {
        logOutput((String)(k+1) + ": " + values_config[k]);
      }
      configRead.close();
    } // if(SPIFFS.exists("/configDisplay.txt"))


    server.on("/newMaster.css", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/newMaster.css", "text/css");
    });
    server.on("/jsMaster.js", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/jsMaster.js", "text/javascript");
    });
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/favicon.ico", "image/ico");
    });
    server.on("/logo.png", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/logo.png", "image/png");
    });
    server.on("/events_placeholder.html", HTTP_GET, [](AsyncWebServerRequest* request){
      request->send(SPIFFS, "/events_placeholder.html", "text/html", false, processor);
    });

    server.on("/home", HTTP_GET, [](AsyncWebServerRequest* request){
      if(userFlag) {
        if(!request->authenticate(value_login[0].c_str(), value_login[1].c_str()))
          return request->requestAuthentication(NULL,false);
        request->send(SPIFFS, "/index.html", "text/html", false, processor);
      } else {          
        request->send(SPIFFS, "/index.html", "text/html", false, processor);
      }
    });

    server.on("/chooseIP", HTTP_GET, [](AsyncWebServerRequest *request){
      if(userFlag) {
        if(!request->authenticate(value_login[0].c_str(), value_login[1].c_str()))
          return request->requestAuthentication(NULL,false);            
        request->send(SPIFFS, "/chooseIP.html", "text/html", false, processor);
      } else {
        request->send(SPIFFS, "/chooseIP.html", "text/html", false, processor);
      }  
    });
    server.on("/dhcpIP", HTTP_GET, [](AsyncWebServerRequest *request){
      if(userFlag) {
        if(!request->authenticate(value_login[0].c_str(), value_login[1].c_str()))
          return request->requestAuthentication(NULL,false);            
        request->send(SPIFFS, "/dhcpIP.html", "text/html", false, processor);
      } else {
        request->send(SPIFFS, "/dhcpIP.html", "text/html", false, processor);
      }
    });
    server.on("/staticIP", HTTP_GET, [](AsyncWebServerRequest *request){
      if(userFlag) {
        if(!request->authenticate(value_login[0].c_str(), value_login[1].c_str()))
          return request->requestAuthentication(NULL,false);            
        request->send(SPIFFS, "/staticIP.html", "text/html", false, processor);
      } else {
        request->send(SPIFFS, "/staticIP.html", "text/html", false, processor);
      }
    });

    server.on("/home", HTTP_POST, [](AsyncWebServerRequest * request){
      if(request->hasArg("save_values")){
        request->redirect("/home");
        int params = request->params();
        for(int i=0;i<params;i++){
          AsyncWebParameter* p = request->getParam(i);
            if(p->isPost() && p->value() != NULL && p->name() != "save_values") {
              File configWrite = SPIFFS.open("/configDisplay.txt", "w");
              if(!configWrite) logOutput((String)"ERROR: Couldn't open configDisplay to save values from POST !");
              logOutput((String)"POST[" + p->name().c_str() + "]: " + p->value().c_str());
              if(p->name() == "getURL") {
              URL = p->value();
              configWrite.println(URL);
              configWrite.println(urlRefreshRate);
              configWrite.println(Brightness);                
            }
            if(p->name() == "getURR") {
              urlRefreshRate = p->value();
              configWrite.println(URL);
              configWrite.println(urlRefreshRate);
              configWrite.println(Brightness);
            }
            if(p->name() == "getBrightness") {
              Brightness = p->value();
              configWrite.println(URL);
              configWrite.println(urlRefreshRate);
              configWrite.println(Brightness);
            }
              configWrite.close();
            }
        } // for(int i=0;i<params;i++)
      } else if(request->hasArg("goUpdate")) {
        request->redirect("/update");
      } else if(request->hasArg("ip_settings")) {
          request->redirect("/chooseIP");
      } else if(request->hasArg("import_export")) {
          request->redirect("/files");
      } else {
        request->redirect("/home");
      }
    });

    server.on("/files", HTTP_ANY, [](AsyncWebServerRequest *request) {
      if(userFlag) {
        if(!request->authenticate(value_login[0].c_str(), value_login[1].c_str()))
          return request->requestAuthentication(NULL,false);            
        if (request->hasParam("filename", true)) { // Download file
          if (request->hasArg("download")) { // File download
            Serial.println("Download Filename: " + request->arg("filename"));
            AsyncWebServerResponse *response = request->beginResponse(SPIFFS, request->arg("filename"), String(), true);
            response->addHeader("Server", "ESP Async Web Server");
            request->send(response);
            return;
          } else if(request->hasArg("delete")) { // Delete file
            if (SPIFFS.remove(request->getParam("filename", true)->value())) {
              logOutput((String)request->getParam("filename", true)->value().c_str() + " was deleted !");
            } else {
              logOutput("Could not delete file. Try again !");
            }
            request->redirect("/files");
          }
        } else if(request->hasArg("goBack")) { // Go Back Button
          request->redirect("/home");
        } else if(request->hasArg("restart_device")) {
          request->send(200,"text/plain", "The device will reboot shortly !");
          ESP.restart();
        }
        String HTML PROGMEM; // HTML code 
        File pageFile = SPIFFS.open("/files.html", "r");
        if (pageFile) {
          HTML = readString(pageFile);
          pageFile.close();
          HTML = addDirList(HTML);
          AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML.c_str(), processor);
          response->addHeader("Server","ESP Async Web Server");
          request->send(response);         
        }
      } else {
        if (request->hasParam("filename", true)) { // Download file
          if (request->hasArg("download")) { // File download
            Serial.println("Download Filename: " + request->arg("filename"));
            AsyncWebServerResponse *response = request->beginResponse(SPIFFS, request->arg("filename"), String(), true);
            response->addHeader("Server", "ESP Async Web Server");
            request->send(response);
            return;
          } else if(request->hasArg("delete")) { // Delete file
            if (SPIFFS.remove(request->getParam("filename", true)->value())) {
              logOutput((String)request->getParam("filename", true)->value().c_str() + " was deleted !");
            } else {
              logOutput("Could not delete file. Try again !");
            }
            request->redirect("/files");
          }
        } else if(request->hasArg("goBack")) { // Go Back Button
          request->redirect("/home");
        } else if(request->hasArg("restart_device")) {
          request->send(200,"text/plain", "The device will reboot shortly !");
          ESP.restart();
        }
        String HTML PROGMEM; // HTML code 
        File pageFile = SPIFFS.open("/files.html", "r");
        if (pageFile) {
          HTML = readString(pageFile);
          pageFile.close();
          HTML = addDirList(HTML);
          AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML.c_str(), processor);
          response->addHeader("Server","ESP Async Web Server");
          request->send(response);         
        }
      }
    }); // server.on("/files", HTTP_ANY, [](AsyncWebServerRequest *request)

    server.on("/staticIP", HTTP_POST, [](AsyncWebServerRequest * request){
      if(request->hasArg("saveStatic")){
        int params = request->params();
        String values_static[8];
        for(int i=0;i<params;i++){
          AsyncWebParameter* p = request->getParam(i);
          if(p->isPost()){
              logOutput((String)"POST[" + p->name().c_str() + "]: " + p->value().c_str() + "\n");
              values_static[i] = p->value();            
            } else {
                logOutput((String)"GET[" + p->name().c_str() + "]: " + p->value().c_str() + "\n");
              }
        } // for(int i=0;i<params;i++)

        if(values_static[0] != NULL &&
          values_static[0].length() != 0 &&
          values_static[1] != NULL &&
          values_static[1].length() != 0 &&
          values_static[2] != NULL &&
          values_static[2].length() != 0 &&
          values_static[3] != NULL &&
          values_static[3].length() != 0 &&
          values_static[4] != NULL &&
          values_static[4].length() != 0 &&
          values_static[5] != NULL &&
          values_static[5].length() != 0) {
              File inputsWrite = SPIFFS.open("/networkDisplay.txt", "w");
              if(!inputsWrite) logOutput((String)"ERROR: Couldn't open networkDisplay to write Static IP credentials !"); 
              inputsWrite.println(values_static[0]);   // SSID
              inputsWrite.println(values_static[1]);   // Password
              inputsWrite.println(values_static[2]);   // Local IP
              inputsWrite.println(values_static[3]);   // Gateway
              inputsWrite.println(values_static[4]);   // Subnet
              inputsWrite.println(values_static[5]);   // DNS
              inputsWrite.close();
              logOutput("Configuration saved !");
              request->send(200, "text/html", (String)"<div style=\"text-align:center; font-family:arial;\">Congratulation !</br></br>You have successfully changed the networks settings.</br></br>The device will now restart and try to apply the new settings.</br></br>Please wait 10 seconds and then press on the \"Go Home\" button to return to the main page.</br></br>If you can't return to the main page please check the entered values.</br></br><form method=\"post\" action=\"http://" + values_static[2] + "\"><input type=\"submit\" value=\"Go Home\"/></form></div>"); 
              shouldReboot = true;
        } else {
          request->redirect("/staticIP");
        }
      } else {
        request->redirect("/staticIP");
      }          
    }); // server.on("/staticLogin", HTTP_POST, [](AsyncWebServerRequest * request)

    server.on("/dhcpIP", HTTP_POST, [](AsyncWebServerRequest * request){
      if(request->hasArg("saveDHCP")){
        int params = request->params();
        String values_dhcp[params];

        for(int i=0;i<params;i++){
          AsyncWebParameter* p = request->getParam(i);
          if(p->isPost()){
              logOutput((String)"POST[" + p->name().c_str() + "]: " + p->value().c_str() + "\n");            
              values_dhcp[i] = p->value();            
            } else {
                logOutput((String)"GET[" + p->name().c_str() + "]: " + p->value().c_str() + "\n");
              }
        }

        if(values_dhcp[0] != NULL && values_dhcp[0].length() != 0 &&
           values_dhcp[1] != NULL && values_dhcp[1].length() != 0) {
            File networkDisplayWrite = SPIFFS.open("/networkDisplay.txt", "w");
            if(!networkDisplayWrite) logOutput((String)"ERROR: Couldn't open file to write DHCP IP credentials !");
            networkDisplayWrite.println(values_dhcp[0]);  // SSID
            networkDisplayWrite.println(values_dhcp[1]);  // Password
            networkDisplayWrite.close();
            logOutput("Configuration saved !");
            request->send(200, "text/html", "<div style=\"text-align:center; font-family:arial;\">Congratulation !</br></br>You have successfully changed the networks settings.</br></br>The device will now restart and try to apply the new settings.</br></br>You can get this device's IP by looking through your AP's DHCP List."); 
            shouldReboot = true;
        } else {
          request->redirect("/dhcpIP");
        }
      } else {
        request->redirect("/dhcpIP");
      }
    }); // server.on("/dhcpLogin", HTTP_POST, [](AsyncWebServerRequest * request)

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
      if(userFlag) {
        if(!request->authenticate(value_login[0].c_str(), value_login[1].c_str()))
          return request->requestAuthentication(NULL,false);            
        request->send(SPIFFS, "/update_page.html", "text/html", false, processor);
      } else {
        request->send(SPIFFS, "/update_page.html", "text/html", false, processor);
      }
    });

    server.onFileUpload(handleUpload);
    server.onNotFound([](AsyncWebServerRequest *request){ request->redirect("/home"); });
    server.begin();
  } else {
    networkRead.close();
    isAPmodeOn = true;
    logOutput(WiFi.mode(WIFI_AP) ? "Controller went in AP Mode !" : "ERROR: Controller couldn't go in AP_MODE. AP_STATION_MODE will start.");
    startAP();
  }
} //--- void setup()

int getPlaces(String string) {
  Serial.println((String)"URL Body 1: " + string);
  string.remove(0, 6);
  string.trim();
  string.toUpperCase();
  Serial.println((String)"URL Body 2: " + string);
  int tag = string.indexOf("#");
  arrow = string.substring(tag+1,tag+2);
  logOutput((String)"Arrow: " + arrow);
  string.replace("#"+arrow, "");
  string.trim();
  // Serial.println((String)"URL Body 3: " + string);
  free_spaces = string.toInt();
  logOutput((String)"Free parking spaces: " + free_spaces);
  return tag;
}

int oldTag = -1;
int httpFailCounter = 0;
int urlFailCounter = 0;

void loop() {
  delay(2);
  if (shouldReboot) {
    restart_sequence(5);
  }

  if(!isAPmodeOn) { // Check if we are in AP_MODE
    int tag = -1;
    
    if (WiFi.status() == WL_CONNECTED && SPIFFS.exists("/configDisplay.txt")) {  //Check the current connection status
      // logOutput((String)"IP ADDRESS: " + IP_copy);
      Serial.println((String)"DEBUG: URL = " + URL);
      if(URL.length() > 15) {
        HTTPClient http;        
        logOutput(http.begin(URL) ? "DNS Resolved" : "DNS Failed");
        int httpCode = http.GET(); //Make the request          
        if (httpCode > 0) { //Check for the returning code          
          logOutput((String)"[HTTP] GET... code: " + httpCode);
          if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                logOutput(payload);
                tag = getPlaces(payload);
                oldTag = tag;
                httpFailCounter = 0;
                if(arrow == "") {
                  urlFailCounter ++;
                } else {
                  urlFailCounter = 0 ;
                }
          }
        } else {
            logOutput((String)"[HTTP] GET... failed, error: " + http.errorToString(httpCode).c_str());
            httpFailCounter++;
        }

        http.end(); //Free the resources

        } else if (URL == "URL Not Set") {
            String values_config[3];
            File configRead = SPIFFS.open("/configDisplay.txt", "r");
            if(!configRead) logOutput((String)": Couldn't open configDisplay to read URL!");
            fileReadLines(configRead, values_config);
            URL = values_config[0];
            configRead.close();
        } else {
            logOutput("WARNING: URL Link is invalid ! Please enter another URL");
        }
      if(urlFailCounter > 1) {
        SPIFFS.remove("/configDisplay.txt");
        logOutput("ERROR: URL Body format incorrect ! Couldn't find parking direction-sign !");
        logOutput("Please check chapter 5.1 of the user guide !");
        scroll_text("URL Body format incorrect ! Couldn't find parking direction-sign !",LED_RED_HIGH,20);
        scroll_text("URL Body format incorrect ! Couldn't find parking direction-sign !",LED_RED_HIGH,30);
        delay(100);
        restart_sequence(5);
      }
      if(httpFailCounter > 4) {
        server.reset();
        delay(500);
        ESP.restart();
      }
      if(Brightness != NULL && Brightness.length() > 0 && Brightness.toInt() != oldBright) {
        oldBright = Brightness.toInt();
        matrixLayer.setBrightness((oldBright*255)/100);
      } 
      if(urlRefreshRate.toInt()*1000 >= 1000) {
        delay(urlRefreshRate.toInt()*1000);
      } else {
        delay(3000);
      }

      if(tag == -1) {
        tag = oldTag;
      }
      if(tag == 0) {  // #> FSPACE
        go_left(free_spaces, arrow);      
      } else if (tag > 0) { // FSPACE <#
          go_right(free_spaces, arrow);
      }
    } else if(WiFi.status() != WL_CONNECTED) {
      matrix->clear();
      matrix->show();
      logOutput("There is no WiFi Connection !");
      WiFi.disconnect();
      delay(100);
      startSTA(v);
    } // if (WiFi.status() == WL_CONNECTED)
  } // if(!isAPmodeOn)
}
