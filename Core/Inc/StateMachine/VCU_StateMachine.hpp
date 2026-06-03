#ifndef VCU_STATE_MACHINE_HPP
#define VCU_STATE_MACHINE_HPP

#ifdef STLIB_ETH
#include "Communications/Packets/OrderPackets.hpp"
#endif
#include "VCU_TYPES.hpp"

namespace VCU_StateMachine {

namespace Detail {

inline void sample_inputs() {
    VCU::high_pressure_sensor.read();
    VCU::low_pressure_sensor.read();
    VCU::pressure_regulator_out_sensor.read();
    VCU::ntc_temperature_1_sensor.read();
    VCU::ntc_temperature_2_sensor.read();
    VCU::sdc_closed_sensor.read();

    VCU::flow_1 = static_cast<float>(
        VCU::read_flow_capture_frequency<ST_LIB::TimerChannel::CHANNEL_1>()
    );
    VCU::flow_2 = static_cast<float>(
        VCU::read_flow_capture_frequency<ST_LIB::TimerChannel::CHANNEL_2>()
    );
    VCU::sdc_closed = VCU::sdc_closed_state == GPIO_PIN_SET;

    if (VCU::brake_fault != nullptr) {
        VCU::brake_fault_detected = VCU::brake_fault->read() == GPIO_PIN_SET;
    }
    if (VCU::sdmmc_card_detect != nullptr) {
        VCU::sdmmc_card_detected = VCU::sdmmc_card_detect->read() == GPIO_PIN_SET;
    }
    if (VCU::sdmmc_write_protect != nullptr) {
        VCU::sdmmc_write_protected = VCU::sdmmc_write_protect->read() == GPIO_PIN_SET;
    }
}

#ifdef STLIB_ETH
inline bool socket_connected(auto* socket) {
    return socket != nullptr && socket->is_connected();
}

inline void refresh_connections() {
    VCU::control_station_connected = socket_connected(OrderPackets::control_station_tcp);
    VCU::hvscu_connected = false;
    VCU::pcu_connected = false;
    VCU::required_peers_connected = VCU::control_station_connected;
    VCU::sync_state_telemetry();
}

inline void handle_orders() {
    if (OrderPackets::Turn_on_electrovalve_flag) {
        VCU::set_electrovalve(true);
        OrderPackets::Turn_on_electrovalve_flag = false;
    }

    if (OrderPackets::Turn_off_electrovalve_flag) {
        VCU::set_electrovalve(false);
        OrderPackets::Turn_off_electrovalve_flag = false;
    }
}
#else
inline void refresh_connections() {
    VCU::control_station_connected = false;
    VCU::hvscu_connected = false;
    VCU::pcu_connected = false;
    VCU::required_peers_connected = false;
    VCU::sync_state_telemetry();
}

inline void handle_orders() {}
#endif

inline void update_status_leds() {
    if (VCU::required_peers_connected) {
        VCU::led_connecting->turn_off();
        VCU::led_operational->turn_on();
        VCU::led_fault->turn_off();
        return;
    }

    VCU::led_operational->turn_off();
    VCU::led_fault->turn_off();
    VCU::led_connecting->toggle();
}

} // namespace Detail

inline void start() {
    VCU::operational_state = VCU::OperationalState::Idle;
    VCU::sync_state_telemetry();
    Detail::sample_inputs();
    Detail::refresh_connections();
    Scheduler::register_task(100'000, +[]() { Detail::sample_inputs(); });
    Scheduler::register_task(200'000, +[]() { Detail::update_status_leds(); });
}

inline void update() {
    Detail::sample_inputs();
    Detail::refresh_connections();
    Detail::handle_orders();
}

} // namespace VCU_StateMachine

#endif // VCU_STATE_MACHINE_HPP
