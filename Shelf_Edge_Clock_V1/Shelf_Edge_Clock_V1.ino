/*
   3D printed smart shelving with a giant hidden digital clock in the front edges of the shelves - DIY Machines

  ==========

  More info and build instructions: https://www.youtube.com/watch?v=8E0SeycTzHw

  3D printed parts can be downloaded from here: https://www.thingiverse.com/thing:4207524

  You will need to install the Adafruit Neopixel library which can be found in the library manager.

  This project also uses the handy DS3231 Simple library:- https://github.com/sleemanj/DS3231_Simple   Please follow the instruction on installing this provided on the libraries page

  Before you install this code you need to set the time on your DS3231. Once you have connected it as shown in this project and have installed the DS3231_Simple library (see above) you
  to go to  'File' >> 'Examples' >> 'DS3231_Simple' >> 'Z1_TimeAndDate' >> 'SetDateTime' and follow the instructions in the example to set the date and time on your RTC

  ==========

   SAY THANKS:

  Buy me a coffee to say thanks: https://ko-fi.com/diymachines
  Support us on Patreon: https://www.patreon.com/diymachines

  SUBSCRIBE:
  ■ https://www.youtube.com/channel/UC3jc4X-kEq-dEDYhQ8QoYnQ?sub_confirmation=1

  INSTAGRAM: https://www.instagram.com/diy_machines/?hl=en
  FACEBOOK: https://www.facebook.com/diymachines/
*/

// Wifi Includes
#include <WiFiNINA.h>
#include <WiFiUdp.h>

// Includes for Time
#include <NTPClient.h>
#include <TimeLib.h>
#include <Timezone.h>
#include "timeRules.h"

// Includes for LED strip
#include <Adafruit_NeoPixel.h>

// include git ignored settings
#include <wifiCreds.h>

// User Config Variables
const int sync_time = 120;     // Time in seconds to Sync NTP Time

#define tzCount 24
timeZones tzs[tzCount] = {
  {usPT, "Pacific"},    // LOCAL TIMEZONE
  {UTC, "Universal"},   // UTC  0
  {usET, "Eastern"},    // UTC -5
  {usCT, "Central"},    // UTC -6
  {usMT, "Mountain"},   // UTC -7
  {usAZ, "Arizona"},    // UTC -7 (no DST)
  {usPT, "Pacific"},     // UTC -8
  {usPT, "Moo1"},
  {usPT, "Moo2"},
  {usPT, "Moo3"},
  {usPT, "Moo4"},
  {usPT, "Moo5"},
  {usPT, "Moo6"},
  {usPT, "Moo7"},
  {usPT, "Moo8"},
  {usPT, "Moo9"},
  {usPT, "Moo10"},
  {usPT, "Moo11"},
  {usPT, "Moo12"},
  {usPT, "Moo13"},
  {usPT, "Moo14"},
  {usPT, "Moo15"},
  {usPT, "Moo16"},
  {usPT, "Moo17"}
};

// Wifi Objects
WiFiUDP ntpUDP;

// Time Objects
NTPClient timeClient(ntpUDP);

// Function prototypes
time_t syncNTPTime();

// Create a variable to hold the time data
time_t MyDateAndTime;

// Color variables, assigned in setup()
uint32_t clockMinuteColour;
uint32_t clockHourColour;

// Globals
int loop_interval = 1000;

// Which pin on the Arduino is connected to the NeoPixels?
constexpr auto LEDCLOCK_PIN = 6;
constexpr auto LEDDOWNLIGHT_PIN = 5;

// How many NeoPixels are attached to the Arduino?
constexpr auto LEDCLOCK_COUNT = 207;
constexpr auto LEDDOWNLIGHT_COUNT = 12;


int clockFaceBrightness = 0;

// Declare our NeoPixel objects:
Adafruit_NeoPixel stripClock(LEDCLOCK_COUNT, LEDCLOCK_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripDownlighter(LEDDOWNLIGHT_COUNT, LEDDOWNLIGHT_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)


//Smoothing of the readings from the light sensor so it is not too twitchy
const int numReadings = 6;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
long total = 0;                 // the running total
long average = 0;               // the average

void setup() {
	Serial.begin(9600);
	// Wifi Setup
	WiFi.begin(ssid, pass);
	Serial.print("Connecting to Wifi: ");
	while (WiFi.status() != WL_CONNECTED) {
		delay(5);
		Serial.print(".");
	}
	Serial.println(" Connected!");

	// Time Setup
	timeClient.begin(0);
	setSyncProvider(syncNTPTime);
	setSyncInterval(sync_time);

	stripClock.begin();           // INITIALIZE NeoPixel stripClock object (REQUIRED)
	stripClock.show();            // Turn OFF all pixels ASAP
	stripClock.setBrightness(100); // Set inital BRIGHTNESS (max = 255)

	stripDownlighter.begin();           // INITIALIZE NeoPixel stripClock object (REQUIRED)
	stripDownlighter.show();            // Turn OFF all pixels ASAP
	stripDownlighter.setBrightness(50); // Set BRIGHTNESS (max = 255)

	//smoothing
	// initialize all the readings to 0:
	for (int thisReading = 0; thisReading < numReadings; thisReading++) {
		readings[thisReading] = 0;
	}

	clockMinuteColour = stripClock.Color(148, 000, 211); // pure red
	clockHourColour = stripClock.Color(148, 000, 211); // pure green
}

void loop() {
	//read the time
	readTheTime();

	//display the time on the LEDs
	displayTheTime();

	//Record a reading from the light sensor and add it to the array
	readings[readIndex] = analogRead(A0); //get an average light level from previouse set of samples
	Serial.print("Light sensor value added to array = ");
	Serial.println(readings[readIndex]);
	readIndex = readIndex + 1; // advance to the next position in the array:

	// if we're at the end of the array move the index back around...
	if (readIndex >= numReadings) {
		// ...wrap around to the beginning:
		readIndex = 0;
	}

	//now work out the sum of all the values in the array
	int sumBrightness = 0;
	for (int i = 0; i < numReadings; i++)
	{
		sumBrightness += readings[i];
	}
	Serial.print("Sum of the brightness array = ");
	Serial.println(sumBrightness);

	// and calculate the average:
	int lightSensorValue = sumBrightness / numReadings;
	Serial.print("Average light sensor value = ");
	Serial.println(lightSensorValue);

	//set the brightness based on ambiant light levels
	clockFaceBrightness = map(lightSensorValue, 50, 1000, 200, 1);
	stripClock.setBrightness(clockFaceBrightness); // Set brightness value of the LEDs
	Serial.print("Mapped brightness value = ");
	Serial.println(clockFaceBrightness);

	stripClock.show();

	//(red * 65536) + (green * 256) + blue ->for 32-bit merged colour value so 16777215 equals white
	stripDownlighter.fill(16777215, 0, LEDDOWNLIGHT_COUNT);
	stripDownlighter.show();

	delay(5000);   //this 5 second delay to slow things down during testing
}

void readTheTime() {
	// Timezone Data
	TimeChangeRule* tcr;
	Timezone tz = tzs[0].tz;
	time_t utc = now();
	MyDateAndTime = tz.toLocal(utc, &tcr);

	// And use it
	Serial.println("");

	// buffer to store a text to display
	char timeBuffer[32];
	sprintf(timeBuffer, "Time is: %2d:%02d:%02d", hour(MyDateAndTime), minute(MyDateAndTime), second(MyDateAndTime));
	Serial.println(timeBuffer);
	sprintf(timeBuffer, "Date is: %02d/%02d/%4d", month(MyDateAndTime), day(MyDateAndTime), year(MyDateAndTime));
	Serial.println(timeBuffer);
}

void displayTheTime() {
	stripClock.clear(); //clear the clock face

	int firstMinuteDigit = minute(MyDateAndTime) % 10; //work out the value of the first digit and then display it
	displayNumber(firstMinuteDigit, 0, clockMinuteColour);

	int secondMinuteDigit = floor(minute(MyDateAndTime) / 10); //work out the value for the second digit and then display it
	displayNumber(secondMinuteDigit, 63, clockMinuteColour);

	int firstHourDigit = hour(MyDateAndTime); //work out the value for the third digit and then display it
	if (firstHourDigit > 12) {
		firstHourDigit = firstHourDigit - 12;
	}

	// Comment out the following three lines if you want midnight to be shown as 12:00 instead of 0:00
	//  if (firstHourDigit == 0){
	//    firstHourDigit = 12;
	//  }

	firstHourDigit = firstHourDigit % 10;
	displayNumber(firstHourDigit, 126, clockHourColour);

	int secondHourDigit = hour(MyDateAndTime); //work out the value for the fourth digit and then display it

	// Comment out the following three lines if you want midnight to be shwon as 12:00 instead of 0:00
	//  if (secondHourDigit == 0){
	//    secondHourDigit = 12;
	//  }

	if (secondHourDigit > 12) {
		secondHourDigit = secondHourDigit - 12;
	}
	if (secondHourDigit > 9) {
		stripClock.fill(clockHourColour, 189, 18);
	}
}

void displayNumber(int digitToDisplay, int offsetBy, uint32_t colourToUse) {
	switch (digitToDisplay) {
	case 0:
		digitZero(offsetBy, colourToUse);
		break;
	case 1:
		digitOne(offsetBy, colourToUse);
		break;
	case 2:
		digitTwo(offsetBy, colourToUse);
		break;
	case 3:
		digitThree(offsetBy, colourToUse);
		break;
	case 4:
		digitFour(offsetBy, colourToUse);
		break;
	case 5:
		digitFive(offsetBy, colourToUse);
		break;
	case 6:
		digitSix(offsetBy, colourToUse);
		break;
	case 7:
		digitSeven(offsetBy, colourToUse);
		break;
	case 8:
		digitEight(offsetBy, colourToUse);
		break;
	case 9:
		digitNine(offsetBy, colourToUse);
		break;
	default:
		break;
	}
}

/*
   syncNTPTime();

   Called as a callback on a timer every sync_time seconds.
   ========================================================================= */
time_t syncNTPTime() {
	unsigned long cur_time, update_time;
	unsigned int drift_time;
	cur_time = timeClient.getEpochTime();
	timeClient.update();
	update_time = timeClient.getEpochTime();
	drift_time = (update_time - cur_time);
	Serial.println("NTP Time Sync <=====================================");
	Serial.print("NTP Epoch: "); Serial.println(timeClient.getEpochTime());
	Serial.print("NTP Time : "); Serial.println(timeClient.getFormattedTime());
	Serial.print("Epoch Pre Sync:  "); Serial.println(cur_time);
	Serial.print("Epoch Post Sync: "); Serial.println(update_time);
	Serial.print("Drift Correct:   "); Serial.println(drift_time);

	return timeClient.getEpochTime();
}
