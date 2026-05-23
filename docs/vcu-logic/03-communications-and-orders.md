# Communications and Orders

## Network Role

The H10 VCU is both a server for operator/control-station commands and a client of remote vehicle boards. It also sends UDP telemetry.

| Endpoint | H10 address or port | Purpose |
| --- | --- | --- |
| VCU | `192.168.1.3` | Local board IP. |
| Control station | `192.168.0.9` | Operator endpoint for telemetry. |
| Control station TCP | local port `50500` | Receives operator orders. |
| Control station UDP | local and remote port `50400` | Sends VCU telemetry. |
| PCU | `192.168.1.5`, local TCP port `50501`, UDP port `50402` | Propulsion and motor-control orders; UDP state feedback. |
| HVSCU | `192.168.1.7`, local TCP port `50502`, UDP port `50403` | Contactor orders; UDP state feedback. |
| BMSL | `192.168.1.254`, ports declared but commented in H10 startup. | Battery management reference. |
| LCU | `192.168.1.4`, local TCP port `50504`, UDP port `50405` in H11. H10 had these ports declared but commented in the inspected startup. | Levitation and booster orders; UDP state feedback. |
| BCU | `192.168.2.17`, port declared but not fully active in H10 startup. | Booster reference. |

The inspected H10 snapshot only required control station, HVSCU, and PCU TCP connections for `Connecting -> Operational`.

## Order Handling Pattern

Most orders use the same high-level flow:

1. A `HeapOrder` is registered with an ID, callback, and optional argument storage.
2. The callback records arguments and sets an order flag.
3. `Comms::update()` calls `check_orders()`.
4. The matching `check_*_order()` function verifies the current operational state.
5. The command is either executed locally or sent to a remote board.
6. For remote commands, the VCU waits until a remote-state variable matches the expected acknowledgement.
7. On success, the VCU clears sent flags, clears the order flag, and updates local state or the demonstration bitfield.
8. On timeout, the VCU calls fault propagation.

This split is important: packet callbacks should stay short and should not bypass state gating.

## Local Orders

| Order | H10 ID | Behavior |
| --- | ---: | --- |
| `Recovery` | 32 | Sets the recovery flag, allowing `Idle -> Recovery`. |
| `Potencia_refri` | 33 | Selects pump 1 or 2 and applies the requested pump duty. In H10, duty `100` turns the selected pump on and any other value turns it off. |
| `Set_Regulator` | 34 | H10-only regulator command. H11 does not expose this order because regulator pressure is set manually outside firmware. |
| `Enable_tapes` | 35 | Drives the H10 tape-enable output to the legacy enabled condition. |
| `Disable_tapes` | 36 | Drives the H10 tape-enable output to the legacy disabled condition. |
| `Brake` | 43 | If `Ready` or `Demonstration`, commands the local brake. H11 does not change regulator pressure. |
| `Unbrake` | 52 | If `Energized` or `Recovery`, waits 2 seconds, then releases the brake. H11 does not change regulator pressure. |
| `Close_contactors` | 44 | If `Idle`, forwards close request to HVSCU and waits for closed acknowledgement. |
| `Open_contactors` | 53 | Forwards open request to HVSCU and waits for opened acknowledgement. |
| `Emergency_stop` | 55 | Immediately faults and propagates the fault. |
| `Reset_vehicle` | 62 | Declared in the enum but not implemented in the inspected H10 handlers. |

## Forwarded Orders

| Domain | Operator order | Remote order ID | Required state | Success acknowledgement |
| --- | --- | ---: | --- | --- |
| HVSCU | Close contactors | 900 | `Idle` | `hvscu_state == HVSCU_Closed` (`2`). |
| HVSCU | Open contactors | 901 | Any checked state in H10 | `hvscu_state == HVSCU_Opened` (`0`). |
| PCU | Runs | 1000 | `Ready` or `Demonstration` | `pcu_state == PCU_Propulsion` (`2`). |
| PCU | SVPWM | 1001 | `Ready` or `Demonstration` | `pcu_state == PCU_Propulsion` (`2`). |
| PCU | Stop motor | 1002 | `Demonstration` | `pcu_state == PCU_Stop_Propulsion` (`0`). |
| PCU | Current control | 1003 | `Ready` or `Demonstration` | `pcu_state == PCU_Propulsion` (`2`). |
| PCU | Speed control | 1004 | `Ready` or `Demonstration` | `pcu_state == PCU_Propulsion` (`2`). |
| PCU | Motor brake | 1005 | `Ready` or `Demonstration` | `pcu_state == PCU_Propulsion` (`2`). |
| LCU | Levitation | 9989 | `Ready` or `Demonstration` | `lcu_v_state == LCU_Levitation` (`3`). |
| LCU | Stop levitation | 9993 | `Demonstration` | `lcu_v_state == LCU_Stop_Levitation` (`0`). |
| LCU/BCU | Booster | 1790 | `Ready` or `Demonstration` | `lcu_h_state == Booster_Enabled` (`1`), then forward booster order to BCU. |
| LCU | Stop booster | 1789 | `Demonstration` | `lcu_h_state == Booster_Disabled` (`0`). |

The H10 propulsion handler contains legacy inconsistencies: one path sends `remote_levitation` through the PCU socket, and the stop-propulsion function checks levitation flags. Treat those as implementation defects, not intended behavior.

## Telemetry Packets

The H10 code registers these outgoing packets:

| Packet | H10 ID | Variables |
| --- | ---: | --- |
| `States` | 249 | General state, operational state. |
| `Flow` | 250 | Flow 1, flow 2. |
| `Reeds` | 251 | Reed 1 through reed 4, all-reeds status. |
| `Regulator` | 252 | H10 regulator 1 pressure command, regulator 2 pressure command. Not used on H11. |
| `Pressure` | 253 | H10 high pressure, regulator pressure, brake pressure, capsule pressure. H11 publishes high pressure, low pressure, and manual-regulator feedback. |
| `Tapes` | 254 | Tape-enable status, tape emergency input. |
| `Sdc` | 255 | SDC state. |
| `hvscu_state` | 941 | Last known HVSCU state. |
| `lcu_state` | 63 | Vertical and horizontal LCU states. |
| `pcu_state` | 64 | Last known PCU state. |
| `vcu_state` | 65 | Recovery status. |

The main telemetry send path transmits local VCU telemetry to the control station over UDP. It also sends the VCU state packet to the PCU UDP socket.

## Command Gating Summary

| Command group | Allowed operational states |
| --- | --- |
| Close contactors | `Idle`. |
| Brake | `Ready`, `Demonstration`. |
| Unbrake | `Energized`, `Recovery`. |
| Run or motion-start commands | `Ready`, `Demonstration`. |
| Motion-stop commands | `Demonstration`. |
| Open contactors | No explicit state gate in H10, but it updates the contactor state and can force `Idle` through the operational state machine. |
| Emergency stop | Always faults immediately. |

## Timeout Behavior

H10 uses timeout-based fault propagation when remote acknowledgements do not arrive.

The exact timeout values in the old code are inconsistent:

- close contactors uses a 6000 ms timeout;
- many remote action acknowledgements use 100 ms;
- H10 unbrake waited 2000 ms before releasing the brake after raising regulator pressure. H11 keeps the release delay but does not command regulator pressure.

Before porting to H11, validate these timings against real network latency, actuator response, and safety requirements.
