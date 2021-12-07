#include "stdinclude.hpp"
#include <EEPROM.h>
#include <FastLED.h>
#include <Wire.h>
#include <PN532.h>
#include <PN532_I2C.h>

namespace component
{
    namespace ongeki_hardware
    {
        const int LEVER = PIN_A0;
        const int LED_PIN = PIN_A1;
        const uint8_t PIN_MAP[10] = {
            // L: A B C SIDE MENU
            1, 0, 4, 14, 8,
            // R: A B C SIDE MENU
            5, 6, 7, 15, 9};

        CRGB lightColors[6];

        typedef union
        {
            uint8_t block[18];
            struct
            {
                uint8_t IDm[8];
                uint8_t PMm[8];
                uint16_t SystemCode;
            };
        } Felica;
        Felica felica;
        PN532_I2C pn532i2c(Wire);
        PN532 nfc(pn532i2c);
        uint8_t cardtype, uid[4], uL;
        uint16_t systemCode = 0xFFFF;
        uint8_t requestCode = 0x01;

        uint8_t AimeKey[6], BanaKey[6];

        void start()
        {
            Wire.begin();
            nfc.begin();
            if (!nfc.getFirmwareVersion()) {
    while (1) {
      Serial.println("wait");
      delay(1000);
    }
            }
  
            nfc.setPassiveActivationRetries(0x10); //设定等待次数
            nfc.SAMConfig();
            
            // setup pin modes for button
            for (unsigned char i : PIN_MAP)
            {
                pinMode(i, INPUT_PULLUP);
            }

            // setup led_t
            FastLED.addLeds<WS2812B, LED_PIN, GRB>(lightColors, 6);
        }

        bool read_io(raw_hid::output_data_t *data)
        {
            bool updated = false;
            for (auto i = 0; i < 10; i++)
            {
                auto read = digitalRead(PIN_MAP[i]) ==
                            ((i == 3 || i == 8) ? HIGH : LOW);
                if (read != data->buttons[i])
                {
                    data->buttons[i] = read;
                    updated = true;
                }
            }

            auto read = analogRead(LEVER);
            if (read != data->lever)
            {
                data->lever = read;
                updated = true;
            }

            if (nfc.felica_Polling(systemCode, requestCode, felica.IDm, felica.PMm, &felica.SystemCode, 2))
            {
                
            Serial.println("success");
                //memcpy((uint8_t *)data->felica_id, 0, 10);
                data->felica_id = *((uint64_t *)(felica.IDm));
                data->scan = true;
                updated = true;
                //cardtype = 0x20;
                //felica.SystemCode = felica.SystemCode >> 8 | felica.SystemCode << 8;

                //return true;
            }
            else
            {
                data->scan = false;
                updated = true;
                //cardtype = 0;
            }
            return updated;
        }

        void set_led(raw_hid::led_t &data)
        {
            FastLED.setBrightness(data.ledBrightness);

            //for(int i = 0; i < 3; i++) {
            //    memcpy(&lightColors[2-i], &data.ledColors[i], 3);
            //memcpy(&lightColors[i + 3], &data.ledColors[i + 5], 3);
            //}
            for (int i = 0; i < 3; i++)
            {
                // game 0 -> 2

                memcpy(&lightColors[5 - i], &data.ledColors[i], 3);
                memcpy(&lightColors[2 - i], &data.ledColors[i + 5], 3);
            }
            /*for(int i=5;i<8;i++){
                memcpy(&lightColors[10-i], &data.ledColors[i], 3);
            }*/

            FastLED.show();
        }
    }
}
