# Quincy Controller

Controls the light and temperature for our lizard Quincy. The following components are used, with the relevant wiring details.

1. A SHT3x sensor that detects temperature and humidity, communicating over I2C.
    a. 3V3 = Red
    b. GND = Black
    c. SCL = Yellow
    d. SDA = Green
2. A latching relay to control the light (60W MAX)
    a. On = A0
    b. Off = A1
3. A latching relay to control the heater (60W MAX)
    a. On = A2
    b. Off = A3

## Development

When installing on a new computer, add a `Secrets.h` file that looks like:

```cpp
#define MQTT_USER "test"
#define MQTT_PASS "test"
```