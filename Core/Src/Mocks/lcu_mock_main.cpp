#include "Communications/Packets/DataPackets.hpp"
#include "Mocks/MockPeer.hpp"
#include "ST-LIB.hpp"
#include "main.h"

namespace LCUMock {

inline constexpr const char* local_ip = "192.168.1.4";

inline constexpr auto eth = ST_LIB::EthernetDomain::Ethernet(
    MockPeer::ethernet_pinset,
    "00:80:e1:00:01:04",
    local_ip,
    "255.255.0.0"
);

using Board = MockPeer::Board<eth>;

inline ST_LIB::EthernetDomain::Instance* ethernet = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* status_led = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* disconnected_led = nullptr;
inline ServerSocket* vcu_socket = nullptr;
inline bool connected_to_master = false;
inline bool was_connected_to_master = false;

inline void update_leds() {
    MockPeer::update_connection_leds(
        connected_to_master,
        was_connected_to_master,
        *status_led,
        *disconnected_led
    );
}

inline void init() {
    ethernet = MockPeer::init_ethernet<Board, eth>();
    status_led = &Board::instance_of<MockPeer::status_led_req>();
    disconnected_led = &Board::instance_of<MockPeer::disconnected_led_req>();
    status_led->turn_off();
    disconnected_led->turn_off();
    vcu_socket = MockPeer::create_vcu_server_socket(local_ip);
    DataPackets::LCU_Mock_Connection_Status_init(connected_to_master);
    DataPackets::start();
    Scheduler::register_task(200'000, +[]() { update_leds(); });
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
