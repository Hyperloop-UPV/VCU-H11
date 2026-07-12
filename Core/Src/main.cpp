#include "ST-LIB.hpp"
#include "VCU.hpp"
#include "main.h"

using namespace ST_LIB;

int main(void) {
    for (volatile uint32_t i = 0; i < 25000000;) {
        auto j = i;
        i = j + 1;
        __NOP();
    }
    VCU::init();

    while (1) {
        VCU::update();
    }
}

extern "C" void Error_Handler(void) {
    PANIC("HAL error handler triggered");
    while (1) {
    }
}
