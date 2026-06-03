#ifndef VCU_TYPES_HPP
#define VCU_TYPES_HPP

#include "Pinout/Pinout.hpp"
#include "ST-LIB.hpp"

#include <cstdint>

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

#ifdef STLIB_ETH
inline ST_LIB::EthernetDomain::Instance* ethernet = nullptr;
#endif

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

inline constexpr uint32_t remote_ack_timeout_us = 500'000;
inline constexpr uint32_t contactor_ack_timeout_us = 6'000'000;

enum class GeneralState : uint8_t {
    Connecting = 0,
    Operational = 1,
    Fault = 2,
};

enum class OperationalState : uint8_t {
    Idle = 0,
    EndOfRun = 1,
    Energized = 2,
    Ready = 3,
    Demonstration = 4,
    Recovery = 5,
};

enum class PumpSelection : uint8_t {
    CoolingPump1 = 0,
    CoolingPump2 = 1,
};

enum class HVSCUState : uint8_t {
    Opened = 0,
    Closed = 2,
};

enum class PCUState : uint8_t {
    Stopped = 0,
    Propulsion = 2,
};

enum class LCUState : uint8_t {
    Stopped = 0,
    Levitation = 3,
};

enum class BoosterState : uint8_t {
    Disabled = 0,
    Enabled = 1,
};

inline constexpr uint8_t levitation_bit = 0;
inline constexpr uint8_t propulsion_bit = 1;
inline constexpr uint8_t booster_bit = 5;

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
inline ST_LIB::DigitalInputDomain::Instance* sdmmc_card_detect = nullptr;
inline ST_LIB::DigitalInputDomain::Instance* sdmmc_write_protect = nullptr;
inline ST_LIB::EXTIDomain::Instance* sdc_closed_interrupt = nullptr;

inline ST_LIB::TimerWrapper<flow_timer_req> flow_timer;

inline constexpr float uncalibrated_linear_gain = 1.0f;
inline constexpr float LOW_PRESSURE_SLOPE = 3.5129857652111625;
inline constexpr float LOW_PRESSURE_OFFSET = -1.6994576891102104;
inline constexpr float HIGH_PRESSURE_SLOPE = 112.25464988418591;
inline constexpr float HIGH_PRESSURE_OFFSET = -38.599525119896384;
inline constexpr float uncalibrated_linear_offset = 0.0f;

inline uint8_t general_state = static_cast<uint8_t>(GeneralState::Connecting);
inline uint8_t operational_state_id = static_cast<uint8_t>(OperationalState::Idle);
inline uint8_t recovery_status = 0;
inline uint32_t demonstration_bitfield = 0;

inline float high_pressure = 0.0f;
inline float low_pressure = 0.0f;
inline float pressure_regulator_out = 0.0f;
inline float flow_1 = 0.0f;
inline float flow_2 = 0.0f;
inline float ntc_temperature_1 = 0.0f;
inline float ntc_temperature_2 = 0.0f;
inline GPIO_PinState sdc_closed_state = GPIO_PIN_RESET;
inline bool sdc_closed = false;
inline bool brake_fault_detected = false;
inline bool sdmmc_card_detected = false;
inline bool sdmmc_write_protected = false;
inline bool contactors_closed = false;
inline bool active_brakes = true;
inline bool recovery_requested = false;
inline bool electrovalve_enabled = false;
inline bool control_station_connected = false;
inline bool hvscu_connected = false;
inline bool pcu_connected = false;
inline bool required_peers_connected = false;
inline bool required_peers_were_connected = false;

inline uint8_t cooling_pump_duty = 0;
inline uint8_t cooling_pump_selection = static_cast<uint8_t>(PumpSelection::CoolingPump1);
inline uint8_t cooling_pump_1_command = 0;
inline uint8_t cooling_pump_2_command = 0;

inline uint8_t hvscu_state = static_cast<uint8_t>(HVSCUState::Opened);
inline uint8_t pcu_state = static_cast<uint8_t>(PCUState::Stopped);
inline uint8_t lcu_vertical_state = static_cast<uint8_t>(LCUState::Stopped);
inline uint8_t lcu_horizontal_state = static_cast<uint8_t>(BoosterState::Disabled);

inline uint8_t run_id = 0;
inline float modulation_frequency_1 = 0.0f;
inline float commutation_frequency_1 = 0.0f;
inline float reference_voltage_1 = 0.0f;
inline float max_voltage_1 = 0.0f;
inline uint8_t motor_direction_1 = 0;
inline float modulation_frequency_2 = 0.0f;
inline float commutation_frequency_2 = 0.0f;
inline float reference_current_2 = 0.0f;
inline float max_voltage_2 = 0.0f;
inline uint8_t motor_direction_2 = 0;
inline float reference_speed_3 = 0.0f;
inline float commutation_frequency_3 = 0.0f;
inline float max_voltage_3 = 0.0f;
inline uint8_t motor_direction_3 = 0;
inline float levitation_distance = 0.0f;

inline LinearSensor<float> high_pressure_sensor;
inline LinearSensor<float> low_pressure_sensor;
inline LinearSensor<float> pressure_regulator_out_sensor;
inline NTC ntc_temperature_1_sensor;
inline NTC ntc_temperature_2_sensor;
inline SensorInterrupt sdc_closed_sensor;

inline OperationalState operational_state = OperationalState::Idle;
inline bool flow_capture_initialized = false;

inline void flow_timer_update_noop(void*) {}

inline void sync_state_telemetry() {
    if (FaultController::is_faulted()) {
        general_state = static_cast<uint8_t>(GeneralState::Fault);
    } else if (required_peers_connected) {
        general_state = static_cast<uint8_t>(GeneralState::Operational);
    } else {
        general_state = static_cast<uint8_t>(GeneralState::Connecting);
    }

    operational_state_id = static_cast<uint8_t>(operational_state);
}

inline void set_cooling_pump(PumpSelection selection, uint8_t duty) {
    const bool enabled = duty >= 100;

    if (selection == PumpSelection::CoolingPump1) {
        cooling_pump_1_command = enabled ? 100 : 0;
        if (cooling_pump_1 != nullptr) {
            enabled ? cooling_pump_1->turn_on() : cooling_pump_1->turn_off();
        }
        return;
    }

    cooling_pump_2_command = enabled ? 100 : 0;
    if (cooling_pump_2 != nullptr) {
        enabled ? cooling_pump_2->turn_on() : cooling_pump_2->turn_off();
    }
}

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

inline void set_electrovalve(bool enabled) {
    electrovalve_enabled = enabled;
    if (electrovalve == nullptr) {
        return;
    }

    enabled ? electrovalve->turn_on() : electrovalve->turn_off();
}

template <ST_LIB::TimerChannel Channel> inline void configure_flow_capture_channel() {
    constexpr uint8_t channel_index = static_cast<uint8_t>(Channel) - 1;
    auto* info = &ST_LIB::TimerDomain::input_capture_info_backing
        [flow_timer.instance->timer_idx][channel_index];

    ST_LIB::TimerDomain::input_capture_info[flow_timer.instance->timer_idx][channel_index] = info;
    info->channel_rising = channel_index;
    info->channel_falling = 0xFF;
    info->value_rising = 0;
    info->value_falling = 0;
    info->period = 0;
    info->duty_cycle = 0.0f;
    info->frequency = 0;

    TIM_IC_InitTypeDef input_capture_config = {
        .ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING,
        .ICSelection = TIM_ICSELECTION_DIRECTTI,
        .ICPrescaler = TIM_ICPSC_DIV1,
        .ICFilter = 0,
    };

    flow_timer.template config_input_compare_channel<Channel>(&input_capture_config);
    flow_timer.template enable_capture_compare_interrupt<Channel>();
    SET_BIT(
        flow_timer.instance->tim->CCER,
        static_cast<uint32_t>(
            TIM_CCER_CC1E << (ST_LIB::TimerDomain::get_channel_mul4(Channel) & 0x1FU)
        )
    );
}

inline void configure_flow_input_captures() {
    if (flow_capture_initialized || flow_timer.instance == nullptr) {
        return;
    }

    flow_timer.set_prescaler(2'000);
    flow_timer.set_limit_value(0xFFFF'FFFFu);
    flow_timer.set_callback(flow_timer_update_noop, nullptr);
    flow_timer.enable_nvic();
    configure_flow_capture_channel<ST_LIB::TimerChannel::CHANNEL_1>();
    configure_flow_capture_channel<ST_LIB::TimerChannel::CHANNEL_2>();
    flow_timer.counter_enable();
    flow_capture_initialized = true;
}

template <ST_LIB::TimerChannel Channel> inline uint32_t read_flow_capture_frequency() {
    constexpr uint8_t channel_index = static_cast<uint8_t>(Channel) - 1;
    if (!flow_capture_initialized || flow_timer.instance == nullptr) {
        return 0;
    }
    return ST_LIB::TimerDomain::input_capture_info_backing[flow_timer.instance->timer_idx]
                                                     [channel_index]
                                                         .frequency;
}

inline void on_fault_enter() {
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
    cooling_pump_1_command = 0;
    cooling_pump_2_command = 0;
    engage_brake();
    sync_state_telemetry();
}

} // namespace VCU

#endif // VCU_TYPES_HPP
