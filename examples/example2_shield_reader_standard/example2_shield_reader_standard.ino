/* Arduino libraries */
#include <Arduino.h>
#include <tic_reader.h>
#include <wk2132.h>

/* Peripherals */
static wk2132 m_wk2132;
static tic_reader m_tic_reader;

/**
 *
 */
void setup(void) {

    /* Setup UART used for debug */
    Serial.begin(115200);
    Serial.println(" [i] Hello world");

    /* Setup I2C */
    Wire.begin();
    Wire.setClock(400000);

    /* Setup UART exapander
     * The first parameter is the frequency (in Hz) of the crystal attached to the WK2132 IC
     * The second parameter is a reference to the default I2C library used to talk with the WK2132 IC
     * The other paramters define the I2C address */
    m_wk2132.setup(11059200, Wire, 0, 0);

    /* Setup UART port that listens to data coming from the electricity meter
     * The parameter is the baudrate: 1200 for historic (most meters) or 9600 for standard */
    m_wk2132.uarts[0].begin(9600);

    /* Setup tic reader
     * The parameter is a reference the first UART of the WK2132 IC */
    m_tic_reader.setup(m_wk2132.uarts[0]);
}

/**
 *
 */
void loop(void) {

    /* Listen for incoming TIC messages */
    struct tic_dataset dataset;
    int res = m_tic_reader.read(dataset);
    if (res < 0) {
        Serial.println(" [e] Failed to process incoming data! Maybe change baudrate?");
    } else if (res > 0) {

        /* Log received message */
        if (strlen(dataset.time) > 0) {
            Serial.print(" [i] Received ");
            Serial.print(dataset.name);
            Serial.print(" = ");
            Serial.print(dataset.data);
            Serial.print(" (at ");
            Serial.print(dataset.time);
            Serial.println(")");
        } else {
            Serial.print(" [i] Received ");
            Serial.print(dataset.name);
            Serial.print(" = ");
            Serial.print(dataset.data);
            Serial.println();
        }
    }
}
