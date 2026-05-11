#ifndef VCU_HPP
#define VCU_HPP

#include "ST-LIB.hpp"
#include "StateMachine/VCU_StateMachine.hpp"
#include "VCU_TYPES.hpp"

namespace VCU {

using Board = ST_LIB::Board<
    ST_LIB::FaultPolicyNoMachine<on_fault_enter>,
#ifdef STLIB_ETH
    eth,
#endif
    led_status_req,
    led_sleep_req,
    led_flash_req,
    led_can_req,
    led_fault_req,
    can_silent_req,
    cooling_pump_req,
    electrovalve_req,
    brake_reset_req,
    brake_fault_req,
    sdmmc_card_detect_req,
    sdmmc_write_protect_req,
    sdc_closed_req,
    high_pressure_adc_req,
    low_pressure_adc_req,
    pressure_regulator_out_adc_req,
    ntc_temperature_1_adc_req,
    ntc_temperature_2_adc_req,
    flow_timer_req>;

inline void init() {
    Board::init();

    led_status = &Board::instance_of<led_status_req>();
    led_sleep = &Board::instance_of<led_sleep_req>();
    led_flash = &Board::instance_of<led_flash_req>();
    led_can = &Board::instance_of<led_can_req>();
    led_fault = &Board::instance_of<led_fault_req>();
    can_silent = &Board::instance_of<can_silent_req>();
    cooling_pump = &Board::instance_of<cooling_pump_req>();
    electrovalve = &Board::instance_of<electrovalve_req>();
    brake_reset = &Board::instance_of<brake_reset_req>();
    brake_fault = &Board::instance_of<brake_fault_req>();
    sdmmc_card_detect = &Board::instance_of<sdmmc_card_detect_req>();
    sdmmc_write_protect = &Board::instance_of<sdmmc_write_protect_req>();
    sdc_closed_interrupt = &Board::instance_of<sdc_closed_req>();

    high_pressure_sensor = LinearSensor<float>(
        Board::instance_of<high_pressure_adc_req>(),
        uncalibrated_linear_gain,
        uncalibrated_linear_offset,
        high_pressure
    );
    low_pressure_sensor = LinearSensor<float>(
        Board::instance_of<low_pressure_adc_req>(),
        uncalibrated_linear_gain,
        uncalibrated_linear_offset,
        low_pressure
    );
    pressure_regulator_out_sensor = LinearSensor<float>(
        Board::instance_of<pressure_regulator_out_adc_req>(),
        uncalibrated_linear_gain,
        uncalibrated_linear_offset,
        pressure_regulator_out
    );
    ntc_temperature_1_sensor =
        NTC(Board::instance_of<ntc_temperature_1_adc_req>(), ntc_temperature_1);
    ntc_temperature_2_sensor =
        NTC(Board::instance_of<ntc_temperature_2_adc_req>(), ntc_temperature_2);
    sdc_closed_sensor = SensorInterrupt(*sdc_closed_interrupt, sdc_closed_state);
    flow_timer = ST_LIB::TimerWrapper<flow_timer_req>(&Board::instance_of<flow_timer_req>());

    sdc_closed_interrupt->turn_on();
    led_status->turn_on();

    VCU_StateMachine::start();
}

inline void update() {
    VCU_StateMachine::update();
    FaultController::check_transitions();
    Board::evaluate_protections();
    Diagnostics::Hub::flush();
    Scheduler::update();
}

} // namespace VCU

#endif // VCU_HPP
