#include <SPI.h>
#include <MFRC522.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

//----------------------------//

#ifndef STASSID
#define STASSID "Hung" // Your WiFi SSID
#define STAPSK  "123" //Your WiFi password
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);
//SIM800 Definition
#define TX D4
#define RX D8
SoftwareSerial mySerial(D4, D8);

//RC522 Definition
#define SS_PIN  D0
#define RST_PIN D3
MFRC522 mfrc522(SS_PIN, RST_PIN);

//Definition DHT Sensor
const int refresh = 3;
#define DHTTYPE DHT11   
#define DHTPin  10
DHT dht(DHTPin, DHTTYPE);
float tValue;
float hValue;
int lcdColumns = 16;
int lcdRows = 2;
//Definiton for LCD I2C
LiquidCrystal_I2C lcd(0x27,lcdColumns, lcdRows);

String messageToScroll ="Measuring...";

void scrollText(int row, String message, int delayTime, int lcdColumns) {
  for (int i=0; i < lcdColumns; i++) {
    message = " " + message;  
  } 
  message = message + " "; 
  for (int pos = 0; pos < message.length(); pos++) {
    lcd.setCursor(0, row);
    lcd.print(message.substring(pos, pos + lcdColumns));
    delay(delayTime);
  }
}
void sendTemp() {

  String page = "<!DOCTYPE html>\n\n";
  page +="    <meta http-equiv='refresh' content='";
  page += String(refresh);// how often temperature is read
  page +="'/>\n";  
  page +="<html>\n";
  page +="<body>\n"; 
  page +="<h1>ESP8266</h1>\n";    
  page +="<p style=\"font-size:50px;\">Temperature: \n";  
  page +="<p style=\"color:red; font-size:50px;\">";
  page += String(tValue, 2);
  page +="</p>\n";
  page +="<p style=\"font-size:50px;\">Humidity: \n";
  page +="<p style=\"color:red; font-size:50px;\">";
  page += String(hValue, 2);
  page +="</p>\n";  
  page +="</body>\n";  
  page +="</html>\n";  
 server.send(200,  "text/html",page);
}

void handleNotFound() {
 
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() 
{
        lcd.init();       // start the LCD              
        lcd.backlight(); // start the LCD
        pinMode(DHTPin, INPUT);
        dht.begin();// start the DHT sensor
        Serial.begin(115200);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("robojaxDHT")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", sendTemp);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
        SPI.begin();
        mfrc522.PCD_Init();   // Start the RC522 reader  
        Serial.begin(115200);
        mySerial.begin(115200);
        Serial.println("Initializing...");
        delay(1000);

        // checking connection with the SIM800L
        mySerial.println("AT"); //Check connection with ESP8266
        updateSerial();
        mySerial.println("AT+CSQ"); //Check signal of SIM800L
        updateSerial();
        delay(500);
        mySerial.println("AT+CCID"); //Read Sim 
        updateSerial();
        mySerial.println("AT+CREG?"); //Check network
        updateSerial();
        
       
        
        lcd.setCursor(0,0);
        lcd.print("ESP8266 Project");
        lcd.setCursor(0,1);
        lcd.print("Welcome");
        delay(3000);     
}

void loop() 
{       
        bool access=0;
        char Temp[50] = "Temp: ";
        char Humid[50]= "Humid: ";
        int voltage = analogRead(A0);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Curr Vol: ");
        lcd.setCursor(0,1);
        lcd.print(voltage);
        delay(2000);
        
        bool fail_to_read = 0;
        float Temperature = dht.readTemperature();
        float Humidity = dht.readHumidity();
            if(isnan(Temperature) || isnan(Humidity))
            {     
                 fail_to_read = 1;
                 lcd.clear();
                 lcd.setCursor(0,0);
                 lcd.print("Failed to read...");
                 delay(2000);
            }
            else
            {
                lcd.clear();
                lcd.setCursor(0,0);
                //lcd.print("Got something...");
                scrollText(0, messageToScroll, 150, lcdColumns); 
                delay(1000);              
                char cal_C[10];
                char cal_H[10];
                dtostrf(Temperature, 6, 2, cal_C);
                dtostrf(Humidity, 6, 2, cal_H); 
                strcat(Temp, cal_C);
                strcat(Humid, cal_H);
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print(Temp);
                lcd.setCursor(0,1);
                lcd.print(Humid);                   
                delay(2000);         
            }
// look for new card
        if ( ! mfrc522.PICC_IsNewCardPresent()) 
        {
          return;
        }
        // Select one of the cards
        if ( ! mfrc522.PICC_ReadCardSerial()) 
        {
          return;
        }     

        Serial.print("UID Tag: ");
        String content = "";
        byte letter;
        for (byte i = 0; i < mfrc522.uid.size; i++) 
            {
                 Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
                 Serial.print(mfrc522.uid.uidByte[i], HEX);
                 content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
                 content.concat(String(mfrc522.uid.uidByte[i], HEX));
            }
        Serial.println();
        Serial.print("Message: ");
        content.toUpperCase();
        if (content.substring(1) == "79 3D 85 9D") //change here the UID of the card/cards that you want to give access
        {
          Serial.println("Access granted");
          access=1;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Access granted");
          delay(1000);
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Sending message");
          delay(1000);
        }      
       else   
       {
          Serial.println(" Access denied");
          access=0;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Access denied");          
          delay(2000);
        }          

       if (access==1 && !fail_to_read)
       {
          mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
          updateSerial();
          mySerial.println("AT+CMGS=\"+84905804889\"");//change ZZ with country code and xxxxxxxxxxx with phone number to sms
          updateSerial();
          mySerial.println(Temp); //text content
          updateSerial();
          mySerial.println(Humid); //text content
          updateSerial();
          mySerial.write(26);
       }
       else if(access==0)
       {
        mySerial.println("ATD+ +84905804889;"); //  change ZZ with country code and xxxxxxxxxxx with phone number to dial
       updateSerial();
       delay(10000); // wait for 10 seconds...
       mySerial.println("ATH"); //hang up
       updateSerial();
       }
       else 
       {
              mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
              updateSerial();
              mySerial.println("AT+CMGS=\"+84905804889\"");//change ZZ with country code and xxxxxxxxxxx with phone number to sms
              updateSerial();
              mySerial.println("Can't read value"); //text content
              updateSerial();
              mySerial.write(26);
       }
       server.handleClient();
  MDNS.update();
  float c = dht.readTemperature();// Read temperature as Celsius (the default)
  float h = dht.readHumidity();// Reading humidity 
  float f = dht.readTemperature(true);// Read temperature as Fahrenheit (isFahrenheit = true)
    Serial.println(c);
 tValue =c;
 hValue = h;
  delay(300);
}
void updateSerial()
{
        delay(500);
        while (Serial.available()) 
        {
          mySerial.write(Serial.read());//Forward what Serial received to Software Serial Port
        }
        while(mySerial.available()) 
        {
          Serial.write(mySerial.read());//Forward what Software Serial received to Serial Port
        }
}
