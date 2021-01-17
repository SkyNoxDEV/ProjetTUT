#include <ss_oled.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DHTesp.h>
#include <FS.h>

//////////////Web Server////////////////

char *ssid = "GEWIFITP1";
char *password = "geiigeii";

void handleRoot() ;
void handleNotFound();
ESP8266WebServer server(80);

///////////////////////////////////////

////////////////DHT/////////////////////

DHTesp dht;
float humidity;
float temperature;

///////////////////////////////////////

/////////////DUST SENSOR//////////////

int pin_dust = D5;
unsigned long duration=0;
unsigned long starttime;
unsigned long sampletime_ms = 10000;//sample period 10s ;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

///////////////////////////////////////

float moyenneTemp = 20.0; //valeurs initiales
float moyenneHumi = 50.0;
int moyenneQair = 80;
float moyenneDust = 100;

////////////////OLED SCREEN///////////

#define SDA_PIN D2
#define SCL_PIN D1

#define RESET_PIN -1

#define OLED_ADDR -1
// Rotation de l'écran
#define FLIP180 180
//Inverse le blanc et le noir
#define INVERT 0
// Bit-Bang the I2C bus
#define USE_HW_I2C 0

#define MY_OLED OLED_128x128
#define OLED_WIDTH 128
#define OLED_HEIGHT 128

SSOLED ssoled;

///////////////////////////////////

void setup() {
  
  Serial.begin(115200);   //initialisation moniteur serie

  dht.setup(D3, DHTesp::DHT22); //initialisation DHT
  
  pinMode(A0,INPUT);  //initialisation air quality sensor

  pinMode(pin_dust,INPUT);  //initialisation dust sensor

  starttime = millis();   //get the current time;


  /////////////ECRAN ATTENTE CONNEXION////////////////
  
  int rc = oledInit(&ssoled, MY_OLED, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, SDA_PIN, SCL_PIN, RESET_PIN, 400000L); // use standard I2C bus at 400Khz
  oledFill(&ssoled, 0, 1);
  oledWriteString(&ssoled, 0,10,0,(char *)"Projet tuteure", FONT_NORMAL, 0, 1);
  oledWriteString(&ssoled, 0,10,2,(char *)"Connexion", FONT_NORMAL, 0, 1);
  oledWriteString(&ssoled, 0,10,4,ssid, FONT_NORMAL, 0, 1);   //affiche SSID
  oledWriteString(&ssoled, 0,10,5,password, FONT_NORMAL, 0, 1);   //affiche mdp
  
  ////////////////////////////////////////////////// 

  //////////////////WEB SERVER/////////////////////
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); //connexion reseau
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  IPAddress ip= WiFi.localIP();
  Serial.print(ip);
  
  //////////////////////////////////////////////

  /*/////////////ECRAN CONNEXION ETABLIE//////////////
  
  char affichIP[50]="";
  IPAddress ip= WiFi.localIP();
  sprintf(affichIP,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);  //affiche IP utilisée sur ecran
  oledFill(&ssoled, 0, 1);
  oledWriteString(&ssoled, 0,10,0,(char *)"Projet tuteure", FONT_NORMAL, 0, 1);
  oledWriteString(&ssoled, 0,0,2,affichIP, FONT_NORMAL, 0, 1);

  ///////////////////////////////////////////////////*/

  
  MDNS.begin("Code_total_dynamique");
  
  server.on("/", handleRoot); //page INDEX
  server.on("/getUptime",handleUptime); //page uptime
  server.on("/getTemp",handleTemp);
  server.on("/avisTemp",handleAvisT); 
  server.on("/getHumid",handleHumid);
  server.on("/avisHumi",handleAvisH);
  server.on("/getQair",handleQair);
  server.on("/avisQair",handleAvisQair);
  server.on("/getPart",handlePart);
  server.on("/avisPart",handleAvisPart);
  
  server.onNotFound(handleNotFound);  //si rien trouve redirige vers page defaut : Error 404

  SPIFFS.begin(); //initialise service SPIFFS
  
  server.onNotFound([]() {              //Si client renseigne une URL
    if (!handleFileRead(server.uri())) //lui permettre l'acces si elle existe
      server.send(404, "text/plain", "404: Not Found"); // sinon repondre "Error 404"
  });
  
  server.begin();
}

////////////////////////////////////////////


void loop() {
  
  //////////////MESURES TOUTES LES 10s//////////
   
  if ((millis()-starttime) > sampletime_ms)
  {
      humidity = dht.getHumidity(); //mesure humidite
      temperature = dht.getTemperature(); //mesure temperature
  
      duration = pulseIn(pin_dust, LOW);
      lowpulseoccupancy = lowpulseoccupancy+duration;
      ratio = lowpulseoccupancy/(sampletime_ms*10.0);  // Integer percentage 0=>100
      concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
      lowpulseoccupancy = 0;
      starttime = millis();

      //moyenne glissante sur 20 valeurs
      moyenneTemp=(1-0.05)*moyenneTemp+0.05*temperature;  
      moyenneHumi=(1-0.05)*moyenneHumi+0.05*humidity;  
      moyenneQair=(1-0.05)*moyenneQair+0.05*analogRead(A0);  
      if(concentration>1)  //si inferieure a 1 -> bug
      {
      moyenneDust=(1-0.05)*moyenneDust+0.05*concentration;  
      }
  }
  
  //////////////////////////////////////////
  
  server.handleClient();
  MDNS.update();
}


void handleNotFound() { //cherche page demandee
  
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

///////////////PAGE PAR DEFAUT///////////////////////////

const char INDEX_HTML[] PROGMEM = R"=====(    
<html>  
<head><meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/></head>
<h3><center><a href="/DynamPage.html">Accéder à la page Analyz'Air</a></b3><br> 
<b><br>Esp Uptime = %ld milliseconds</center></b>
</html>
)=====";

////////////////////////////////////////////////////////

///////////////////FONCTION APPELLE DE PAGE//////////////////

void handleRoot()   //appelle page Index_HTML
{
 char temp[400]; // >= total size of the webpage to be served.
 snprintf(temp, 400, INDEX_HTML,millis());
 server.send(200, "text/html", temp);
}

void handleUptime()   //appelle page uptime
{
  String uptime = String(millis()/60000) ;
  server.send(200, "text/plain", uptime);
}

void handleTemp() 
{
  server.send(200, "text/plain", String(moyenneTemp));
}

void handleAvisT()
{
  String data;
  if(moyenneTemp<=30.0)
  {
    data="CORRECT";
  }
  else
  {
    data="DANGER";
  }
  
  server.send(200, "text/plain", data);
}

void handleHumid() 
{
  server.send(200, "text/plain", String(moyenneHumi));
}

void handleAvisH()
{
  String data;
  if(moyenneHumi<=65.0)
  {
    data="CORRECT";
  }
  else
  {
    data="DANGER";
  }
  
  server.send(200, "text/plain", data);
}

void handleQair() 
{
  server.send(200, "text/plain", String(moyenneQair));
}

void handleAvisQair()
{
  String data;
  if(moyenneQair<=90)
  {
    data="CORRECT";
  }
  else
  {
    data="DANGER";
  }
  
  server.send(200, "text/plain", data);
}

void handlePart() 
{
  server.send(200, "text/plain", String(moyenneDust));
}

void handleAvisPart()
{
  String data;
  if(moyenneDust<=800)
  {
    data="CORRECT";
  }
  else
  {
    data="DANGER";
  }
  
  server.send(200, "text/plain", data);
}


////////Convertit les extensions en MIME type///////////////////////////

String getContentType(String filename) {  
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".jpg")) return"image/jpeg";
  return "text/plain";
}

////////////////////////////////////////////////////////////////////////

/////////////////UTILISER FICHIER MEMOIRE SPIFFS////////////////////////

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it "r"=read
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}

////////////////////////////////////////////////////////////////////////
