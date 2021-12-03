#include "stdinclude.hpp"

namespace component
{
    namespace manager
    {
        void start()
        {
            raw_hid::start();
            ongeki_hardware::start();
            jvsio::start();
        }

        void update()
        {
            raw_hid::update();
            jvsio::update();
        }
    }
}
