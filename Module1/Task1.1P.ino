// Outputs
int yellowLed = 13;

// Inputs
int pinTemp = A1;

// Global vars
float tempThreshold = 25.0f;


// Main
void setup()
{
  	// Initialise serial connection
  	Serial.begin(9600);
  
  	// Set actuators
	pinMode(yellowLed, OUTPUT);
}

void loop()
{
  	// Get temperature from sensor and convert to celcius
	float temp = ConvertToCelcius(analogRead(pinTemp));

  	// Determine if LED should be on from temp threshold
  	if (temp >= tempThreshold)
      	digitalWrite(yellowLed, HIGH);
  	else
      	digitalWrite(yellowLed, LOW);
  
  	// Print the sensor and actuator values
  	Serial.println("Temperature: " + String(temp) + "C");
  	Serial.println("LED Status: " + String(digitalRead(yellowLed)));
  
  	// Wait one second
    delay(1000);
}

// Logic adapted from https://bc-robotics.com/tutorials/using-a-tmp36-temperature-sensor-with-arduino/
float ConvertToCelcius(int tmpVolt)
{
  	// Get temp from max voltage range (1024)
	float temp = (double)tmpVolt / 1024;
	temp = temp * 5;
	temp = temp - 0.5;
	temp = temp * 100;
  	return temp;
}