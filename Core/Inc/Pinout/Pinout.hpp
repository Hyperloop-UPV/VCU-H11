#ifndef PINOUT_HPP
#define PINOUT_HPP

#include "HALAL/Models/Pin.hpp"
#include "HALAL/Models/PinModel/Pin.hpp"

namespace Pinout {

// ============================================
// Info LEDs
// ============================================

inline auto& led_sleep = ST_LIB::PG9;
inline auto& led_can = ST_LIB::PG10;
inline auto& led_connecting = ST_LIB::PG11;
inline auto& led_fault = ST_LIB::PG12;
inline auto& led_operational = ST_LIB::PG13;
// inline auto& led_connecting = ST_LIB::PB0;
// inline auto& led_fault = ST_LIB::PB14;
// inline auto& led_operational = ST_LIB::PE1;

// ============================================
// CAN
// ============================================

inline auto& can_txd = ST_LIB::PA12;
inline auto& can_rxd = ST_LIB::PA11;
inline auto& can_silent = ST_LIB::PA8;

// ============================================
// SDMMC
// ============================================

inline auto& sdmmc_cmd = ST_LIB::PD2;
inline auto& sdmmc_clk = ST_LIB::PC12;
inline auto& sdmmc_d0 = ST_LIB::PC8;
inline auto& sdmmc_d1 = ST_LIB::PC9;
inline auto& sdmmc_d2 = ST_LIB::PC10;
inline auto& sdmmc_d3 = ST_LIB::PC11;
inline auto& sdmmc_card_detect = ST_LIB::PG4;
inline auto& sdmmc_write_protect = ST_LIB::PG3;

// ============================================
// Sensors
// ============================================

inline auto& flow_1 = ST_LIB::PF0;
inline auto& flow_2 = ST_LIB::PF1;
inline auto& ntc_temperature_1 = ST_LIB::PF3;
inline auto& ntc_temperature_2 = ST_LIB::PF4;
inline auto& high_pressure = ST_LIB::PF11;
inline auto& low_pressure = ST_LIB::PF12;
inline auto& pressure_regulator_out = ST_LIB::PA5;
inline auto& sdc_closed = ST_LIB::PF6;

// ============================================
// Actuators
// ============================================

inline auto& cooling_pump_1 = ST_LIB::PE13;
inline auto& cooling_pump_2 = ST_LIB::PE14;
inline auto& electrovalve = ST_LIB::PE15;
inline auto& brake_reset = ST_LIB::PD15;
inline auto& brake_fault = ST_LIB::PD14;

} // namespace Pinout

#endif // PINOUT_HPP
