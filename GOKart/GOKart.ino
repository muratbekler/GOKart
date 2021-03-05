/*
 Name:		GOKart.ino
 Created:	2019-11-13 5:22:16 PM
 Author:	murat
*/

// the setup function runs once when you press reset or power the board
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h> 

#include <TinyGPS++.h> 
#include <SoftwareSerial.h>


static const uint8_t D0 = 16;
static const uint8_t D1 = 5;
static const uint8_t D2 = 4;
static const uint8_t D3 = 0;
static const uint8_t D4 = 2;
static const uint8_t D5 = 14;
static const uint8_t D6 = 12;
static const uint8_t D7 = 13;
static const uint8_t D8 = 15;
static const uint8_t RX = 3;
static const uint8_t TX = 1;

TinyGPSPlus gps;
SoftwareSerial ss(D6, D5);
Adafruit_PCD8544 display = Adafruit_PCD8544(D4, D3, D2, D1, D0);
const char* ssid = "GOKART";
const char* password = "123456789";

float t = 0.0;
float h = 0.0;

float latitude, longitude;
double kmh;
int year, month, date, hour, minute, second;
String date_str, time_str, lat_str, lng_str;

int pm;

AsyncWebServer server(80);

unsigned long previousMillis = 0;

const long interval = 10000;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>GOKart Server</h2>
  <p>
    <span class="dht-labels">Latitude</span> 
    <span id="latitude">%latitude%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <span class="dht-labels">Longitude</span>
    <span id="longitude">%longitude%</span>
    <sup class="units">&deg;C</sup>
  </p>
 <p>
    <span class="dht-labels">Speed</span>
    <span id="kmh">%kmh%</span>
    <sup class="units">km/h</sup>
  </p>
  <p>
    <span class="dht-labels">Date</span>
    <span id="date">%date%</span>
    <sup class="units"></sup>
  </p>
  <p>
    <span class="dht-labels">Time</span>
    <span id="time">%time%</span>
    <sup class="units"></sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("latitude").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/latitude", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("longitude").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/longitude", true);
  xhttp.send();
}, 1000 ) ; 
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("date").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/date", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("time").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/time", true);
  xhttp.send();
}, 1000 ) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("kmh").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/kmh", true);
  xhttp.send();
}, 1000 ) ;

</script>
</html>)rawliteral";

String processor(const String& var) {
	return String();
}

void setup() {
	Serial.begin(9600);
	ss.begin(9600);
	Serial.print("AP (Access Point)…");
	WiFi.softAP(ssid, password);

	IPAddress IP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(IP);

	Serial.println(WiFi.localIP());

	// Route for root / web page
	server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send_P(200, "text/html", index_html, processor);
		});
	server.on("/latitude", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send_P(200, "text/plain", String(latitude).c_str());
		});
	server.on("/longitude", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send_P(200, "text/plain", String(longitude).c_str());
		});
	server.on("/date", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send_P(200, "text/plain", String(date_str).c_str());
		});
	server.on("/time", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send_P(200, "text/plain", String(time_str).c_str());
		});
	server.on("/kmh", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send_P(200, "text/plain", String(kmh).c_str());
		});

	// Start server
	server.begin();

	display.begin();
	display.setContrast(50); // Set LCD contrast from here.
	analogWrite(D4, 255); // PWM brightness should vary from 255 to 0.
	display.clearDisplay(); introdisplay();
	display.clearDisplay(); wifiinfodisplay();
}

void loop() {
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis >= interval) {
		previousMillis = currentMillis;
	}
	gpsr();
}
void gpsr()
{
	if(ss.available() > 0) //while data is available	
	{
		int dato = ss.read();
		if (gps.encode(dato)) //read gps data
		{
			if (gps.location.isValid()) //check whether gps location is valid
			{
				latitude = gps.location.lat();
				lat_str = String(latitude, 6); // latitude location is stored in a string
				longitude = gps.location.lng();
				lng_str = String(longitude, 6); //longitude location is stored in a string
			}
			if (gps.date.isValid()) //check whether gps date is valid
			{
				date_str = "";
				date = gps.date.day();
				month = gps.date.month();
				year = gps.date.year();
				if (date < 10)
					date_str = "0";
				date_str += String(date);// values of date,month and year are stored in a string
				date_str += " / ";

				if (month < 10)
					date_str += '0';
				date_str += String(month); // values of date,month and year are stored in a string
				date_str += " / ";
				if (year < 10)
					date_str += '0';
				date_str += String(year); // values of date,month and year are stored in a string
			}
			if (gps.time.isValid())  //check whether gps time is valid
			{
				time_str = "";
				hour = gps.time.hour();
				minute = gps.time.minute();
				second = gps.time.second();
				minute = (minute + 30); // converting to IST
				if (minute > 59)
				{
					minute = minute - 60;
					hour = hour + 1;
				}
				hour = (hour + 5);
				if (hour > 23)
					hour = hour - 24;   // converting to IST
				if (hour >= 12)  // checking whether AM or PM
					pm = 1;
				else
					pm = 0;
				hour = hour % 12;
				if (hour < 10)
					time_str = "0";
				time_str += String(hour); //values of hour,minute and time are stored in a string
				time_str += " : ";
				if (minute < 10)
					time_str += '0';
				time_str += String(minute); //values of hour,minute and time are stored in a string
				time_str += " : ";
				if (second < 10)
					time_str += '0';
				time_str += String(second); //values of hour,minute and time are stored in a string
				if (pm == 1)
					time_str += " PM ";
				else
					time_str += " AM ";
			}
			if (gps.speed.value() > 0) {
				kmh = gps.speed.kmph();
			}
			else
				kmh = 0;
		}
		display.println(gps.satellites.value());
		display.display();
		delay(1000);
	}
}
void introdisplay(void) {
	display.setTextSize(1);
	display.setCursor(0, 0);
	display.setTextColor(BLACK);
	display.println("ESP8266");
	display.setCursor(0, 13);
	display.println("GOKart");
	display.fillRect(0, 23, 84, 24, BLACK);

	delay(2000);
}
void wifiinfodisplay(void) {
	display.setTextSize(1);
	display.setCursor(0, 0);
	display.setTextColor(WHITE, BLACK);
	display.println("WiFi Connected");
	display.setCursor(0, 16);
	display.setTextColor(BLACK);
	display.print(""); display.println(ssid);
	display.print(""); display.println(WiFi.localIP());
	display.print(""); display.print("RSSI:");
	display.print(WiFi.RSSI());
	display.println(" dBm");
	display.display();
	delay(2000);
}
