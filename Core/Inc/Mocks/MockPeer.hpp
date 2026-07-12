#ifndef MOCK_PEER_HPP
#define MOCK_PEER_HPP

#include "ST-LIB.hpp"

#include <cstdint>

#ifndef STLIB_ETH
#error "Mock peers require Ethernet. Build with an Ethernet preset and STLIB_ETH enabled."
#endif

namespace MockPeer {

inline constexpr const char* master_ip = "192.168.1.3";
inline constexpr uint32_t master_tcp_port = VCU_MOCK_MASTER_TCP_PORT;
#ifndef MOCK_NO_LEDS
inline constexpr auto status_led_req = ST_LIB::DigitalOutputDomain::DigitalOutput(ST_LIB::PB0);
inline constexpr auto disconnected_led_req =
    ST_LIB::DigitalOutputDomain::DigitalOutput(ST_LIB::PB14);
#endif

#if defined(USE_PHY_LAN8742) || defined(USE_PHY_LAN8700)
inline constexpr auto ethernet_pinset = ST_LIB::EthernetDomain::PINSET_H10;
#elif defined(USE_PHY_KSZ8041)
inline constexpr auto ethernet_pinset = ST_LIB::EthernetDomain::PINSET_H11;
#else
#error "No PHY selected for Ethernet pinset selection"
#endif

inline void on_fault_enter() {}

#ifdef MOCK_NO_LEDS
template <auto& EthernetRequest>
using Board = ST_LIB::Board<ST_LIB::FaultPolicyNoMachine<on_fault_enter>, EthernetRequest>;
#else
template <auto& EthernetRequest>
using Board = ST_LIB::Board<
    ST_LIB::FaultPolicyNoMachine<on_fault_enter>,
    EthernetRequest,
    status_led_req,
    disconnected_led_req>;
#endif

template <typename BoardT, auto& EthernetRequest>
inline ST_LIB::EthernetDomain::Instance* init_ethernet() {
    BoardT::init();
    return &BoardT::template instance_of<EthernetRequest>();
}

inline ServerSocket* create_vcu_server_socket(const char* local_ip) {
    return new ServerSocket(local_ip, master_tcp_port);
}

// inline void service_connection(
//     ST_LIB::EthernetDomain::Instance& ethernet,
//     Socket& master_socket
// ) {
//     ethernet.update();
//     if (!master_socket.is_connected() || !ethernet.is_connected()) {
//         FAULT("Master socket is disconnected or Ethernet is down");
//     }
//     // if (!master_socket.is_connected()) {
//     //     master_socket.reconnect();
//     // }
//     master_socket.send();
//     FaultController::check_transitions();
//     Diagnostics::Hub::flush();
//     Scheduler::update();
// }

#ifndef MOCK_NO_LEDS
inline void update_connection_leds(
    bool connected_to_master,
    bool was_connected_to_master,
    ST_LIB::DigitalOutputDomain::Instance& status_led,
    ST_LIB::DigitalOutputDomain::Instance& disconnected_led
) {
    if (FaultController::is_faulted()) {
        status_led.turn_off();
        disconnected_led.turn_off();
        return;
    }
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
#endif

inline void service_connection_status(
    ST_LIB::EthernetDomain::Instance& ethernet,
    ServerSocket*& vcu_socket,
    const char* local_ip,
    bool& connected_to_master,
    bool& was_connected_to_master
) {
    ethernet.update();

    connected_to_master = vcu_socket->is_connected();

    if (connected_to_master && !was_connected_to_master) {
        INFO("Connected to master");
    }

    if (connected_to_master) {
        was_connected_to_master = true;
    } else if (was_connected_to_master) {
        FAULT("Master socket is disconnected or Ethernet is down");
        was_connected_to_master = false;
    }

    if (was_connected_to_master && !ethernet.is_connected()) {
        FAULT("Master socket is disconnected or Ethernet is down");
    }
    vcu_socket->send();
    FaultController::check_transitions();
    Diagnostics::Hub::flush();
    Scheduler::update();
}

} // namespace MockPeer

#endif // MOCK_PEER_HPP
