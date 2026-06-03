#ifndef VCU_STATE_MACHINE_HPP
#define VCU_STATE_MACHINE_HPP

#include "Communications/Packets/OrderPackets.hpp"
#include "VCU_TYPES.hpp"

namespace VCU_StateMachine {

namespace Detail {

inline bool is_state(VCU::OperationalState state) {
    return VCU::operational_state == state;
}

inline bool is_fault_state() {
    return is_state(VCU::OperationalState::Fault) || FaultController::is_faulted();
}

inline void open_sdc() {
    // H11 currently exposes SDC as an input/protection, not as a controllable output.
}

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

inline void check_pressure_warning() {
    if (VCU::high_pressure < VCU::high_pressure_warning_threshold_bar) {
        if (!VCU::high_pressure_warning_active) {
            WARNING("High pressure below 50 bar");
            VCU::high_pressure_warning_active = true;
        }
        return;
    }

    VCU::high_pressure_warning_active = false;
}

inline bool socket_connected(auto* socket) {
    return socket != nullptr && socket->is_connected();
}

inline bool send_remote_order(auto* socket, HeapOrder* order, const char* description) {
    if (socket == nullptr || order == nullptr) {
        FAULT("Cannot send %s: socket or order is not initialized", description);
        return false;
    }
    if (!socket->is_connected()) {
        FAULT("Cannot send %s: remote socket is disconnected", description);
        return false;
    }
    if (!socket->send_order(*order)) {
        FAULT("Failed to send %s", description);
        return false;
    }
    return true;
}

inline void request_open_contactors() {
    if (OrderPackets::hvbms_tcp != nullptr && OrderPackets::hvbms_tcp->is_connected() &&
        OrderPackets::Remote_open_contactors_order != nullptr) {
        OrderPackets::hvbms_tcp->send_order(*OrderPackets::Remote_open_contactors_order);
    }
    VCU::contactors_closed = false;
}

inline void request_precharge() {
    send_remote_order(
        OrderPackets::hvbms_tcp,
        OrderPackets::Remote_precharge_order,
        "precharge"
    );
}

inline void stop_propulsion() {
    send_remote_order(
        OrderPackets::pcu_tcp,
        OrderPackets::Remote_stop_motor_order,
        "stop propulsion"
    );
}

inline void stop_levitation() {
    send_remote_order(
        OrderPackets::lcu_tcp,
        OrderPackets::Remote_stop_levitation_order,
        "stop levitation"
    );
}

inline void start_propulsion() {
    send_remote_order(OrderPackets::pcu_tcp, OrderPackets::Remote_runs_order, "propulsion");
}

inline void start_static_levitation() {
    send_remote_order(
        OrderPackets::lcu_tcp,
        OrderPackets::Remote_levitation_order,
        "static levitation"
    );
}

inline void start_dynamic_levitation() {
    start_static_levitation();
    start_propulsion();
}

inline void propagate_fault() {
    if (OrderPackets::pcu_tcp != nullptr && OrderPackets::pcu_tcp->is_connected()) {
        OrderPackets::pcu_tcp->send_order(*OrderPackets::FAULT_order);
    }
    if (OrderPackets::hvbms_tcp != nullptr && OrderPackets::hvbms_tcp->is_connected()) {
        OrderPackets::hvbms_tcp->send_order(*OrderPackets::FAULT_order);
    }
    if (OrderPackets::lcu_tcp != nullptr && OrderPackets::lcu_tcp->is_connected()) {
        OrderPackets::lcu_tcp->send_order(*OrderPackets::FAULT_order);
    }
}

inline void refresh_connections() {
    VCU::control_station_connected = socket_connected(OrderPackets::control_station_tcp);
    VCU::hvbms_connected = socket_connected(OrderPackets::hvbms_tcp);
    VCU::pcu_connected = socket_connected(OrderPackets::pcu_tcp);
    VCU::lcu_connected = socket_connected(OrderPackets::lcu_tcp);
    VCU::required_peers_connected = VCU::control_station_connected && VCU::hvbms_connected &&
                                    VCU::pcu_connected && VCU::lcu_connected;

    if (VCU::required_peers_connected) {
        VCU::required_peers_were_connected = true;
    }
    VCU::sync_state_telemetry();
}

inline void exit_state(VCU::OperationalState state) {
    switch (state) {
    case VCU::OperationalState::Propulsion:
        stop_propulsion();
        break;
    case VCU::OperationalState::StaticLevitation:
        stop_levitation();
        break;
    case VCU::OperationalState::DynamicLevitation:
        stop_propulsion();
        stop_levitation();
        break;
    default:
        break;
    }
}

inline void enter_state(VCU::OperationalState state) {
    switch (state) {
    case VCU::OperationalState::Idle:
        VCU::engage_brake();
        request_open_contactors();
        break;
    case VCU::OperationalState::Connected:
        VCU::engage_brake();
        request_open_contactors();
        break;
    case VCU::OperationalState::Manteinance:
        VCU::release_brake();
        break;
    case VCU::OperationalState::Precharging:
        request_precharge();
        break;
    case VCU::OperationalState::HVActive:
        break;
    case VCU::OperationalState::Ready:
        VCU::release_brake();
        break;
    case VCU::OperationalState::Propulsion:
        start_propulsion();
        break;
    case VCU::OperationalState::StaticLevitation:
        start_static_levitation();
        break;
    case VCU::OperationalState::DynamicLevitation:
        start_dynamic_levitation();
        break;
    case VCU::OperationalState::Fault:
        open_sdc();
        if (VCU::led_fault != nullptr) {
            VCU::led_fault->turn_on();
        }
        propagate_fault();
        request_open_contactors();
        VCU::engage_brake();
        break;
    }
    VCU::sync_state_telemetry();
}

inline void transition_to(VCU::OperationalState next_state) {
    if (VCU::operational_state == next_state) {
        return;
    }

    const auto previous_state = VCU::operational_state;
    exit_state(previous_state);
    VCU::operational_state = next_state;
    enter_state(next_state);
}

inline void transition_to_fault(const char* reason) {
    if (!is_state(VCU::OperationalState::Fault)) {
        exit_state(VCU::operational_state);
        VCU::operational_state = VCU::OperationalState::Fault;
        enter_state(VCU::OperationalState::Fault);
    }
    if (!FaultController::is_faulted()) {
        FAULT("%s", reason);
    }
}

inline void reject_order(bool& flag, const char* message) {
    WARNING("%s", message);
    flag = false;
}

inline void check_fault_inputs() {
    if (is_fault_state()) {
        return;
    }

    if (VCU::brake_fault_detected) {
        transition_to_fault("Brake fault detected");
        return;
    }

    if (VCU::tapes_reached) {
        transition_to_fault("Tapes reached");
        return;
    }

    if (!VCU::sdc_closed) {
        transition_to_fault("SDC is open");
        return;
    }

    if (VCU::required_peers_were_connected && !VCU::control_station_connected &&
        !is_state(VCU::OperationalState::Idle)) {
        transition_to_fault("Control station disconnected");
    }
}

inline void handle_connection_transition() {
    if (is_state(VCU::OperationalState::Idle) && VCU::required_peers_connected) {
        transition_to(VCU::OperationalState::Connected);
    }
}

inline void handle_fault_orders() {
    if (OrderPackets::FAULT_flag) {
        OrderPackets::FAULT_flag = false;
        transition_to_fault("FAULT order received");
    }

    if (OrderPackets::Emergency_stop_flag) {
        OrderPackets::Emergency_stop_flag = false;
        transition_to_fault("Emergency stop order received");
    }
}

inline void handle_common_orders() {
    if (OrderPackets::Cooling_pump_power_flag) {
        const auto selection = static_cast<VCU::PumpSelection>(VCU::cooling_pump_selection);
        VCU::set_cooling_pump(selection, VCU::cooling_pump_duty);
        OrderPackets::Cooling_pump_power_flag = false;
    }

    if (OrderPackets::Stop_flag) {
        OrderPackets::Stop_flag = false;
        switch (VCU::operational_state) {
        case VCU::OperationalState::Manteinance:
        case VCU::OperationalState::Precharging:
        case VCU::OperationalState::HVActive:
        case VCU::OperationalState::Ready:
        case VCU::OperationalState::Propulsion:
        case VCU::OperationalState::StaticLevitation:
        case VCU::OperationalState::DynamicLevitation:
            transition_to(VCU::OperationalState::Connected);
            break;
        default:
            break;
        }
        return;
    }
}

inline void handle_connected_orders() {
    if (!is_state(VCU::OperationalState::Connected)) {
        return;
    }

    if (OrderPackets::MANTEINANCE_flag) {
        OrderPackets::MANTEINANCE_flag = false;
        transition_to(VCU::OperationalState::Manteinance);
        return;
    }

    if (OrderPackets::Precharge_flag) {
        OrderPackets::Precharge_flag = false;
        transition_to(VCU::OperationalState::Precharging);
        return;
    }
}

inline void handle_precharging() {
    if (!is_state(VCU::OperationalState::Precharging)) {
        return;
    }

    if (VCU::hvbms_state == static_cast<uint8_t>(VCU::HVBMSState::Closed)) {
        VCU::contactors_closed = true;
        transition_to(VCU::OperationalState::HVActive);
        return;
    }
}

inline void handle_hv_active_orders() {
    if (!is_state(VCU::OperationalState::HVActive)) {
        return;
    }

    if (OrderPackets::Unbrake_flag) {
        OrderPackets::Unbrake_flag = false;
        transition_to(VCU::OperationalState::Ready);
        return;
    }
}

inline void handle_ready_orders() {
    if (!is_state(VCU::OperationalState::Ready)) {
        return;
    }

    if (OrderPackets::Brake_flag) {
        OrderPackets::Brake_flag = false;
        VCU::engage_brake();
        transition_to(VCU::OperationalState::HVActive);
        return;
    }

    if (OrderPackets::Propulsion_flag) {
        OrderPackets::Propulsion_flag = false;
        transition_to(VCU::OperationalState::Propulsion);
        return;
    }

    if (OrderPackets::Static_levitation_flag) {
        OrderPackets::Static_levitation_flag = false;
        transition_to(VCU::OperationalState::StaticLevitation);
        return;
    }

    if (OrderPackets::Dynamic_levitation_flag) {
        OrderPackets::Dynamic_levitation_flag = false;
        transition_to(VCU::OperationalState::DynamicLevitation);
        return;
    }
}

inline void handle_static_levitation_orders() {
    if (!is_state(VCU::OperationalState::StaticLevitation)) {
        return;
    }

    if (OrderPackets::Dynamic_levitation_flag) {
        OrderPackets::Dynamic_levitation_flag = false;
        transition_to(VCU::OperationalState::DynamicLevitation);
        return;
    }
}

inline void handle_propulsion_orders() {
    if (!is_state(VCU::OperationalState::Propulsion) &&
        !is_state(VCU::OperationalState::DynamicLevitation)) {
        return;
    }

    if (OrderPackets::Runs_flag) {
        OrderPackets::Runs_flag = false;
        send_remote_order(OrderPackets::pcu_tcp, OrderPackets::Remote_runs_order, "runs");
    }
    if (OrderPackets::SVPWM_flag) {
        OrderPackets::SVPWM_flag = false;
        send_remote_order(OrderPackets::pcu_tcp, OrderPackets::Remote_SVPWM_order, "SVPWM");
    }
    if (OrderPackets::Current_control_flag) {
        OrderPackets::Current_control_flag = false;
        send_remote_order(
            OrderPackets::pcu_tcp,
            OrderPackets::Remote_current_control_order,
            "current control"
        );
    }
    if (OrderPackets::Speed_control_flag) {
        OrderPackets::Speed_control_flag = false;
        send_remote_order(
            OrderPackets::pcu_tcp,
            OrderPackets::Remote_speed_control_order,
            "speed control"
        );
    }
    if (OrderPackets::Motor_brake_flag) {
        OrderPackets::Motor_brake_flag = false;
        send_remote_order(
            OrderPackets::pcu_tcp,
            OrderPackets::Remote_motor_brake_order,
            "motor brake"
        );
    }
}

inline void handle_levitation_orders() {
    if (!is_state(VCU::OperationalState::StaticLevitation) &&
        !is_state(VCU::OperationalState::DynamicLevitation)) {
        return;
    }

    if (OrderPackets::Levitation_flag) {
        OrderPackets::Levitation_flag = false;
        send_remote_order(
            OrderPackets::lcu_tcp,
            OrderPackets::Remote_levitation_order,
            "levitation"
        );
    }
}

inline void clear_remote_callback_flags() {
    OrderPackets::Remote_close_contactors_flag = false;
    OrderPackets::Remote_open_contactors_flag = false;
    OrderPackets::Remote_precharge_flag = false;
    OrderPackets::Remote_runs_flag = false;
    OrderPackets::Remote_SVPWM_flag = false;
    OrderPackets::Remote_stop_motor_flag = false;
    OrderPackets::Remote_current_control_flag = false;
    OrderPackets::Remote_speed_control_flag = false;
    OrderPackets::Remote_motor_brake_flag = false;
    OrderPackets::Remote_levitation_flag = false;
    OrderPackets::Remote_stop_levitation_flag = false;
    OrderPackets::Remote_booster_flag = false;
    OrderPackets::Remote_stop_booster_flag = false;
    OrderPackets::Forward_booster_flag = false;
}

inline void handle_orders() {
    handle_fault_orders();
    handle_common_orders();
    handle_connected_orders();
    handle_precharging();
    handle_hv_active_orders();
    handle_ready_orders();
    handle_static_levitation_orders();
    handle_propulsion_orders();
    handle_levitation_orders();
    clear_remote_callback_flags();
}

inline void update_status_leds() {
    if (is_fault_state()) {
        if (VCU::led_connecting != nullptr) {
            VCU::led_connecting->turn_off();
        }
        if (VCU::led_operational != nullptr) {
            VCU::led_operational->turn_off();
        }
        if (VCU::led_fault != nullptr) {
            VCU::led_fault->turn_on();
        }
        return;
    }

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
    Detail::enter_state(VCU::OperationalState::Idle);
    Detail::sample_inputs();
    Detail::refresh_connections();
    Scheduler::register_task(100'000, +[]() { Detail::sample_inputs(); });
    Scheduler::register_task(200'000, +[]() { Detail::update_status_leds(); });
}

inline void update() {
    Detail::sample_inputs();
    Detail::check_pressure_warning();
    Detail::refresh_connections();
    Detail::check_fault_inputs();
    if (Detail::is_fault_state()) {
        return;
    }
    Detail::handle_connection_transition();
    Detail::handle_orders();
}

} // namespace VCU_StateMachine

#endif // VCU_STATE_MACHINE_HPP
