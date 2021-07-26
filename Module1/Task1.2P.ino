// Pins
const uint8_t PIN_LED = 13;
const uint8_t PIN_MTN = 2;

// States
volatile bool ledState = LOW;

// Main
void setup()
{
  	// Initialise serial connection
  	Serial.begin(9600);
  	// Set pin modes
	pinMode(PIN_LED, OUTPUT);
  	pinMode(PIN_MTN, INPUT);
   	// Define interrupt functions
	attachInterrupt(digitalPinToInterrupt(PIN_MTN), LightSwitch, CHANGE);
}

void loop()
{
	// Set LED to state
	digitalWrite(PIN_LED, ledState);
	delay(100);
}

void LightSwitch()
{
	// Swap state
	ledState = !ledState;
	// Print interrupt details
	Serial.println("\n[INFO]: Motion detected, interrupt fired");
	Serial.println("[INFO]: LED stated changed to " + (String)ledState);
}
