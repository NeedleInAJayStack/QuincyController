/*
 * Project DHT22sensor
 * Author:Jay Herron 
 * Date: 2019-03-26
 * 
 * This sketch reads the temperature and humidity from a DHT22 temp/humidity sensor and
 * publishes "celsius", "fahrenheit", and "humidity" to the cloud. 
 * 
 * Sensor documentation here: https://learn.adafruit.com/dht/connecting-to-a-dhtxx-sensor
 * 
 * PietteTech_DHT repository with code examples here: https://github.com/eliteio/PietteTech_DHT
 * 
 * I/O setup:
 * It is fairly easy to connect up to the DHT sensors. They have four pins
 * 
 *     1. VCC - red wire / left pin when sensor mesh cover is facing you
 *         - Connect to 3.3 - 5V power. Sometime 3.3V power isn't enough in which case try 5V power.
 *     2. Data out - white or yellow wire / second to left
 *     3. Not connected
 *     4. Ground - black wire / right
 * 
 * Simply ignore pin 3, its not used.
 * 
 * In our case, we will use the built-in 3.3V and ground pins on the Particle, and the data will be on pin D4.
 * 
 * You will want to place a 10 Kohm resistor between VCC and the data pin, to act as a medium-strength pull up
 * on the data line. The DHT22 often has a pullup already inside, but it doesn't hurt to add another one! 
 */

#include <PietteTech_DHT.h>

 // system defines
#define DHTTYPE DHT22               // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPIN D4                   // Digital pin for communications
#define LOOP_DELAY 5000             // 5s loop interval

// Instantiate DHT class
PietteTech_DHT DHT(DHTPIN, DHTTYPE);

// double celsius;
double fahrenheit;
double humidity;

void setup() {
    Serial.begin(9600);
    
    // Declare particle variables
    //Particle.variable("celsius", celsius);
    Particle.variable("fahrenheit", fahrenheit);
    Particle.variable("humidity", humidity);
    
    // Set up 
    while (!Serial.available() && millis() < 30000) {
        Serial.println("Press any key to start.");
        Particle.process();
        delay(1000);
    }
    Serial.println("DHT Simple program using DHT.acquireAndWait");
    Serial.print("LIB version: ");
    Serial.println(DHTLIB_VERSION);
    Serial.println("---------------");
    
    // Start the sensor
    DHT.begin();
}

// Looping function. Runs periodically to update the variables internally so they are 
// relatively current when requested via a variable request.
void loop(void) {
    
    Serial.print("\n");
    Serial.print(": Retrieving information from sensor: ");
    Serial.print("Read sensor: ");

    int result = DHT.acquireAndWait(1000); // wait up to 1 sec (default indefinitely)
    String error = "";
    switch (result) {
        case DHTLIB_OK:
            Serial.println("OK");
        break;
        case DHTLIB_ERROR_CHECKSUM:
            error = "Checksum";
        break;
        case DHTLIB_ERROR_ISR_TIMEOUT:
            error = "ISR time out";
        break;
        case DHTLIB_ERROR_RESPONSE_TIMEOUT:
            error = "Response time out";
        break;
        case DHTLIB_ERROR_DATA_TIMEOUT:
            error = "Data time out";
        break;
        case DHTLIB_ERROR_ACQUIRING:
            error = "Acquiring";
        break;
        case DHTLIB_ERROR_DELTA:
            error = "Delta time too small";
        break;
        case DHTLIB_ERROR_NOTSTARTED:
            error = "Not started";
        break;
        default:
            error = "Unknown";
        break;
    }

    if(error != "") { // We have an error
        Particle.publish("Error", error);
    }
    else {
        // Serial.print("Humidity (%): ");
        // Serial.println(DHT.getHumidity(), 2);
        
        // Serial.print("Temperature (oC): ");
        // Serial.println(DHT.getCelsius(), 2);
        
        // Serial.print("Temperature (oF): ");
        // Serial.println(DHT.getFahrenheit(), 2);
        
        // Serial.print("Temperature (K): ");
        // Serial.println(DHT.getKelvin(), 2);
        
        // Serial.print("Dew Point (oC): ");
        // Serial.println(DHT.getDewPoint());
        
        // Serial.print("Dew Point Slow (oC): ");
        // Serial.println(DHT.getDewPointSlow());
        
        // Assign to Particle variables
        fahrenheit = DHT.getFahrenheit();
        humidity = DHT.getHumidity();
        
        // Write to Particle events
        //Particle.publish("fahrenheit", String(fahrenheit), PRIVATE); // publish to cloud
        //Particle.publish("humidity", String(humidity), PRIVATE); // publish to cloud
    }
    
    delay(LOOP_DELAY); // 5 second delay
}