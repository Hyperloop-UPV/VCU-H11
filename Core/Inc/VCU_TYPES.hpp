#ifndef VCU_TYPES_HPP
#define VCU_TYPES_HPP

#include "Communications/Packets/DataPackets.hpp"
#include "Communications/RemoteBoards.hpp"
#include "Pinout/Pinout.hpp"
#include "ST-LIB.hpp"

#include <cstdint>

#ifndef STLIB_ETH
#error "VCU-H11 requires Ethernet. Build with an Ethernet preset and STLIB_ETH enabled."
#endif

namespace VCU {

#if defined(USE_PHY_LAN8742)
inline constexpr auto eth = ST_LIB::EthernetDomain::Ethernet(
    ST_LIB::EthernetDomain::PINSET_H10,
    "00:80:e1:00:01:03",
    "192.168.1.3",
    "255.255.255.0"
);
#elif defined(USE_PHY_LAN8700)
inline constexpr auto eth = ST_LIB::EthernetDomain::Ethernet(
    ST_LIB::EthernetDomain::PINSET_H10,
    "00:80:e1:00:01:03",
    "192.168.1.3",
    "255.255.255.0"
);
#elif defined(USE_PHY_KSZ8041)
inline constexpr auto eth = ST_LIB::EthernetDomain::Ethernet(
    ST_LIB::EthernetDomain::PINSET_H11,
    "00:80:e1:00:01:03",
    "192.168.1.3",
    "255.255.255.0"
);
#else
#error "No PHY selected for Ethernet pinset selection"
#endif

inline ST_LIB::EthernetDomain::Instance* ethernet = nullptr;

inline constexpr auto led_operational_req =
    ST_LIB::DigitalOutputDomain::DigitalOutput(Pinout::led_operational);

inline constexpr auto led_sleep_req = ST_LIB::DigitalOutputDomain::DigitalOutput(Pinout::led_sleep);
inline constexpr auto led_can_req = ST_LIB::DigitalOutputDomain::DigitalOutput(Pinout::led_can);
inline constexpr auto led_connecting_req =
    ST_LIB::DigitalOutputDomain::DigitalOutput(Pinout::led_connecting);
inline constexpr auto led_fault_req = ST_LIB::DigitalOutputDomain::DigitalOutput(Pinout::led_fault);

inline constexpr auto can_silent_req =
    ST_LIB::DigitalOutputDomain::DigitalOutput(Pinout::can_silent);

inline constexpr auto cooling_pump_1_req =
    ST_LIB::DigitalOutputDomain::DigitalOutput(Pinout::cooling_pump_1);
inline constexpr auto cooling_pump_2_req =
    ST_LIB::DigitalOutputDomain::DigitalOutput(Pinout::cooling_pump_2);
inline constexpr auto electrovalve_req =
    ST_LIB::DigitalOutputDomain::DigitalOutput(Pinout::electrovalve);
inline constexpr auto brake_reset_req =
    ST_LIB::DigitalOutputDomain::DigitalOutput(Pinout::brake_reset);

inline constexpr auto brake_fault_req =
    ST_LIB::DigitalInputDomain::DigitalInput(Pinout::brake_fault);

inline void on_sdc_closed_edge() {}

inline constexpr auto sdc_closed_req = ST_LIB::EXTIDomain::Device(
    Pinout::sdc_closed,
    ST_LIB::EXTIDomain::Trigger::BOTH_EDGES,
    on_sdc_closed_edge
);

inline constexpr auto pressure_adc_resolution = ST_LIB::ADCDomain::Resolution::BITS_12;
inline constexpr auto pressure_adc_sample_time = ST_LIB::ADCDomain::SampleTime::CYCLES_2_5;
inline constexpr auto pressure_adc_prescaler = ST_LIB::ADCDomain::ClockPrescaler::DIV4;

inline constexpr auto high_pressure_adc_req = ST_LIB::ADCDomain::ADC(
    Pinout::high_pressure,
    ST_LIB::ADCDomain::Peripheral::ADC_1,
    ST_LIB::ADCDomain::Channel::CH2,
    pressure_adc_resolution,
    pressure_adc_sample_time,
    pressure_adc_prescaler
);
inline constexpr auto low_pressure_adc_req = ST_LIB::ADCDomain::ADC(
    Pinout::low_pressure,
    ST_LIB::ADCDomain::Peripheral::ADC_1,
    ST_LIB::ADCDomain::Channel::CH6,
    pressure_adc_resolution,
    pressure_adc_sample_time,
    pressure_adc_prescaler
);
inline constexpr auto pressure_regulator_out_adc_req = ST_LIB::ADCDomain::ADC(
    Pinout::pressure_regulator_out,
    ST_LIB::ADCDomain::Peripheral::ADC_2,
    ST_LIB::ADCDomain::Channel::CH19,
    pressure_adc_resolution,
    pressure_adc_sample_time,
    pressure_adc_prescaler
);

inline constexpr auto ntc_adc_resolution = ST_LIB::ADCDomain::Resolution::BITS_12;
inline constexpr auto ntc_adc_sample_time = ST_LIB::ADCDomain::SampleTime::CYCLES_2_5;
inline constexpr auto ntc_adc_prescaler = ST_LIB::ADCDomain::ClockPrescaler::DIV4;

inline constexpr auto ntc_temperature_1_adc_req = ST_LIB::ADCDomain::ADC(
    Pinout::ntc_temperature_1,
    ST_LIB::ADCDomain::Peripheral::ADC_3,
    ST_LIB::ADCDomain::Channel::CH5,
    ntc_adc_resolution,
    ntc_adc_sample_time,
    ntc_adc_prescaler
);
inline constexpr auto ntc_temperature_2_adc_req = ST_LIB::ADCDomain::ADC(
    Pinout::ntc_temperature_2,
    ST_LIB::ADCDomain::Peripheral::ADC_3,
    ST_LIB::ADCDomain::Channel::CH9,
    ntc_adc_resolution,
    ntc_adc_sample_time,
    ntc_adc_prescaler
);

inline constexpr uint32_t remote_ack_timeout_us = 100'000;
inline constexpr uint32_t contactor_ack_timeout_us = 6'000'000;

inline DataPackets::state operational_state = DataPackets::state::Idle;
inline bool high_pressure_warning_active = false;

inline ST_LIB::DigitalOutputDomain::Instance* led_operational = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* led_sleep = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* led_can = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* led_connecting = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* led_fault = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* can_silent = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* cooling_pump_1 = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* cooling_pump_2 = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* electrovalve = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* brake_reset = nullptr;

inline ST_LIB::DigitalInputDomain::Instance* brake_fault = nullptr;
inline ST_LIB::EXTIDomain::Instance* sdc_closed_interrupt = nullptr;

inline constexpr float uncalibrated_linear_gain = 1.0f;
inline constexpr float LOW_PRESSURE_SLOPE = 3.5129857652111625;
inline constexpr float LOW_PRESSURE_OFFSET = -1.6994576891102104;
inline constexpr float HIGH_PRESSURE_SLOPE = 112.25464988418591;
inline constexpr float HIGH_PRESSURE_OFFSET = -38.599525119896384;
inline constexpr float uncalibrated_linear_offset = 0.0f;
inline constexpr float high_pressure_warning_threshold_bar = 50.0f;

inline float high_pressure = 0.0f;
inline float low_pressure = 0.0f;
inline float pressure_regulator_out = 0.0f;
inline float ntc_temperature_1 = 0.0f;
inline float ntc_temperature_2 = 0.0f;
inline GPIO_PinState sdc_closed_state = GPIO_PIN_RESET;
inline bool sdc_closed = false;
inline bool brake_fault_detected = false;
inline bool contactors_closed = false;
inline bool active_brakes = true;
inline bool recovery_requested = false;
inline bool electrovalve_enabled = false;
inline bool control_station_connected = false;
inline bool hvbms_connected = false;
inline bool pcu_connected = false;
inline bool lcu_connected = false;
inline bool required_peers_connected = false;
inline bool required_peers_were_connected = false;
inline bool control_station_was_connected = false;
inline bool hvbms_was_connected = false;
inline bool pcu_was_connected = false;
inline bool lcu_was_connected = false;

inline LinearSensor<float> high_pressure_sensor;
inline LinearSensor<float> low_pressure_sensor;
inline LinearSensor<float> pressure_regulator_out_sensor;
inline SensorInterrupt sdc_closed_sensor;
inline NTC ntc_temperature_1_sensor;
inline NTC ntc_temperature_2_sensor;

inline constexpr auto sdc_closed_protection = Protections::protection<"SDC", sdc_closed>(
    Protections::Rules::equals(false)
);

inline constexpr auto brake_fault_protection =
    Protections::protection<"brake_fault", brake_fault_detected>(
        Protections::Rules::equals(true)
    );

inline void engage_brake() {
    active_brakes = true;
    if (brake_reset != nullptr) {
        brake_reset->turn_off();
    }
}

inline void release_brake() {
    active_brakes = false;
    if (brake_reset != nullptr) {
        brake_reset->turn_on();
    }
}

inline void on_fault_enter() {
    operational_state = DataPackets::state::Fault;
    if (led_operational != nullptr) {
        led_operational->turn_off();
    }
    if (led_connecting != nullptr) {
        led_connecting->turn_off();
    }
    if (led_fault != nullptr) {
        led_fault->turn_on();
    }
    if (cooling_pump_1 != nullptr) {
        cooling_pump_1->turn_off();
    }
    if (cooling_pump_2 != nullptr) {
        cooling_pump_2->turn_off();
    }
    if (electrovalve != nullptr) {
        electrovalve->turn_off();
    }
    electrovalve_enabled = false;
    contactors_closed = false;
    engage_brake();
}

} // namespace VCU

#endif // VCU_TYPES_HPP
