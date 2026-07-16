#ifndef VCU_STATE_MACHINE_HPP
#define VCU_STATE_MACHINE_HPP

#include "Communications/Packets/OrderPackets.hpp"
#include "Communications/RemoteBoards.hpp"
#include "VCU_TYPES.hpp"

namespace VCU_StateMachine {

namespace Detail {

inline void transition_to_fault(const char* reason);

inline bool verification_pending = false;
inline uint16_t verification_timeout_task_id = 0;
inline DataPackets::state verification_target = DataPackets::state::Idle;

inline bool is_state(DataPackets::state state) { return VCU::operational_state == state; }

inline bool is_fault_state() {
    return is_state(DataPackets::state::Fault) || FaultController::is_faulted();
}

inline void sample_inputs() {
    VCU::high_pressure_sensor.read();
    VCU::low_pressure_sensor.read();
    VCU::pressure_regulator_out_sensor.read();
    VCU::sdc_closed_sensor.read();

    VCU::ntc_temperature_1_sensor.read();
    VCU::ntc_temperature_2_sensor.read();

    VCU::sdc_closed = VCU::sdc_closed_state == GPIO_PIN_SET;

    if (VCU::brakes_status_input != nullptr) {
        bool pin_set = VCU::brakes_status_input->read() == GPIO_PIN_SET;
        VCU::brakes_status =
            pin_set ? DataPackets::brakes_status::UNBRAKED : DataPackets::brakes_status::BRAKED;
        VCU::brakes_unbraked = pin_set;
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

inline bool socket_connected(auto* socket) { return socket != nullptr && socket->is_connected(); }

inline void reconnect_if_needed(auto* socket) {
    if (socket != nullptr && !socket->is_connected()) {
        socket->reconnect();
    }
}

inline void handle_connection_change(
    bool connected,
    bool& was_connected,
    const char* board_name,
    const char* disconnect_reason
) {
    if (connected && !was_connected) {
        INFO("%s connected to master", board_name);
    } else if (!connected && was_connected) {
        transition_to_fault(disconnect_reason);
    }
    was_connected = connected;
}

inline bool required_remote_peers_connected() {
    bool ok = true;
#ifdef ENABLE_HVBMS
    ok = ok && VCU::hvbms_connected;
#endif
#ifdef ENABLE_LCU
    ok = ok && VCU::lcu_connected;
#endif
#ifdef ENABLE_PCU
    ok = ok && VCU::pcu_connected;
#endif
    return ok;
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

inline void request_precharge() {
#ifdef SINGLE
    VCU::contactors_closed = true;
#elif defined(ENABLE_HVBMS)
    send_remote_order(OrderPackets::hvbms_tcp, RemoteBoards::Precharge_to_hvbms_order, "precharge");
#endif
}

inline void stop_propulsion() {
#if !defined(SINGLE) && defined(ENABLE_PCU)
    send_remote_order(
        OrderPackets::pcu_tcp,
        RemoteBoards::Stop_Motor_to_pcu_order,
        "stop propulsion"
    );
#endif
}

inline void stop_levitation() {
#if !defined(SINGLE) && defined(ENABLE_LCU)
    send_remote_order(OrderPackets::lcu_tcp, RemoteBoards::Stop_to_lcu_order, "stop levitation");
#endif
}

inline void start_propulsion() {
#if !defined(SINGLE) && defined(ENABLE_PCU)
    send_remote_order(OrderPackets::pcu_tcp, RemoteBoards::Start_SVPWM_to_pcu_order, "propulsion");
#endif
}

inline void start_static_levitation() {
#if !defined(SINGLE) && defined(ENABLE_LCU)
    send_remote_order(
        OrderPackets::lcu_tcp,
        RemoteBoards::Levitate_to_lcu_order,
        "static levitation"
    );
#endif
}

inline void start_dynamic_levitation() {
    start_static_levitation();
    start_propulsion();
}

inline void refresh_connections() {
    VCU::control_station_connected = socket_connected(OrderPackets::control_station_tcp);
#ifdef SINGLE
    VCU::hvbms_connected = false;
    VCU::pcu_connected = false;
    VCU::lcu_connected = false;
    VCU::required_peers_connected = VCU::control_station_connected;
#else
#ifdef ENABLE_HVBMS
    reconnect_if_needed(OrderPackets::hvbms_tcp);
#endif
#ifdef ENABLE_PCU
    reconnect_if_needed(OrderPackets::pcu_tcp);
#endif
#ifdef ENABLE_LCU
    reconnect_if_needed(OrderPackets::lcu_tcp);
#endif

#ifdef ENABLE_HVBMS
    VCU::hvbms_connected = socket_connected(OrderPackets::hvbms_tcp);
#endif
#ifdef ENABLE_PCU
    VCU::pcu_connected = socket_connected(OrderPackets::pcu_tcp);
#endif
#ifdef ENABLE_LCU
    VCU::lcu_connected = socket_connected(OrderPackets::lcu_tcp);
#endif
    VCU::required_peers_connected =
        VCU::control_station_connected && required_remote_peers_connected();
#endif

#ifdef ADJ_COMMIT_VALIDATION
#ifdef ENABLE_HVBMS
    bool hvbms_just_connected = VCU::hvbms_connected && !VCU::hvbms_was_connected;
#endif
#ifdef ENABLE_PCU
    bool pcu_just_connected = VCU::pcu_connected && !VCU::pcu_was_connected;
#endif
#ifdef ENABLE_LCU
    bool lcu_just_connected = VCU::lcu_connected && !VCU::lcu_was_connected;
#endif
#endif

    if (VCU::control_station_connected && !VCU::control_station_was_connected) {
#ifdef ADJ_COMMIT_VALIDATION
        RemoteBoards::adj_hash_verified = false;
        RemoteBoards::adj_hashes_sent = false;
#endif
#ifdef CUSTOM_KEEPALIVE
        if (RemoteBoards::keepalive_timeout_id != Scheduler::INVALID_ID) {
            Scheduler::cancel_timeout(RemoteBoards::keepalive_timeout_id);
        }
        RemoteBoards::keepalive_timeout_id = Scheduler::set_timeout(
            100'000,
            +[]() { keepalive_timeout_trigger(); }
        );
#endif
    }

    handle_connection_change(
        VCU::control_station_connected,
        VCU::control_station_was_connected,
        "Control station",
        "Control station disconnected"
    );
#ifndef SINGLE
#ifdef ENABLE_HVBMS
    handle_connection_change(
        VCU::hvbms_connected,
        VCU::hvbms_was_connected,
        "HVBMS",
        "HVBMS disconnected"
    );
#endif
#ifdef ENABLE_PCU
    handle_connection_change(VCU::pcu_connected, VCU::pcu_was_connected, "PCU", "PCU disconnected");
#endif
#ifdef ENABLE_LCU
    handle_connection_change(VCU::lcu_connected, VCU::lcu_was_connected, "LCU", "LCU disconnected");
#endif

#ifdef ADJ_COMMIT_VALIDATION
    if (RemoteBoards::adj_hash_verified) {
#ifdef ENABLE_HVBMS
        if (hvbms_just_connected) {
            RemoteBoards::adj_hash_on_wire = RemoteBoards::adj_commit_hash_value;
            send_remote_order(
                OrderPackets::hvbms_tcp,
                RemoteBoards::adj_hash_order,
                "ADJ hash to HVBMS"
            );
        }
#endif
#ifdef ENABLE_PCU
        if (pcu_just_connected) {
            RemoteBoards::adj_hash_on_wire = RemoteBoards::adj_commit_hash_value;
            send_remote_order(
                OrderPackets::pcu_tcp,
                RemoteBoards::adj_hash_order,
                "ADJ hash to PCU"
            );
        }
#endif
#ifdef ENABLE_LCU
        if (lcu_just_connected) {
            RemoteBoards::adj_hash_on_wire = RemoteBoards::adj_commit_hash_value;
            send_remote_order(
                OrderPackets::lcu_tcp,
                RemoteBoards::adj_hash_order,
                "ADJ hash to LCU"
            );
        }
#endif

        if (!RemoteBoards::adj_hashes_sent) {
#ifdef ENABLE_HVBMS
            if (VCU::hvbms_connected) {
                RemoteBoards::adj_hash_on_wire = RemoteBoards::adj_commit_hash_value;
                send_remote_order(
                    OrderPackets::hvbms_tcp,
                    RemoteBoards::adj_hash_order,
                    "ADJ hash to HVBMS"
                );
            }
#endif
#ifdef ENABLE_PCU
            if (VCU::pcu_connected) {
                RemoteBoards::adj_hash_on_wire = RemoteBoards::adj_commit_hash_value;
                send_remote_order(
                    OrderPackets::pcu_tcp,
                    RemoteBoards::adj_hash_order,
                    "ADJ hash to PCU"
                );
            }
#endif
#ifdef ENABLE_LCU
            if (VCU::lcu_connected) {
                RemoteBoards::adj_hash_on_wire = RemoteBoards::adj_commit_hash_value;
                send_remote_order(
                    OrderPackets::lcu_tcp,
                    RemoteBoards::adj_hash_order,
                    "ADJ hash to LCU"
                );
            }
#endif
            RemoteBoards::adj_hashes_sent = true;
        }
    }
#endif
#else
    VCU::hvbms_was_connected = false;
    VCU::pcu_was_connected = false;
    VCU::lcu_was_connected = false;
#endif

    if (VCU::required_peers_connected) {
        VCU::required_peers_were_connected = true;
    }
}

inline void exit_state(DataPackets::state state) {
    switch (state) {
    case DataPackets::state::Propulsion:
        stop_propulsion();
        break;
    case DataPackets::state::Static_Levitation:
        stop_levitation();
        break;
    case DataPackets::state::Dynamic_Levitation:
        stop_propulsion();
        stop_levitation();
        break;
    default:
        break;
    }
}

inline void enter_state(DataPackets::state state) {
    switch (state) {
    case DataPackets::state::Idle:
        VCU::engage_brake();
        VCU::request_open_contactors();
        break;
    case DataPackets::state::Connected:
        VCU::engage_brake();
        VCU::request_open_contactors();
        break;
    case DataPackets::state::Maintenance:
        VCU::release_brake();
        break;
    case DataPackets::state::Precharging:
        request_precharge();
        break;
    case DataPackets::state::HVActive:
        break;
    case DataPackets::state::Ready:
        VCU::release_brake();
        break;
    case DataPackets::state::Propulsion:
        start_propulsion();
        break;
    case DataPackets::state::Static_Levitation:
        start_static_levitation();
        break;
    case DataPackets::state::Dynamic_Levitation:
        start_dynamic_levitation();
        break;
    }
}

inline void do_transition(DataPackets::state next_state) {
    if (VCU::operational_state == next_state)
        return;
    const auto previous_state = VCU::operational_state;
    exit_state(previous_state);
    VCU::operational_state = next_state;
    enter_state(next_state);
}

inline void cancel_verification() {
    if (!verification_pending)
        return;
    verification_pending = false;
    Scheduler::cancel_timeout(verification_timeout_task_id);
    verification_timeout_task_id = Scheduler::INVALID_ID;
}

inline bool verify_remote_states(DataPackets::state target) {
    switch (target) {
    case DataPackets::state::Precharging:
#ifdef ENABLE_HVBMS
        return RemoteBoards::hvbms_sm_state == RemoteBoards::HVBMS_State::Precharging;
#else
        return true;
#endif
    case DataPackets::state::HVActive:
#ifdef ENABLE_HVBMS
        return RemoteBoards::hvbms_sm_state == RemoteBoards::HVBMS_State::Energized;
#else
        return true;
#endif
    case DataPackets::state::Propulsion:
#ifdef ENABLE_PCU
        return RemoteBoards::pcu_state == RemoteBoards::PCU_State::Accelerating;
#else
        return true;
#endif
    case DataPackets::state::Static_Levitation:
#ifdef ENABLE_LCU
        return RemoteBoards::lcu_state == RemoteBoards::LCU_State::Levitating;
#else
        return true;
#endif
    case DataPackets::state::Dynamic_Levitation: {
        bool ok = true;
#ifdef ENABLE_LCU
        ok = ok && RemoteBoards::lcu_state == RemoteBoards::LCU_State::Levitating;
#endif
#ifdef ENABLE_PCU
        ok = ok && RemoteBoards::pcu_state == RemoteBoards::PCU_State::Accelerating;
#endif
        return ok;
    }
    case DataPackets::state::Connected: {
        bool ok = true;
#ifdef ENABLE_HVBMS
        ok = ok && (RemoteBoards::hvbms_sm_state == RemoteBoards::HVBMS_State::Idle ||
                    RemoteBoards::hvbms_sm_state == RemoteBoards::HVBMS_State::Ready_To_Precharge);
#endif
#ifdef ENABLE_PCU
        ok = ok && RemoteBoards::pcu_state == RemoteBoards::PCU_State::Idle;
#endif
#ifdef ENABLE_LCU
        ok = ok && RemoteBoards::lcu_state == RemoteBoards::LCU_State::Idle;
#endif
        return ok;
    }
    default:
        return true;
    }
}

inline bool needs_remote_verification(DataPackets::state target) {
    switch (target) {
    case DataPackets::state::Precharging:
    case DataPackets::state::HVActive:
    case DataPackets::state::Propulsion:
    case DataPackets::state::Static_Levitation:
    case DataPackets::state::Dynamic_Levitation:
    case DataPackets::state::Connected:
        return true;
    default:
        return false;
    }
}

#ifdef ENABLE_HVBMS
inline const char* hvbms_state_name(RemoteBoards::HVBMS_State s) {
    switch (s) {
    case RemoteBoards::HVBMS_State::Connecting:
        return "Connecting";
    case RemoteBoards::HVBMS_State::Idle:
        return "Idle";
    case RemoteBoards::HVBMS_State::Ready_To_Precharge:
        return "Ready_To_Precharge";
    case RemoteBoards::HVBMS_State::Precharging:
        return "Precharging";
    case RemoteBoards::HVBMS_State::Energized:
        return "Energized";
    case RemoteBoards::HVBMS_State::Fault:
        return "Fault";
    default:
        return "Unknown";
    }
}
#endif

#ifdef ENABLE_PCU
inline const char* pcu_state_name(RemoteBoards::PCU_State s) {
    switch (s) {
    case RemoteBoards::PCU_State::Connecting:
        return "Connecting";
    case RemoteBoards::PCU_State::Idle:
        return "Idle";
    case RemoteBoards::PCU_State::Accelerating:
        return "Accelerating";
    case RemoteBoards::PCU_State::Fault:
        return "Fault";
    default:
        return "Unknown";
    }
}
#endif

#ifdef ENABLE_LCU
inline const char* lcu_state_name(RemoteBoards::LCU_State s) {
    switch (s) {
    case RemoteBoards::LCU_State::Connecting:
        return "Connecting";
    case RemoteBoards::LCU_State::Idle:
        return "Idle";
    case RemoteBoards::LCU_State::Levitating:
        return "Levitating";
    case RemoteBoards::LCU_State::Current_Control:
        return "Current_Control";
    case RemoteBoards::LCU_State::Debug:
        return "Debug";
    case RemoteBoards::LCU_State::Fault:
        return "Fault";
    default:
        return "Unknown";
    }
}
#endif

inline const char* verification_fault_reason(DataPackets::state target) {
    static char buffer[128];
    switch (target) {
    case DataPackets::state::Precharging:
#ifdef ENABLE_HVBMS
        snprintf(
            buffer,
            sizeof(buffer),
            "HVBMS expected Precharging, got %s",
            hvbms_state_name(RemoteBoards::hvbms_sm_state)
        );
#else
        snprintf(buffer, sizeof(buffer), "Precharging verification failed (board disabled)");
#endif
        break;
    case DataPackets::state::HVActive:
#ifdef ENABLE_HVBMS
        snprintf(
            buffer,
            sizeof(buffer),
            "HVBMS expected Energized, got %s",
            hvbms_state_name(RemoteBoards::hvbms_sm_state)
        );
#else
        snprintf(buffer, sizeof(buffer), "HVActive verification failed (board disabled)");
#endif
        break;
    case DataPackets::state::Propulsion:
#ifdef ENABLE_PCU
        snprintf(
            buffer,
            sizeof(buffer),
            "PCU expected Accelerating, got %s",
            pcu_state_name(RemoteBoards::pcu_state)
        );
#else
        snprintf(buffer, sizeof(buffer), "Propulsion verification failed (board disabled)");
#endif
        break;
    case DataPackets::state::Static_Levitation:
#ifdef ENABLE_LCU
        snprintf(
            buffer,
            sizeof(buffer),
            "LCU expected Levitating, got %s",
            lcu_state_name(RemoteBoards::lcu_state)
        );
#else
        snprintf(buffer, sizeof(buffer), "Static levitation verification failed (board disabled)");
#endif
        break;
    case DataPackets::state::Dynamic_Levitation:
        snprintf(
            buffer,
            sizeof(buffer),
            "LCU expected Levitating got %s, PCU expected Accelerating got %s",
#ifdef ENABLE_LCU
            lcu_state_name(RemoteBoards::lcu_state),
#else
            "(board disabled)",
#endif
#ifdef ENABLE_PCU
            pcu_state_name(RemoteBoards::pcu_state)
#else
            "(board disabled)"
#endif
        );
        break;
    case DataPackets::state::Connected:
        snprintf(
            buffer,
            sizeof(buffer),
            "HVBMS expected Idle got %s, PCU expected Idle got %s, LCU expected Idle got %s",
#ifdef ENABLE_HVBMS
            hvbms_state_name(RemoteBoards::hvbms_sm_state),
#else
            "(board disabled)",
#endif
#ifdef ENABLE_PCU
            pcu_state_name(RemoteBoards::pcu_state),
#else
            "(board disabled)",
#endif
#ifdef ENABLE_LCU
            lcu_state_name(RemoteBoards::lcu_state)
#else
            "(board disabled)"
#endif
        );
        break;
    default:
        snprintf(buffer, sizeof(buffer), "Remote board did not reach expected state");
        break;
    }
    return buffer;
}

inline void on_verification_timeout() {
    if (!verification_pending)
        return;

    if (verify_remote_states(verification_target)) {
        verification_pending = false;
        verification_timeout_task_id = Scheduler::INVALID_ID;
    } else {
        cancel_verification();
        transition_to_fault(verification_fault_reason(verification_target));
    }
}

inline void start_remote_verification(DataPackets::state target) {
    if (!needs_remote_verification(target))
        return;

    if (verification_pending) {
        cancel_verification();
    }

    verification_pending = true;
    verification_target = target;
    verification_timeout_task_id = Scheduler::set_timeout(
        VCU::remote_ack_timeout_us,
        +[]() { on_verification_timeout(); }
    );
}

inline void transition_to(DataPackets::state next_state) {
    if (VCU::operational_state == next_state) {
        return;
    }

    cancel_verification();

    if (next_state == DataPackets::state::Fault) {
        do_transition(DataPackets::state::Fault);
        return;
    }

    do_transition(next_state);
    start_remote_verification(next_state);
}

inline void transition_to_fault(const char* reason) {
    cancel_verification();
    if (!is_state(DataPackets::state::Fault)) {
        exit_state(VCU::operational_state);
        VCU::operational_state = DataPackets::state::Fault;
    }
    if (!FaultController::is_faulted()) {
        FAULT("%s", reason);
    }
}

inline void reject_order(bool& flag, const char* message) {
    WARNING("%s", message);
    flag = false;
}

inline void handle_connection_transition() {
#ifdef ADJ_COMMIT_VALIDATION
    if (is_state(DataPackets::state::Idle) && VCU::required_peers_connected &&
        RemoteBoards::adj_hash_verified) {
#ifdef SINGLE
        transition_to(DataPackets::state::Connected);
#else
        if (RemoteBoards::adj_hashes_sent) {
            transition_to(DataPackets::state::Connected);
        }
#endif
    }
#else
    if (is_state(DataPackets::state::Idle) && VCU::required_peers_connected) {
        transition_to(DataPackets::state::Connected);
    }
#endif
}

inline void handle_fault_orders() {
    if (OrderPackets::FAULT_flag) {
        OrderPackets::FAULT_flag = false;
        transition_to_fault("FAULT order received");
    }
}

inline void handle_common_orders() {
    if (OrderPackets::Stop_flag) {
        OrderPackets::Stop_flag = false;
        switch (VCU::operational_state) {
        case DataPackets::state::Maintenance:
        case DataPackets::state::Precharging:
        case DataPackets::state::HVActive:
        case DataPackets::state::Ready:
        case DataPackets::state::Propulsion:
        case DataPackets::state::Static_Levitation:
        case DataPackets::state::Dynamic_Levitation:
            transition_to(DataPackets::state::Connected);
            break;
        default:
            break;
        }
        return;
    }

    if (OrderPackets::Open_Contactors_flag) {
        OrderPackets::Open_Contactors_flag = false;
        switch (VCU::operational_state) {
        case DataPackets::state::Maintenance:
        case DataPackets::state::Precharging:
        case DataPackets::state::HVActive:
        case DataPackets::state::Ready:
            transition_to(DataPackets::state::Connected);
            break;
        default:
            WARNING("Open contactors order ignored: not in commandable state");
            break;
        }
        return;
    }
}

inline void handle_connected_orders() {
    if (!is_state(DataPackets::state::Connected)) {
        return;
    }

    if (OrderPackets::Maintenance_flag) {
        OrderPackets::Maintenance_flag = false;
        transition_to(DataPackets::state::Maintenance);
        return;
    }

    if (OrderPackets::Precharge_flag) {
        OrderPackets::Precharge_flag = false;
#ifdef ENABLE_HVBMS
        if (RemoteBoards::hvbms_sm_state != RemoteBoards::HVBMS_State::Ready_To_Precharge) {
            WARNING("Cannot precharge: HVBMS not ready to precharge");
            return;
        }
#endif
        transition_to(DataPackets::state::Precharging);
        return;
    }
}

inline void handle_precharging() {
    if (!is_state(DataPackets::state::Precharging))
        return;

#ifdef SINGLE
    if (VCU::contactors_closed) {
        transition_to(DataPackets::state::HVActive);
    }
#elif defined(ENABLE_HVBMS)
    if (RemoteBoards::hvbms_sm_state == RemoteBoards::HVBMS_State::Energized) {
        transition_to(DataPackets::state::HVActive);
    }
#endif
}

inline void handle_hv_active_orders() {
    if (!is_state(DataPackets::state::HVActive)) {
        return;
    }

    if (OrderPackets::Unbrake_flag) {
        OrderPackets::Unbrake_flag = false;
        transition_to(DataPackets::state::Ready);
        return;
    }
}

inline void handle_ready_orders() {
    if (!is_state(DataPackets::state::Ready)) {
        return;
    }

    if (OrderPackets::Brake_flag) {
        OrderPackets::Brake_flag = false;
        VCU::engage_brake();
        transition_to(DataPackets::state::HVActive);
        return;
    }

    if (OrderPackets::Propulsion_flag || OrderPackets::Propulsion_Parameterized_flag) {
        OrderPackets::Propulsion_flag = false;
        OrderPackets::Propulsion_Parameterized_flag = false;
        transition_to(DataPackets::state::Propulsion);
        RemoteBoards::reset_control_params();
        return;
    }

    if (OrderPackets::Static_Levitation_flag ||
        OrderPackets::Static_Levitation_Parameterized_flag) {
        OrderPackets::Static_Levitation_flag = false;
        OrderPackets::Static_Levitation_Parameterized_flag = false;
        transition_to(DataPackets::state::Static_Levitation);
        RemoteBoards::reset_control_params();
        return;
    }

    if (OrderPackets::Dynamic_Levitation_flag ||
        OrderPackets::Dynamic_Levitation_Parameterized_flag) {
        OrderPackets::Dynamic_Levitation_flag = false;
        OrderPackets::Dynamic_Levitation_Parameterized_flag = false;
        transition_to(DataPackets::state::Dynamic_Levitation);
        RemoteBoards::reset_control_params();
        return;
    }
}

inline void handle_active_mode_brake_order() {
    switch (VCU::operational_state) {
    case DataPackets::state::Propulsion:
    case DataPackets::state::Static_Levitation:
    case DataPackets::state::Dynamic_Levitation:
        break;
    default:
        return;
    }

    if (OrderPackets::Brake_flag) {
        OrderPackets::Brake_flag = false;
        VCU::engage_brake();
        transition_to(DataPackets::state::HVActive);
    }
}

inline void handle_static_levitation_orders() {
    if (!is_state(DataPackets::state::Static_Levitation)) {
        return;
    }

    if (OrderPackets::Dynamic_Levitation_flag ||
        OrderPackets::Dynamic_Levitation_Parameterized_flag) {
        OrderPackets::Dynamic_Levitation_flag = false;
        OrderPackets::Dynamic_Levitation_Parameterized_flag = false;
        transition_to(DataPackets::state::Dynamic_Levitation);
        RemoteBoards::reset_control_params();
        return;
    }
}

inline void clear_stale_order_flags() {
    if (OrderPackets::Maintenance_flag) {
        reject_order(
            OrderPackets::Maintenance_flag,
            "Maintenance order rejected: not in Connected state"
        );
    }
    if (OrderPackets::Precharge_flag) {
        reject_order(
            OrderPackets::Precharge_flag,
            "Precharge order rejected: not in Connected state"
        );
    }
    if (OrderPackets::Unbrake_flag) {
        reject_order(OrderPackets::Unbrake_flag, "Unbrake order rejected: not in HVActive state");
    }
    if (OrderPackets::Brake_flag) {
        reject_order(
            OrderPackets::Brake_flag,
            "Brake order rejected: not in Ready or active movement state"
        );
    }
    if (OrderPackets::Propulsion_flag) {
        reject_order(
            OrderPackets::Propulsion_flag,
            "Propulsion order rejected: not in Ready state"
        );
    }
    if (OrderPackets::Propulsion_Parameterized_flag) {
        reject_order(
            OrderPackets::Propulsion_Parameterized_flag,
            "Propulsion order rejected: not in Ready state"
        );
    }
    if (OrderPackets::Static_Levitation_flag) {
        reject_order(
            OrderPackets::Static_Levitation_flag,
            "Static levitation order rejected: not in Ready state"
        );
    }
    if (OrderPackets::Static_Levitation_Parameterized_flag) {
        reject_order(
            OrderPackets::Static_Levitation_Parameterized_flag,
            "Static levitation order rejected: not in Ready state"
        );
    }
    if (OrderPackets::Dynamic_Levitation_flag) {
        reject_order(
            OrderPackets::Dynamic_Levitation_flag,
            "Dynamic levitation order rejected: not in Ready or Static_Levitation state"
        );
    }
    if (OrderPackets::Dynamic_Levitation_Parameterized_flag) {
        reject_order(
            OrderPackets::Dynamic_Levitation_Parameterized_flag,
            "Dynamic levitation order rejected: not in Ready or Static_Levitation state"
        );
    }
    if (OrderPackets::Open_Contactors_flag) {
        reject_order(
            OrderPackets::Open_Contactors_flag,
            "Open contactors order rejected: not in commandable state"
        );
    }
}

inline void handle_orders() {
    handle_fault_orders();
    handle_common_orders();
    handle_connected_orders();
    handle_precharging();
    handle_hv_active_orders();
    handle_ready_orders();
    handle_active_mode_brake_order();
    handle_static_levitation_orders();
    clear_stale_order_flags();
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

    if (is_state(DataPackets::state::Idle)) {
        VCU::led_connecting->turn_off();
        VCU::led_fault->turn_off();
        VCU::led_operational->toggle();
        return;
    }

    VCU::led_operational->turn_off();
    VCU::led_fault->turn_off();
    VCU::led_connecting->toggle();
}

} // namespace Detail

inline void start() {
    VCU::operational_state = DataPackets::state::Idle;
    Detail::enter_state(DataPackets::state::Idle);
    Detail::sample_inputs();
    if (VCU::brakes_status_input != nullptr && VCU::brakes_status_input->read() == GPIO_PIN_SET) {
        Detail::transition_to_fault("brakes are not deployed when the POD turns on");
    }
    Detail::refresh_connections();
    Scheduler::register_task(
        100'000,
        +[]() { Detail::sample_inputs(); }
    );
    Scheduler::register_task(
        200'000,
        +[]() { Detail::update_status_leds(); }
    );
}

inline void update() {
    Detail::sample_inputs();
    Detail::check_pressure_warning();
    Detail::refresh_connections();
    if (Detail::is_fault_state()) {
        return;
    }
    Detail::handle_connection_transition();
    Detail::handle_orders();
}

} // namespace VCU_StateMachine

#endif // VCU_STATE_MACHINE_HPP
