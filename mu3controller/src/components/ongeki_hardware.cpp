#include "stdinclude.hpp"
#include <EEPROM.h>
#include <FastLED.h>

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

        void start()
        {
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
            if(read != data->lever){
                data->lever = read;
                updated = true;
            }

            if (data->buttons[4] && data->buttons[9])
            {
                EEPROM.get(0, data->aimi_id);
                data->scan = true;
            }
            else
            {
                memset(&data->aimi_id, 0, 10);
                data->scan = false;
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
