#include "Communications/Packets/DataPackets.hpp"
#include "Mocks/MockPeer.hpp"
#include "ST-LIB.hpp"
#include "main.h"

namespace PCUMock {

inline constexpr const char* local_ip = "192.168.1.5";

inline constexpr auto eth = ST_LIB::EthernetDomain::Ethernet(
    MockPeer::ethernet_pinset,
    "00:80:e1:00:01:05",
    local_ip,
    "255.255.0.0"
);

using Board = MockPeer::Board<eth>;

inline ST_LIB::EthernetDomain::Instance* ethernet = nullptr;
#ifndef MOCK_NO_LEDS
inline ST_LIB::DigitalOutputDomain::Instance* status_led = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* disconnected_led = nullptr;
#endif
inline ServerSocket* vcu_socket = nullptr;
inline bool connected_to_master = false;
inline bool was_connected_to_master = false;

inline DataPackets::pcu_state pcu_state = DataPackets::pcu_state::Stopped;
inline uint8_t dummy_run_id = 0;

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

    new HeapOrder(56, +[]() {
        pcu_state = DataPackets::pcu_state::Propulsion;
    }, &dummy_run_id);
    new HeapOrder(58, +[]() {
        pcu_state = DataPackets::pcu_state::Stopped;
    });
    new HeapOrder(0, +[]() {
        pcu_state = DataPackets::pcu_state::Stopped;
    });

    DataPackets::PCU_Mock_Connection_Status_init(connected_to_master);
    DataPackets::PCU_State_init(pcu_state);
    DataPackets::start();
#ifndef MOCK_NO_LEDS
    Scheduler::register_task(200'000, +[]() { update_leds(); });
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

} // namespace PCUMock

int main(void) {
    Hard_fault_check();
    PCUMock::init();

    while (1) {
        PCUMock::update();
    }
}

extern "C" void Error_Handler(void) {
    PANIC("HAL error handler triggered");
    while (1) {
    }
}
