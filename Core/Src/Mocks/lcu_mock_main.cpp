#include "Mocks/MockPeer.hpp"
#include "ST-LIB.hpp"
#include "main.h"

namespace LCUMock {

inline constexpr const char* local_ip = "192.168.1.4";

inline constexpr auto eth = ST_LIB::EthernetDomain::Ethernet(
    MockPeer::ethernet_pinset,
    "00:80:e1:00:01:04",
    local_ip,
    "255.255.255.0"
);

using Board = MockPeer::Board<eth>;

enum class LCU_State : uint8_t {
    Connecting = 0,
    Idle = 1,
    Levitating = 2,
    Current_Control = 3,
    Debug = 4,
    Fault = 5,
};

inline ST_LIB::EthernetDomain::Instance* ethernet = nullptr;
#ifndef MOCK_NO_LEDS
inline ST_LIB::DigitalOutputDomain::Instance* status_led = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* disconnected_led = nullptr;
#endif
inline ServerSocket* vcu_socket = nullptr;
inline bool connected_to_master = false;
inline bool was_connected_to_master = false;

inline LCU_State lcu_state = LCU_State::Idle;
inline LCU_State lcu_slave_state = LCU_State::Idle;
inline float dummy_levitation_distance = 0.0f;

inline HeapPacket* state_packet = nullptr;
inline DatagramSocket* state_udp = nullptr;

#ifndef MOCK_NO_LEDS
inline void update_leds() {
    MockPeer::update_connection_leds(
        connected_to_master,
        was_connected_to_master,
        *status_led,
        *disconnected_led
    );
}
#endif

inline void init() {
    ethernet = MockPeer::init_ethernet<Board, eth>();
#ifndef MOCK_NO_LEDS
    status_led = &Board::instance_of<MockPeer::status_led_req>();
    disconnected_led = &Board::instance_of<MockPeer::disconnected_led_req>();
    status_led->turn_off();
    disconnected_led->turn_off();
#endif
    vcu_socket = MockPeer::create_vcu_server_socket(local_ip);

    Diagnostics::install_ethernet_sink(vcu_socket);

    new HeapOrder(
        9010,
        +[]() { lcu_state = LCU_State::Levitating; },
        &dummy_levitation_distance
    );
    new HeapOrder(
        9000,
        +[]() { lcu_state = LCU_State::Idle; }
    );

    state_packet = new HeapPacket(static_cast<uint16_t>(9520), &lcu_state, &lcu_slave_state);

    state_udp = new DatagramSocket("192.168.1.4", 50405, "192.168.1.3", 50405);

    Scheduler::register_task(
        500,
        +[]() { state_udp->send_packet(*state_packet); }
    );

#ifndef MOCK_NO_LEDS
    Scheduler::register_task(
        200'000,
        +[]() { update_leds(); }
    );
#endif
}

inline void update() {
    MockPeer::service_connection_status(
        *ethernet,
        vcu_socket,
        local_ip,
        connected_to_master,
        was_connected_to_master
    );
}

} // namespace LCUMock

int main(void) {
    Hard_fault_check();
    LCUMock::init();

    while (1) {
        LCUMock::update();
    }
}

extern "C" void Error_Handler(void) {
    PANIC("HAL error handler triggered");
    while (1) {
    }
}
