/* Arduino libraries */
#include <Arduino.h>
#include <LiquidCrystal.h>
#include <tic_reader.h>
#include <wk2132.h>

/* Peripherals */
static wk2132 m_wk2132;
static tic_reader m_tic_reader;
static LiquidCrystal m_lcd(8, 9, 4, 5, 6, 7);

/**
 *
 */
void setup(void) {

    /* Setup UART used for debug */
    Serial.begin(115200);
    Serial.println(" [i] Hello world");

    /* Setup LCD display
     * The one used for this example can be bought here https://fr.aliexpress.com/item/32842943705.html */
    m_lcd.begin(16, 2);
    m_lcd.clear();
    m_lcd.print("Hello world");

    /* Setup I2C */
    Wire.begin();
    Wire.setClock(400000);

    /* Setup UART exapander
     * The first parameter is the frequency (in Hz) of the crystal attached to the WK2132 IC
     * The second parameter is a reference to the default I2C library used to talk with the WK2132 IC
     * The other paramters define the I2C address */
    m_wk2132.setup(11059200, Wire, 0, 0);

    /* Setup tic reader
     * The parameter is a reference the first UART of the WK2132 IC */
    m_tic_reader.setup(m_wk2132.uarts[0]);
}

/**
 *
 */
void loop(void) {

    /* Main state machine which tries reading data in the two possible modes
     * It first configures the baudrate for a mode, then tries to read data in this mode
     * If there are any reception errors, it switches to the other mode */
    static enum {
        STATE_HISTORIC_0,
        STATE_HISTORIC_1,
        STATE_STANDARD_0,
        STATE_STANDARD_1,
    } m_sm;
    switch (m_sm) {

        case STATE_HISTORIC_0: {

            /* Setup UART port that listens to data coming from the electricity meter
             * The parameter is the baudrate: 1200 for historic (most meters) or 9600 for standard */
            m_wk2132.uarts[0].end();
            m_wk2132.uarts[0].begin(1200);

            /* Move on */
            m_sm = STATE_HISTORIC_1;
            break;
        }

        case STATE_HISTORIC_1: {

            /* Listen for incoming TIC messages */
            struct tic_dataset dataset;
            int res = m_tic_reader.read(dataset);
            if (res < 0) {
                Serial.println(" [e] Failed to process incoming data! Trying standard...");
                m_sm = STATE_STANDARD_0;
            } else if (res >= 1) {

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

                /* Handle "Puissance apparente" */
                if (strcmp_P(dataset.name, PSTR("PAPP")) == 0) {

                    /* Convert string into uint32 */
                    uint32_t power = strtoul(dataset.data, NULL, 10);

                    /* Display */
                    m_lcd.clear();
                    m_lcd.setCursor(0, 0);
                    m_lcd.print("PAPP =");
                    m_lcd.setCursor(0, 1);
                    m_lcd.print(power);
                    m_lcd.print(" VA");
                }
            }

            /* Stay in this state unless there is an error when receiving the data */
            break;
        }

        case STATE_STANDARD_0: {

            /* Setup UART port that listens to data coming from the electricity meter
             * The parameter is the baudrate: 1200 for historic (most meters) or 9600 for standard */
            m_wk2132.uarts[0].end();
            m_wk2132.uarts[0].begin(9600);

            /* Move on */
            m_sm = STATE_STANDARD_1;
            break;
        }

        case STATE_STANDARD_1: {

            /* Listen for incoming TIC messages */
            struct tic_dataset dataset;
            int res = m_tic_reader.read(dataset);
            if (res < 0) {
                Serial.println(" [e] Failed to process incoming data! Trying historic...");
                m_sm = STATE_HISTORIC_0;
            } else if (res >= 1) {

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

                /* Handle "Puissance app. Instantanée soutirée" */
                if (strcmp_P(dataset.name, PSTR("SINSTS")) == 0) {

                    /* Convert string into int32 */
                    int32_t power = strtol(dataset.data, NULL, 10);

                    /* Display */
                    m_lcd.clear();
                    m_lcd.setCursor(0, 0);
                    m_lcd.print("SINSTS =");
                    m_lcd.setCursor(0, 1);
                    m_lcd.print(power);
                    m_lcd.print(" VA");
                }
            }

            /* Stay in this state unless there is an error when receiving the data */
            break;
        }
    }
}
