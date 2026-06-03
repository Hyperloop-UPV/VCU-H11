#ifndef VCU_HPP
#define VCU_HPP

#ifdef STLIB_ETH
#include "Communications/Packets/DataPackets.hpp"
#include "Communications/Packets/OrderPackets.hpp"
#endif
#include "ST-LIB.hpp"
#include "StateMachine/VCU_StateMachine.hpp"
#include "VCU_TYPES.hpp"

namespace VCU {

using Board = ST_LIB::Board<
    ST_LIB::FaultPolicyNoMachine<on_fault_enter>,
#ifdef STLIB_ETH
    eth,
#endif
    led_operational_req,
    led_sleep_req,
    led_can_req,
    led_connecting_req,
    led_fault_req,
    can_silent_req,
    cooling_pump_1_req,
    cooling_pump_2_req,
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

#ifdef STLIB_ETH
    ethernet = &Board::instance_of<eth>();
#endif

    led_operational = &Board::instance_of<led_operational_req>();
    led_sleep = &Board::instance_of<led_sleep_req>();
    led_can = &Board::instance_of<led_can_req>();
    led_connecting = &Board::instance_of<led_connecting_req>();
    led_fault = &Board::instance_of<led_fault_req>();
    can_silent = &Board::instance_of<can_silent_req>();
    cooling_pump_1 = &Board::instance_of<cooling_pump_1_req>();
    cooling_pump_2 = &Board::instance_of<cooling_pump_2_req>();
    electrovalve = &Board::instance_of<electrovalve_req>();
    brake_reset = &Board::instance_of<brake_reset_req>();
    brake_fault = &Board::instance_of<brake_fault_req>();
    sdmmc_card_detect = &Board::instance_of<sdmmc_card_detect_req>();
    sdmmc_write_protect = &Board::instance_of<sdmmc_write_protect_req>();
    sdc_closed_interrupt = &Board::instance_of<sdc_closed_req>();

    high_pressure_sensor = LinearSensor<float>(
        Board::instance_of<high_pressure_adc_req>(),
        HIGH_PRESSURE_SLOPE,
        HIGH_PRESSURE_OFFSET,
        high_pressure
    );
    low_pressure_sensor = LinearSensor<float>(
        Board::instance_of<low_pressure_adc_req>(),
        LOW_PRESSURE_SLOPE,
        LOW_PRESSURE_OFFSET,
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
    configure_flow_input_captures();

    sdc_closed_interrupt->turn_on();
    led_operational->turn_off();
    led_sleep->turn_off();
    led_can->turn_off();
    led_connecting->turn_off();
    led_fault->turn_off();

#ifdef STLIB_ETH
    DataPackets::Outputs_init(
        cooling_pump_1_command,
        cooling_pump_2_command,
        electrovalve_enabled
    );
    OrderPackets::Turn_on_electrovalve_init();
    OrderPackets::Turn_off_electrovalve_init();

    OrderPackets::start();
    DataPackets::start();
#endif

    VCU_StateMachine::start();
}

inline void update() {
#ifdef STLIB_ETH
    ethernet->update();
#endif
    VCU_StateMachine::update();
    FaultController::check_transitions();
    Diagnostics::Hub::flush();
    Scheduler::update();
}

} // namespace VCU

#endif // VCU_HPP
