#ifndef MOCK_PEER_HPP
#define MOCK_PEER_HPP

#include "Mocks/MockHeartbeat.hpp"
#include "ST-LIB.hpp"

#include <cstdint>

#ifndef STLIB_ETH
#error "Mock peers require Ethernet. Build with an Ethernet preset and STLIB_ETH enabled."
#endif

namespace MockPeer {

inline constexpr const char* master_ip = "192.168.1.3";
inline constexpr uint32_t master_tcp_port = VCU_MOCK_MASTER_TCP_PORT;
inline constexpr auto status_led_req = ST_LIB::DigitalOutputDomain::DigitalOutput(ST_LIB::PB0);
inline constexpr auto disconnected_led_req =
    ST_LIB::DigitalOutputDomain::DigitalOutput(ST_LIB::PB14);

#if defined(USE_PHY_LAN8742)
inline constexpr auto ethernet_pinset = ST_LIB::EthernetDomain::PINSET_H10;
#elif defined(USE_PHY_LAN8700) || defined(USE_PHY_KSZ8041)
inline constexpr auto ethernet_pinset = ST_LIB::EthernetDomain::PINSET_H11;
#else
#error "No PHY selected for Ethernet pinset selection"
#endif

inline void on_fault_enter() {}

template <auto& EthernetRequest>
using Board = ST_LIB::Board<
    ST_LIB::FaultPolicyNoMachine<on_fault_enter>,
    EthernetRequest,
    status_led_req,
    disconnected_led_req>;

template <typename BoardT, auto& EthernetRequest>
inline ST_LIB::EthernetDomain::Instance* init_ethernet() {
    BoardT::init();
    return &BoardT::template instance_of<EthernetRequest>();
}

inline ServerSocket* create_vcu_server_socket(const char* local_ip) {
    return new ServerSocket(local_ip, master_tcp_port);
}

inline void service_connection(
    ST_LIB::EthernetDomain::Instance& ethernet,
    Socket& master_socket
) {
    ethernet.update();
    if (!master_socket.is_connected()) {
        master_socket.reconnect();
    }
    master_socket.send();
    FaultController::check_transitions();
    Diagnostics::Hub::flush();
    Scheduler::update();
}

inline void update_connection_leds(
    bool connected_to_master,
    bool was_connected_to_master,
    ST_LIB::DigitalOutputDomain::Instance& status_led,
    ST_LIB::DigitalOutputDomain::Instance& disconnected_led
) {
    if (connected_to_master) {
        status_led.turn_on();
        disconnected_led.turn_off();
    } else if (was_connected_to_master) {
        status_led.turn_off();
        disconnected_led.turn_on();
    } else {
        status_led.toggle();
        disconnected_led.turn_off();
    }
}

inline void service_connection_status(
    ST_LIB::EthernetDomain::Instance& ethernet,
    ServerSocket*& vcu_socket,
    const char* local_ip,
    bool& connected_to_master,
    bool& was_connected_to_master,
    uint64_t& last_master_heartbeat_us
) {
    ethernet.update();

    const uint64_t now = Scheduler::get_global_tick();
    connected_to_master = vcu_socket->is_connected() &&
                          (now - last_master_heartbeat_us) <= MockHeartbeat::timeout_us;

    if (connected_to_master) {
        was_connected_to_master = true;
    } else if (was_connected_to_master && !vcu_socket->is_listening()) {
        delete vcu_socket;
        vcu_socket = create_vcu_server_socket(local_ip);
    }
    vcu_socket->send();
    FaultController::check_transitions();
    Diagnostics::Hub::flush();
    Scheduler::update();
}

} // namespace MockPeer

#endif // MOCK_PEER_HPP
