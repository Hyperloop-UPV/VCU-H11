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
| HVBMS | `192.168.1.7`, local TCP port `50502`, UDP port `50403` | Contactor orders; UDP state feedback. |
| BMSL | `192.168.1.254`, ports declared but commented in H10 startup. | Battery management reference. |
| LCU | `192.168.1.4`, local TCP port `50504`, UDP port `50405` in H11. H10 had these ports declared but commented in the inspected startup. | Levitation and booster orders; UDP state feedback. |
| BCU | `192.168.2.17`, port declared but not fully active in H10 startup. | Booster reference. |

The inspected H10 snapshot only required control station, HVBMS, and PCU TCP connections for `Connecting -> Operational`.

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

| Order | ID | Behavior |
| --- | ---: | --- |
| `FAULT` | 0 | Enters `Fault` and propagates fault handling. |
| `Cooling pump power` | 33 | Selects pump 1 or 2 and applies the requested pump duty. Duty `100` turns the selected pump on and any other value turns it off. |
| `MANTEINANCE` | 34 | `Connected -> Manteinance`. The spelling follows the current order contract. |
| `Precharge` | 35 | `Connected -> Precharging`. |
| `Stop` | 36 | Returns commandable non-idle modes to `Connected`, running active-mode exit actions. |
| `Propulsion` | 38 | `Ready -> Propulsion`. |
| `Static levitation` | 39 | `Ready -> StaticLevitation`. |
| `Dynamic levitation` | 40 | `Ready` or `StaticLevitation -> DynamicLevitation`. |
| `Brake` | 43 | `Ready`, `Propulsion`, `StaticLevitation`, or `DynamicLevitation -> HVActive`; engages brakes and keeps contactors closed. |
| `Unbrake` | 52 | `HVActive -> Ready` and releases brakes through `Ready` entry. |
| `Emergency stop` | 55 | Enters `Fault` and propagates fault handling. |

## Forwarded Orders

| Domain | Operator order | Remote order ID | Required state | Behavior |
| --- | --- | ---: | --- | --- |
| High-voltage link | Precharge | 902 | `Precharging` entry | Sends the formal HVBMS precharge request over the existing high-voltage remote link until a dedicated HVBMS socket exists. |
| HVBMS | Open contactors | 901 | `Idle`, `Connected`, `Fault`, and stop-to-connected entry paths | Best-effort safe contactor-open request. |
| PCU | Runs | 1000 | `Propulsion`, `DynamicLevitation` | Forwarded as a propulsion-specific order. |
| PCU | SVPWM | 1001 | `Propulsion`, `DynamicLevitation` | Forwarded as a propulsion-specific order. |
| PCU | Stop motor | 1002 | Exit from `Propulsion` or `DynamicLevitation` | Stops propelling. |
| PCU | Current control | 1003 | `Propulsion`, `DynamicLevitation` | Forwarded as a propulsion-specific order. |
| PCU | Speed control | 1004 | `Propulsion`, `DynamicLevitation` | Forwarded as a propulsion-specific order. |
| PCU | Motor brake | 1005 | `Propulsion`, `DynamicLevitation` | Forwarded as a propulsion-specific order. |
| LCU | Levitation | 9989 | `StaticLevitation`, `DynamicLevitation` | Forwarded as a levitation-specific order. |
| LCU | Stop levitation | 9993 | Exit from `StaticLevitation` or `DynamicLevitation` | Stops levitating. |

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
| `hvbms_state` | 941 | Last known HVBMS state. |
| `lcu_state` | 63 | Vertical and horizontal LCU states. |
| `pcu_state` | 64 | Last known PCU state. |
| `vcu_state` | 65 | Recovery status. |

The main telemetry send path transmits local VCU telemetry to the control station over UDP. It also sends the VCU state packet to the PCU UDP socket.

## Command Gating Summary

| Command group | Allowed operational states |
| --- | --- |
| MANTEINANCE, Precharge | `Connected`. |
| Stop | `Manteinance`, `Precharging`, `HVActive`, `Ready`, `Propulsion`, `StaticLevitation`, `DynamicLevitation`. |
| Unbrake | `HVActive`. |
| Brake | `Ready`, `Propulsion`, `StaticLevitation`, `DynamicLevitation`. |
| Propulsion, Static levitation, Dynamic levitation | `Ready`; dynamic levitation is also accepted from `StaticLevitation`. |
| Propulsion-specific low-level orders | `Propulsion`, `DynamicLevitation`. |
| Levitation-specific low-level orders | `StaticLevitation`, `DynamicLevitation`. |
| Emergency stop, FAULT | Always enter `Fault`. |

## Timeout Behavior

The current explicit H11 FSM does not use the old demonstration-bitfield remote
acknowledgement timeouts. Precharge completion is inferred from the existing
high-voltage state packet until a dedicated HVBMS acknowledgement is defined.

Before adding timeout-based faulting back, validate the timing against real
network latency, actuator response, and safety requirements.
