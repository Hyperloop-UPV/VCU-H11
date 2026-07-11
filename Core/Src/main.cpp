#include "ST-LIB.hpp"
#include "VCU.hpp"
#include "main.h"

using namespace ST_LIB;

int main(void) {
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
