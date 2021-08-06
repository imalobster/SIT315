// Pins
const uint8_t PIN_LEDa = 12;
const uint8_t PIN_LEDb = 13;
const uint8_t PIN_MTN = 2;
const uint8_t PIN_SWT = 3;

// States
volatile bool ledState_a = LOW;
volatile bool ledState_b = LOW;

// Main
void setup()
{
  	// Initialise serial connection
  	Serial.begin(9600);
  
  	// Set pin modes
	pinMode(PIN_LEDa, OUTPUT);
  	pinMode(PIN_LEDb, OUTPUT);
  	pinMode(PIN_MTN, INPUT);
  	pinMode(PIN_SWT, INPUT);
  
   	// Define interrupt functions
	attachInterrupt(digitalPinToInterrupt(PIN_MTN), LedSwitch_a, CHANGE);
  	attachInterrupt(digitalPinToInterrupt(PIN_SWT), LedSwitch_b, CHANGE);
}

void loop()
{

}

void LedSwitch_a()
{
  	if (ledState_b == LOW)
	{
	  	Serial.println("[INFO]: Interrupt fired, but switch is off...");
	  	return;
	}
	ledState_a = !ledState_a;
  	digitalWrite(PIN_LEDa, ledState_a);
  
	if (ledState_a == HIGH)
		Serial.println("[INFO]: Interrupt fired, motion detected");
	else
		Serial.println("[INFO]: Interrupt fired, motion no longer detected");
  
  	Serial.println("[INFO]: Red LED stated changed to " + (String)ledState_a);
}

void LedSwitch_b()
{
	ledState_b = !ledState_b;
  	digitalWrite(PIN_LEDb, ledState_b);
  
	if (ledState_b == HIGH)
		Serial.println("[INFO]: Interrupt fired, switch turned on");
	else
		Serial.println("[INFO]: Interrupt fired, switch turned off");
  
  	Serial.println("[INFO]: Blue LED stated changed to " + (String)ledState_b);
}

