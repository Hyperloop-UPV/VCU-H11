#ifndef VCU_TYPES_HPP
#define VCU_TYPES_HPP

#include "Pinout/Pinout.hpp"
#include "ST-LIB.hpp"

#include <optional>

namespace VCU {

#ifdef STLIB_ETH
#if defined(USE_PHY_LAN8742)
inline constexpr auto eth = ST_LIB::EthernetDomain::Ethernet(
    ST_LIB::EthernetDomain::PINSET_H10,
    "00:80:e1:00:01:03",
    "192.168.1.3",
    "255.255.0.0"
);
#elif defined(USE_PHY_LAN8700)
inline constexpr auto eth = ST_LIB::EthernetDomain::Ethernet(
    ST_LIB::EthernetDomain::PINSET_H11,
    "00:80:e1:00:01:03",
    "192.168.1.3",
    "255.255.255.0"
);
#elif defined(USE_PHY_KSZ8041)
inline constexpr auto eth = ST_LIB::EthernetDomain::Ethernet(
    ST_LIB::EthernetDomain::PINSET_H11,
    "00:80:e1:00:01:03",
    "192.168.1.3",
    "255.255.0.0"
);
#else
#error "No PHY selected for Ethernet pinset selection"
#endif
#endif

inline constexpr auto led_status_req =
    ST_LIB::DigitalOutputDomain::DigitalOutput(Pinout::led_status);

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
inline constexpr auto sdmmc_card_detect_req =
    ST_LIB::DigitalInputDomain::DigitalInput(Pinout::sdmmc_card_detect);
inline constexpr auto sdmmc_write_protect_req =
    ST_LIB::DigitalInputDomain::DigitalInput(Pinout::sdmmc_write_protect);

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

inline constexpr ST_LIB::TimerPin pressure_regulator_in_pwm_pin{
    .af = ST_LIB::TimerAF::PWM,
    .pin = Pinout::pressure_regulator_in,
    .channel = ST_LIB::TimerChannel::CHANNEL_1,
};
inline constexpr auto pressure_regulator_timer_req = ST_LIB::TimerDomain::Timer{
    ST_LIB::TimerRequest::SlaveTimer_13,
    ST_LIB::TimerDomain::EMPTY_TIMER_NAME,
    pressure_regulator_in_pwm_pin,
};

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

inline constexpr ST_LIB::TimerPin flow_1_timer_pin{
    .af = ST_LIB::TimerAF::InputCapture,
    .pin = Pinout::flow_1,
    .channel = ST_LIB::TimerChannel::CHANNEL_1,
};
inline constexpr ST_LIB::TimerPin flow_2_timer_pin{
    .af = ST_LIB::TimerAF::InputCapture,
    .pin = Pinout::flow_2,
    .channel = ST_LIB::TimerChannel::CHANNEL_2,
};
inline constexpr auto flow_timer_req = ST_LIB::TimerDomain::Timer{
    ST_LIB::TimerRequest::GeneralPurpose32bit_23,
    ST_LIB::TimerDomain::EMPTY_TIMER_NAME,
    flow_1_timer_pin,
    flow_2_timer_pin,
};

inline ST_LIB::DigitalOutputDomain::Instance* led_status = nullptr;
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
inline ST_LIB::DigitalInputDomain::Instance* sdmmc_card_detect = nullptr;
inline ST_LIB::DigitalInputDomain::Instance* sdmmc_write_protect = nullptr;
inline ST_LIB::EXTIDomain::Instance* sdc_closed_interrupt = nullptr;

inline ST_LIB::TimerWrapper<flow_timer_req> flow_timer;
inline ST_LIB::TimerWrapper<pressure_regulator_timer_req> pressure_regulator_timer;
using PressureRegulatorPwm = ST_LIB::PWM<pressure_regulator_timer_req, pressure_regulator_in_pwm_pin>;
inline std::optional<PressureRegulatorPwm> pressure_regulator_pwm;

inline constexpr float uncalibrated_linear_gain = 1.0f;
inline constexpr float uncalibrated_linear_offset = 0.0f;

inline float high_pressure = 0.0f;
inline float low_pressure = 0.0f;
inline float pressure_regulator_out = 0.0f;
inline float ntc_temperature_1 = 0.0f;
inline float ntc_temperature_2 = 0.0f;
inline GPIO_PinState sdc_closed_state = GPIO_PIN_RESET;

inline LinearSensor<float> high_pressure_sensor;
inline LinearSensor<float> low_pressure_sensor;
inline LinearSensor<float> pressure_regulator_out_sensor;
inline NTC ntc_temperature_1_sensor;
inline NTC ntc_temperature_2_sensor;
inline SensorInterrupt sdc_closed_sensor;

enum class OperationalState : uint8_t {
    Idle,
    EndOfRun,
    Energized,
    Ready,
    Demonstration,
    Recovery,
};

inline OperationalState operational_state = OperationalState::Idle;

inline void on_fault_enter() {
    if (led_status != nullptr) {
        led_status->turn_on();
    }
}

} // namespace VCU

#endif // VCU_TYPES_HPP
