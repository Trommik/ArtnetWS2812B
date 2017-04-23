/*
 Name:		ArtnetWS2812B.ino
 Created:	24.03.2017 16:56:20
 Author:	Thomas
*/


//Includes
#include "Adafruit_NeoPixel.h" // Neopixel Libary for WS2812B 

#include "ESP8266WiFi.h" // ESP8266 Core Functions
#include "ArtnetWifi.h" // Artnet Libary

//Game Includes Pong
#include "PongGame.h"
#include "Puck.h"
#include "Paddel.h"


//Led settings
#define NUM_LEDS 290
#define DATA_PIN 14


//Mode settings
int mode;

#define MOD_A 4
#define MOD_B 5

#define GLEDIATOR_MODE 1
#define SNAKE_MODE 2
#define PONG_MODE 3
#define DEBUG_ART_MODE 4


//ArtNet settings
#define START_UNIVERSE 0 // universe which starts with pixel 0 
#define CHANNELS  NUM_LEDS * 3 // Total number of channels you want to receive (1 led = 3 channels)
#define MAX_UNIVERSES ((CHANNELS / 512) + ((CHANNELS % 512) ? 1 : 0))

bool universesReceived[MAX_UNIVERSES];
bool sendFrame = 1;
int previousDataLength = 0;


//Wifi settings
const char* ssid = "Speedport Schwark 2,4 GHz";
const char* password = "4858035152347806";


//Create objects
ArtnetWifi artnet; // Initialize the artnet libary
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800); // Initialize the led strip


// connect to wifi and returns true if successful or false if not
boolean ConnectWifi(void) {

	boolean state = true;
	int i = 0;

	WiFi.mode(WIFI_STA);

	WiFi.begin(ssid, password);
	Serial.println("");
	Serial.println("Connecting to WiFi");

	// Wait for connection
	Serial.print("Connecting");
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
		if (i > 20) {
			state = false;
			break;
		}
		i++;
	}
	if (state) {
		Serial.println("");
		Serial.print("Connected to ");
		Serial.println(ssid);
		Serial.print("IP address: ");
		Serial.println(WiFi.localIP());
		strip.setPixelColor(0, strip.Color(0, 255, 0)); // set buffer pixel to green for OK WiFi connection 
		strip.show();
	}
	else {
		Serial.println("");
		Serial.println("Connection failed.");
		strip.setPixelColor(0, strip.Color(255, 0, 0)); // set buffer pixel to red for FAILED WiFi connection 
		strip.show();
	}

	return state;
}


//different ArtNet callbacks
void printDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
	boolean tail = false;

	Serial.print("DMX: Univ: ");
	Serial.print(universe, DEC);
	Serial.print(", Seq: ");
	Serial.print(sequence, DEC);
	Serial.print(", Data (");
	Serial.print(length, DEC);
	Serial.print("): ");

	if (length > 16) {
		length = 16;
		tail = true;
	}
	// send out the buffer
	for (int i = 0; i < length; i++)
	{
		Serial.print(data[i], HEX);
		Serial.print(" ");
	}
	if (tail) {
		Serial.print("...");
	}
	Serial.println();
}

void onInpFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
	/*Serial.print("DMX: Univ: ");
	Serial.print(universe, DEC);
	Serial.print(", Seq: ");
	Serial.print(sequence, DEC);
	Serial.print(", Axes: ");
	Serial.print(data[0], DEC);
	Serial.print(", Btns: ");
	Serial.print(data[1], DEC);
	Serial.print(", Data (");
	Serial.print(length-2, DEC);
	Serial.print("): ");

	// send out the buffer
	for (int i = 2; i < length; i++)
	{
		Serial.print(data[i], HEX);
		Serial.print(" ");
	}
	Serial.println();*/

	int axes = data[0];
	int btns = data[1];

	double *axisVal;                //Declare pointer to type of array
	axisVal = new double[axes];   //use 'new' to create array of size x
	
	for (int a = 0; a < axes; a++){
		union { char buf[sizeof(double)]; double _axisVal; };


		for (int i = 0; i < sizeof(double); i++) // 8times
		{
			buf[i] = data[(a*sizeof(double)) + i + 2];
		}

		Serial.print("Axis ");
		Serial.print(a);
		Serial.print(": ");
		Serial.print(_axisVal);
		Serial.println();
		axisVal[a] = _axisVal;
	}

	delete[] axisVal;       //remeber to free memeory when finished.
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
	sendFrame = 1;

	// Store which universe has got in
	if (universe - START_UNIVERSE < MAX_UNIVERSES)
		universesReceived[universe] = 1;

	for (int i = START_UNIVERSE; i < MAX_UNIVERSES; i++)
	{
		if (universesReceived[i] == 0)
		{
			sendFrame = 0;
			break;
		}
	}

	// read universe and put into the right part of the display buffer
	for (int i = 0; i < length / 3; i++)
	{
		int led = i + (universe - START_UNIVERSE) * (previousDataLength / 3);
		if (led < NUM_LEDS)
			strip.setPixelColor(led+1, data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
	}
	previousDataLength = length;

	if (sendFrame)
	{
		strip.show(); // show strip
		memset(universesReceived, 0, MAX_UNIVERSES); // Reset universeReceived to 0
	}
}


//Mode selection
int getMode()
{
	pinMode(MOD_A, INPUT);
	pinMode(MOD_B, INPUT);

	bool a = digitalRead(MOD_A);
	bool b = digitalRead(MOD_B);

	int _mode = DEBUG_ART_MODE;

	if (!a && !b)
		_mode = GLEDIATOR_MODE;
	if (a && !b)
		_mode = SNAKE_MODE;
	if (!a && b)
		_mode = PONG_MODE;
	if (a && b)
		_mode = DEBUG_ART_MODE;

	return _mode;
}

void initMode()
{
	mode = getMode();

	switch (mode)
	{
	case GLEDIATOR_MODE:
		Serial.println("Mode: GLEDIATOR_MODE");
		strip.setPixelColor(0, strip.Color(255, 255, 0)); // set buffer pixel to green for OK WiFi connection 
		strip.show();
		artnet.setArtDmxCallback(onDmxFrame); // this will be called for each artnet packet received
		//GLEDIATOR MODE
		break;
	case SNAKE_MODE:
		Serial.println("Mode: SNAKE_MODE");
		strip.setPixelColor(0, strip.Color(0, 255, 255)); // set buffer pixel to green for OK WiFi connection
		artnet.setArtInpCallback(onInpFrame);
		strip.show();
		//SNAKE MODE
		break;
	case PONG_MODE:
		Serial.println("Mode: PONG_MODE");
		strip.setPixelColor(0, strip.Color(0, 255, 255)); // set buffer pixel to green for OK WiFi connection
		artnet.setArtInpCallback(onInpFrame);
		strip.show();

		puck.init();
		leftPlayer.init(LEFT_SIDE);
		rightPlayer.init(RIGHT_SIDE);
		//PONG MODE
		break;
	case DEBUG_ART_MODE:
		Serial.println("Mode: DEBUG_MODE");
		strip.setPixelColor(0, strip.Color(255, 0, 255)); // set buffer pixel to green for OK WiFi connection 
		strip.show();
		artnet.setArtDmxCallback(printDmxFrame); // this will be called for each artnet packet received and outputs it to the serial monitor
		//DEBUG MODE
		break;
	default:
		strip.setPixelColor(0, strip.Color(255, 0, 255)); // set buffer pixel to green for OK WiFi connection 
		strip.show();
		artnet.setArtDmxCallback(printDmxFrame); // this will be called for each artnet packet received and outputs it to the serial monitor
		// DEBUG MODE
		break;
	}
}


//Pong move player
void paddelsMove(){}


//Render led Strip for the games
void renderStrip()
{
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			color c = pong.getColor(x, y);

			int index;
			if (y % 2) {
				index = (y * HEIGHT) + (WIDTH - 1) - x;
			}
			else {
				index = y * HEIGHT + x;
			}

			strip.setPixelColor(index, strip.Color(c.r, c.g, c.b));
		}
	}
	strip.show();
	pong.reset();
}


//Setup
void setup()
{
	Serial.begin(115200); // Init Serial at 115200 Baud

	strip.begin(); // Init strip
	strip.setPixelColor(0, strip.Color(0, 0, 255)); // set buffer pixel to blue for INIT WiFi connection 
	strip.show(); // Show strip

	ConnectWifi(); // Connect to WiFi

	initMode(); // set the right mode and set the different artnet/neopixel/game Parameters

	artnet.begin(); // Init artnet libary
}

//Loop
void loop()
{
	artnet.read(); // Read artnet
	if (mode == SNAKE_MODE){

	}else if (mode == PONG_MODE){
		paddelsMove();

		puck.update();

		for (int off = 0; off < leftPlayer.getHeight(); off++) {
			pong.set(leftPlayer.render(off));
		}
		for (int off = 0; off < rightPlayer.getHeight(); off++) {
			pong.set(rightPlayer.render(off));
		}

		pong.setScore(scoreRight, RIGHT_SIDE); // right side = top side
		pong.setScore(scoreLeft, LEFT_SIDE); // left side = bottom side


		pong.set(puck.render()); // render Puck
		pong.set(puck.renderLast()); // render last Puck 
		pong.set(puck.renderLastLast()); // render lastlast Puck 

		renderStrip();

		delay(100);
	}
}
