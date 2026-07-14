#ifndef VCU_HPP
#define VCU_HPP

#include "Communications/Packets/DataPackets.hpp"
#include "Communications/Packets/OrderPackets.hpp"
#include "Communications/RemoteBoards.hpp"
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
    brakes_status_req,
    sdc_closed_req,
    high_pressure_adc_req,
    low_pressure_adc_req,
    pressure_regulator_out_adc_req,
    ntc_temperature_1_adc_req,
    ntc_temperature_2_adc_req,
#ifndef DISABLE_BRAKE_UNBRAKED_PROTECTION
    brakes_unbraked_protection,
#endif
    sdc_closed_protection>;

inline void init() {
    Board::init();
    // HAL_Delay(10000);

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
    brakes_status_input = &Board::instance_of<brakes_status_req>();
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
    sdc_closed_sensor = SensorInterrupt(*sdc_closed_interrupt, sdc_closed_state);
    ntc_temperature_1_sensor =
        NTC(Board::instance_of<ntc_temperature_1_adc_req>(), ntc_temperature_1);
    ntc_temperature_2_sensor =
        NTC(Board::instance_of<ntc_temperature_2_adc_req>(), ntc_temperature_2);

    sdc_closed_interrupt->turn_on();
    led_operational->turn_off();
    led_sleep->turn_off();
    led_can->turn_off();
    led_connecting->turn_off();
    led_fault->turn_off();

    DataPackets::VCU_State_init(operational_state);
    DataPackets::Pressures_init(high_pressure, low_pressure, pressure_regulator_out);
    DataPackets::Brake_Status_init(active_brakes, reinterpret_cast<uint8_t&>(brakes_status));
    DataPackets::Outputs_init(electrovalve_enabled);
    DataPackets::Safety_init(sdc_closed, hvbms_connected, pcu_connected, lcu_connected);

    RemoteBoards::init_remote_state_receivers();

    OrderPackets::FAULT_init();
    OrderPackets::Maintenance_init();
    OrderPackets::Precharge_init();
    OrderPackets::Stop_init();
    OrderPackets::Propulsion_init();
    OrderPackets::Static_Levitation_init();
    OrderPackets::Dynamic_Levitation_init();
    OrderPackets::Brake_init();
    OrderPackets::Unbrake_init();
    OrderPackets::Open_Contactors_init();

    OrderPackets::Propulsion_Parameterized_init(
        RemoteBoards::propulsion_target_speed,
        RemoteBoards::propulsion_max_current
    );
    OrderPackets::Static_Levitation_Parameterized_init(RemoteBoards::levitation_target_height);
    OrderPackets::Dynamic_Levitation_Parameterized_init(
        RemoteBoards::propulsion_target_speed,
        RemoteBoards::propulsion_max_current,
        RemoteBoards::levitation_target_height
    );

    RemoteBoards::init_remote_orders();

    OrderPackets::start();
    DataPackets::start();

    FaultController::register_fault_propagation(
        OrderPackets::control_station_tcp,
        OrderPackets::FAULT_order
    );
#ifndef SINGLE
    FaultController::register_fault_propagation(OrderPackets::hvbms_tcp, OrderPackets::FAULT_order);
    FaultController::register_fault_propagation(OrderPackets::pcu_tcp, OrderPackets::FAULT_order);
    FaultController::register_fault_propagation(OrderPackets::lcu_tcp, OrderPackets::FAULT_order);
#endif

    Diagnostics::install_ethernet_sink(OrderPackets::control_station_tcp);

    VCU_StateMachine::start();
    using namespace std::chrono_literals;
    Watchdog::watchdog_time = 100ms;
    Watchdog::start();
}

inline void update() {
    ethernet->update();
    VCU_StateMachine::update();
    FaultController::check_transitions();
    Board::evaluate_protections();
    Diagnostics::Hub::flush();
    Scheduler::update();
    Watchdog::refresh();
}

} // namespace VCU

#endif // VCU_HPP
