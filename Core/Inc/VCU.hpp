#ifndef VCU_HPP
#define VCU_HPP

#include "Communications/Packets/DataPackets.hpp"
#include "Communications/Packets/OrderPackets.hpp"
#include "ST-LIB.hpp"
#include "StateMachine/VCU_StateMachine.hpp"
#include "VCU_TYPES.hpp"

namespace VCU {

using Board = ST_LIB::Board<
    ST_LIB::FaultPolicyNoMachine<on_fault_enter>,
    eth,
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
    flow_timer_req,
    sdc_closed_protection,
    brake_fault_protection,
    tapes_reached_protection>;

inline void init() {
    Board::init();

    ethernet = &Board::instance_of<eth>();

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

    DataPackets::VCU_State_init(
        general_state,
        operational_state_id,
        recovery_status,
        demonstration_bitfield
    );
    DataPackets::Flow_init(flow_1, flow_2);
    DataPackets::Temperatures_init(ntc_temperature_1, ntc_temperature_2);
    DataPackets::Pressures_init(high_pressure, low_pressure, pressure_regulator_out);
    DataPackets::Brake_Status_init(active_brakes, brake_fault_detected);
    DataPackets::Outputs_init(
        cooling_pump_1_command,
        cooling_pump_2_command,
        electrovalve_enabled
    );
    DataPackets::Safety_init(
        sdc_closed,
        tapes_reached,
        contactors_closed,
        control_station_connected,
        hvbms_connected,
        pcu_connected,
        lcu_connected,
        required_peers_connected
    );
    DataPackets::HVBMS_State_init(hvbms_state);
    DataPackets::LCU_State_init(lcu_vertical_state, lcu_horizontal_state);
    DataPackets::PCU_State_init(pcu_state);
    DataPackets::Remote_States_init(
        hvbms_state,
        pcu_state,
        lcu_vertical_state,
        lcu_horizontal_state
    );
    DataPackets::VCU_State_For_PCU_init(recovery_status);

    OrderPackets::FAULT_init();
    OrderPackets::Recovery_init();
    OrderPackets::Cooling_pump_power_init(cooling_pump_duty, cooling_pump_selection);
    OrderPackets::MANTEINANCE_init();
    OrderPackets::Precharge_init();
    OrderPackets::Stop_init();
    OrderPackets::Propulsion_init();
    OrderPackets::Static_levitation_init();
    OrderPackets::Dynamic_levitation_init();
    OrderPackets::Brake_init();
    OrderPackets::Close_contactors_init();
    OrderPackets::Unbrake_init();
    OrderPackets::Open_contactors_init();
    OrderPackets::Emergency_stop_init();
    OrderPackets::Runs_init(run_id);
    OrderPackets::SVPWM_init(
        modulation_frequency_1,
        commutation_frequency_1,
        reference_voltage_1,
        max_voltage_1,
        motor_direction_1
    );
    OrderPackets::Stop_motor_init();
    OrderPackets::Current_control_init(
        modulation_frequency_2,
        commutation_frequency_2,
        reference_current_2,
        max_voltage_2,
        motor_direction_2
    );
    OrderPackets::Speed_control_init(
        reference_speed_3,
        commutation_frequency_3,
        max_voltage_3,
        motor_direction_3
    );
    OrderPackets::Motor_brake_init();
    OrderPackets::Levitation_init(levitation_distance);
    OrderPackets::Stop_levitation_init();
    OrderPackets::Booster_init();
    OrderPackets::Stop_booster_init();
    OrderPackets::Remote_close_contactors_init();
    OrderPackets::Remote_open_contactors_init();
    OrderPackets::Remote_precharge_init();
    OrderPackets::Remote_runs_init(run_id);
    OrderPackets::Remote_SVPWM_init(
        modulation_frequency_1,
        commutation_frequency_1,
        reference_voltage_1,
        max_voltage_1,
        motor_direction_1
    );
    OrderPackets::Remote_stop_motor_init();
    OrderPackets::Remote_current_control_init(
        modulation_frequency_2,
        commutation_frequency_2,
        reference_current_2,
        max_voltage_2,
        motor_direction_2
    );
    OrderPackets::Remote_speed_control_init(
        reference_speed_3,
        commutation_frequency_3,
        max_voltage_3,
        motor_direction_3
    );
    OrderPackets::Remote_motor_brake_init();
    OrderPackets::Remote_levitation_init(levitation_distance);
    OrderPackets::Remote_stop_levitation_init();
    OrderPackets::Remote_booster_init();
    OrderPackets::Remote_stop_booster_init();
    OrderPackets::Forward_booster_init();

    OrderPackets::start();
    DataPackets::start();

    VCU_StateMachine::start();
}

inline void update() {
    ethernet->update();
    VCU_StateMachine::update();
    FaultController::check_transitions();
    Board::evaluate_protections();
    Diagnostics::Hub::flush();
    Scheduler::update();
}

} // namespace VCU

#endif // VCU_HPP
