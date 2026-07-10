#include "ST-LIB.hpp"
#include "VCU.hpp"
#include "main.h"

using namespace ST_LIB;

int main(void) {
    VCU::init();
    // HAL_Delay(15 * 1000);

    while (1) {
        VCU::update();
    }
}

extern "C" void Error_Handler(void) {
    PANIC("HAL error handler triggered");
    while (1) {
    }
}
