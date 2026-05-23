#ifndef VCU_STATE_MACHINE_HPP
#define VCU_STATE_MACHINE_HPP

#ifdef STLIB_ETH
#include "Communications/Packets/OrderPackets.hpp"
#endif
#include "VCU_TYPES.hpp"

namespace VCU_StateMachine {

namespace Detail {

struct PendingRemoteOrder {
    bool sent = false;
    uint64_t sent_at_us = 0;
};

inline PendingRemoteOrder close_contactors_order;
inline PendingRemoteOrder open_contactors_order;
inline PendingRemoteOrder runs_order;
inline PendingRemoteOrder svpwm_order;
inline PendingRemoteOrder stop_motor_order;
inline PendingRemoteOrder current_control_order;
inline PendingRemoteOrder speed_control_order;
inline PendingRemoteOrder motor_brake_order;
inline PendingRemoteOrder levitation_order;
inline PendingRemoteOrder stop_levitation_order;
inline PendingRemoteOrder booster_order;
inline PendingRemoteOrder stop_booster_order;
inline bool booster_forwarded = false;

inline bool elapsed(uint64_t start_us, uint32_t timeout_us) {
    return Scheduler::get_global_tick() - start_us >= timeout_us;
}

inline void mark_sent(PendingRemoteOrder& order) {
    order.sent = true;
    order.sent_at_us = Scheduler::get_global_tick();
}

inline void clear(PendingRemoteOrder& order) {
    order.sent = false;
    order.sent_at_us = 0;
}

inline bool can_start_motion_order() {
    return VCU::operational_state == VCU::OperationalState::Ready ||
           VCU::operational_state == VCU::OperationalState::Demonstration;
}

inline bool can_stop_motion_order() {
    return VCU::operational_state == VCU::OperationalState::Demonstration;
}

inline void update_operational_state() {
    if (VCU::recovery_requested && !VCU::contactors_closed) {
        VCU::operational_state = VCU::OperationalState::Recovery;
    } else if (!VCU::contactors_closed) {
        VCU::operational_state = VCU::OperationalState::Idle;
    } else if (VCU::demonstration_bitfield != 0) {
        VCU::operational_state = VCU::OperationalState::Demonstration;
    } else if (VCU::active_brakes) {
        VCU::operational_state = VCU::OperationalState::Energized;
    } else {
        VCU::operational_state = VCU::OperationalState::Ready;
    }

    VCU::sync_state_telemetry();
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

#ifdef STLIB_ETH
inline bool socket_connected(auto* socket) {
    return socket != nullptr && socket->is_connected();
}

inline void refresh_connections() {
    VCU::control_station_connected = socket_connected(OrderPackets::control_station_tcp);
    VCU::hvscu_connected = socket_connected(OrderPackets::hvscu_tcp);
    VCU::pcu_connected = socket_connected(OrderPackets::pcu_tcp);
    VCU::required_peers_connected =
        VCU::control_station_connected && VCU::hvscu_connected && VCU::pcu_connected;

    if (VCU::required_peers_connected) {
        VCU::required_peers_were_connected = true;
    } else if (VCU::required_peers_were_connected && !FaultController::is_faulted()) {
        FAULT("Required VCU communication peer disconnected");
    }

    VCU::sync_state_telemetry();
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

inline void reject_order(bool& flag, const char* message) {
    WARNING("%s", message);
    flag = false;
}

inline void handle_fault_orders() {
    if (OrderPackets::FAULT_flag) {
        OrderPackets::FAULT_flag = false;
        FAULT("FAULT order received");
    }

    if (OrderPackets::Emergency_stop_flag) {
        OrderPackets::Emergency_stop_flag = false;
        FAULT("Emergency stop order received");
    }
}

inline void handle_local_orders() {
    if (OrderPackets::Recovery_flag) {
        VCU::recovery_requested = true;
        VCU::recovery_status = 1;
        OrderPackets::Recovery_flag = false;
    }

    if (OrderPackets::Cooling_pump_power_flag) {
        const auto selection = static_cast<VCU::PumpSelection>(VCU::cooling_pump_selection);
        VCU::set_cooling_pump(selection, VCU::cooling_pump_duty);
        OrderPackets::Cooling_pump_power_flag = false;
    }

    if (OrderPackets::Brake_flag) {
        if (VCU::operational_state != VCU::OperationalState::Ready &&
            VCU::operational_state != VCU::OperationalState::Demonstration) {
            reject_order(OrderPackets::Brake_flag, "Cannot brake in this operational state");
        } else {
            VCU::engage_brake();
            OrderPackets::Brake_flag = false;
        }
    }

    if (OrderPackets::Unbrake_flag) {
        if (VCU::operational_state != VCU::OperationalState::Energized &&
            VCU::operational_state != VCU::OperationalState::Recovery) {
            reject_order(OrderPackets::Unbrake_flag, "Cannot unbrake in this operational state");
        } else {
            Scheduler::set_timeout(2'000'000, +[]() { VCU::release_brake(); });
            OrderPackets::Unbrake_flag = false;
        }
    }
}

inline void handle_contactor_orders() {
    if (OrderPackets::Close_contactors_flag) {
        if (VCU::operational_state != VCU::OperationalState::Idle) {
            reject_order(
                OrderPackets::Close_contactors_flag,
                "Cannot close contactors outside Idle"
            );
            clear(close_contactors_order);
        } else if (!close_contactors_order.sent) {
            if (send_remote_order(
                    OrderPackets::hvscu_tcp,
                    OrderPackets::Remote_close_contactors_order,
                    "close contactors"
                )) {
                mark_sent(close_contactors_order);
            } else {
                OrderPackets::Close_contactors_flag = false;
                clear(close_contactors_order);
            }
        } else if (VCU::hvscu_state == static_cast<uint8_t>(VCU::HVSCUState::Closed)) {
            VCU::contactors_closed = true;
            OrderPackets::Close_contactors_flag = false;
            clear(close_contactors_order);
        } else if (elapsed(close_contactors_order.sent_at_us, VCU::contactor_ack_timeout_us)) {
            OrderPackets::Close_contactors_flag = false;
            clear(close_contactors_order);
            FAULT("HVSCU did not acknowledge closed contactors");
        }
    }

    if (OrderPackets::Open_contactors_flag) {
        if (!open_contactors_order.sent) {
            if (send_remote_order(
                    OrderPackets::hvscu_tcp,
                    OrderPackets::Remote_open_contactors_order,
                    "open contactors"
                )) {
                mark_sent(open_contactors_order);
            } else {
                OrderPackets::Open_contactors_flag = false;
                clear(open_contactors_order);
            }
        } else if (VCU::hvscu_state == static_cast<uint8_t>(VCU::HVSCUState::Opened)) {
            VCU::contactors_closed = false;
            VCU::engage_brake();
            OrderPackets::Open_contactors_flag = false;
            clear(open_contactors_order);
        } else if (elapsed(open_contactors_order.sent_at_us, VCU::remote_ack_timeout_us)) {
            OrderPackets::Open_contactors_flag = false;
            clear(open_contactors_order);
            FAULT("HVSCU did not acknowledge opened contactors");
        }
    }
}

inline void handle_start_remote_order(
    bool& flag,
    PendingRemoteOrder& pending_order,
    auto* socket,
    HeapOrder* remote_order,
    uint8_t& reported_state,
    uint8_t expected_state,
    uint8_t demonstration_bit,
    const char* description
) {
    if (!flag) {
        return;
    }
    if (!can_start_motion_order()) {
        reject_order(flag, "Cannot start remote motion order in this operational state");
        clear(pending_order);
        return;
    }
    if (!pending_order.sent) {
        if (send_remote_order(socket, remote_order, description)) {
            mark_sent(pending_order);
        } else {
            flag = false;
            clear(pending_order);
        }
        return;
    }
    if (reported_state == expected_state) {
        VCU::demonstration_bitfield |= (1UL << demonstration_bit);
        flag = false;
        clear(pending_order);
        return;
    }
    if (elapsed(pending_order.sent_at_us, VCU::remote_ack_timeout_us)) {
        flag = false;
        clear(pending_order);
        FAULT("Remote order acknowledgement timeout: %s", description);
    }
}

inline void handle_stop_remote_order(
    bool& flag,
    PendingRemoteOrder& pending_order,
    auto* socket,
    HeapOrder* remote_order,
    uint8_t& reported_state,
    uint8_t expected_state,
    uint8_t demonstration_bit,
    const char* description
) {
    if (!flag) {
        return;
    }
    if (!can_stop_motion_order()) {
        reject_order(flag, "Cannot stop remote motion order outside Demonstration");
        clear(pending_order);
        return;
    }
    if (!pending_order.sent) {
        if (send_remote_order(socket, remote_order, description)) {
            mark_sent(pending_order);
        } else {
            flag = false;
            clear(pending_order);
        }
        return;
    }
    if (reported_state == expected_state) {
        VCU::demonstration_bitfield &= ~(1UL << demonstration_bit);
        flag = false;
        clear(pending_order);
        return;
    }
    if (elapsed(pending_order.sent_at_us, VCU::remote_ack_timeout_us)) {
        flag = false;
        clear(pending_order);
        FAULT("Remote order acknowledgement timeout: %s", description);
    }
}

inline void handle_pcu_orders() {
    handle_start_remote_order(
        OrderPackets::Runs_flag,
        runs_order,
        OrderPackets::pcu_tcp,
        OrderPackets::Remote_runs_order,
        VCU::pcu_state,
        static_cast<uint8_t>(VCU::PCUState::Propulsion),
        VCU::propulsion_bit,
        "runs"
    );
    handle_start_remote_order(
        OrderPackets::SVPWM_flag,
        svpwm_order,
        OrderPackets::pcu_tcp,
        OrderPackets::Remote_SVPWM_order,
        VCU::pcu_state,
        static_cast<uint8_t>(VCU::PCUState::Propulsion),
        VCU::propulsion_bit,
        "SVPWM"
    );
    handle_start_remote_order(
        OrderPackets::Current_control_flag,
        current_control_order,
        OrderPackets::pcu_tcp,
        OrderPackets::Remote_current_control_order,
        VCU::pcu_state,
        static_cast<uint8_t>(VCU::PCUState::Propulsion),
        VCU::propulsion_bit,
        "current control"
    );
    handle_start_remote_order(
        OrderPackets::Speed_control_flag,
        speed_control_order,
        OrderPackets::pcu_tcp,
        OrderPackets::Remote_speed_control_order,
        VCU::pcu_state,
        static_cast<uint8_t>(VCU::PCUState::Propulsion),
        VCU::propulsion_bit,
        "speed control"
    );
    handle_start_remote_order(
        OrderPackets::Motor_brake_flag,
        motor_brake_order,
        OrderPackets::pcu_tcp,
        OrderPackets::Remote_motor_brake_order,
        VCU::pcu_state,
        static_cast<uint8_t>(VCU::PCUState::Propulsion),
        VCU::propulsion_bit,
        "motor brake"
    );
    handle_stop_remote_order(
        OrderPackets::Stop_motor_flag,
        stop_motor_order,
        OrderPackets::pcu_tcp,
        OrderPackets::Remote_stop_motor_order,
        VCU::pcu_state,
        static_cast<uint8_t>(VCU::PCUState::Stopped),
        VCU::propulsion_bit,
        "stop motor"
    );
}

inline void handle_lcu_orders() {
    handle_start_remote_order(
        OrderPackets::Levitation_flag,
        levitation_order,
        OrderPackets::lcu_tcp,
        OrderPackets::Remote_levitation_order,
        VCU::lcu_vertical_state,
        static_cast<uint8_t>(VCU::LCUState::Levitation),
        VCU::levitation_bit,
        "levitation"
    );
    handle_stop_remote_order(
        OrderPackets::Stop_levitation_flag,
        stop_levitation_order,
        OrderPackets::lcu_tcp,
        OrderPackets::Remote_stop_levitation_order,
        VCU::lcu_vertical_state,
        static_cast<uint8_t>(VCU::LCUState::Stopped),
        VCU::levitation_bit,
        "stop levitation"
    );
    handle_start_remote_order(
        OrderPackets::Booster_flag,
        booster_order,
        OrderPackets::lcu_tcp,
        OrderPackets::Remote_booster_order,
        VCU::lcu_horizontal_state,
        static_cast<uint8_t>(VCU::BoosterState::Enabled),
        VCU::booster_bit,
        "booster"
    );
    if (!OrderPackets::Booster_flag &&
        !booster_forwarded &&
        ((VCU::demonstration_bitfield & (1UL << VCU::booster_bit)) != 0) &&
        VCU::lcu_horizontal_state == static_cast<uint8_t>(VCU::BoosterState::Enabled) &&
        OrderPackets::bcu_tcp != nullptr && OrderPackets::bcu_tcp->is_connected()) {
        OrderPackets::bcu_tcp->send_order(*OrderPackets::Forward_booster_order);
        booster_forwarded = true;
    }
    handle_stop_remote_order(
        OrderPackets::Stop_booster_flag,
        stop_booster_order,
        OrderPackets::lcu_tcp,
        OrderPackets::Remote_stop_booster_order,
        VCU::lcu_horizontal_state,
        static_cast<uint8_t>(VCU::BoosterState::Disabled),
        VCU::booster_bit,
        "stop booster"
    );
    if (VCU::lcu_horizontal_state == static_cast<uint8_t>(VCU::BoosterState::Disabled)) {
        booster_forwarded = false;
    }
}

inline void clear_remote_callback_flags() {
    OrderPackets::Remote_close_contactors_flag = false;
    OrderPackets::Remote_open_contactors_flag = false;
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
    handle_local_orders();
    handle_contactor_orders();
    handle_pcu_orders();
    handle_lcu_orders();
    clear_remote_callback_flags();
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
    if (FaultController::is_faulted()) {
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
    VCU::sync_state_telemetry();
    Detail::sample_inputs();
    Detail::refresh_connections();
    Scheduler::register_task(100'000, +[]() { Detail::sample_inputs(); });
    Scheduler::register_task(200'000, +[]() { Detail::update_status_leds(); });
}

inline void update() {
    Detail::sample_inputs();
    Detail::refresh_connections();
    Detail::update_operational_state();
    Detail::handle_orders();
    Detail::update_operational_state();
}

} // namespace VCU_StateMachine

#endif // VCU_STATE_MACHINE_HPP
