# State Machine

## Startup Flow

The H10 application starts in two phases:

1. `VCU::init()` creates the state machines, links protections to the general fault state, sets the VCU protection ID, and initializes VCU subsystems.
2. `main()` starts ST-LIB networking, waits 4 seconds, then calls `VCU::start()` and enables state-machine checks.

After startup, the main loop calls the ST-LIB update path and then `VCU::update()`.

The H10 `VCU::update()` loop performs this work:

1. If the delayed start flag is enabled, check general and operational state transitions.
2. Process communication order flags.
3. Read sensors when the cyclic scheduler requested it.
4. Send telemetry when the cyclic scheduler requested it.
5. Check protections.

## General State Machine

The general state machine represents whether the VCU is connected, operational, or faulted.

| State | Meaning | Entry behavior |
| --- | --- | --- |
| `Connecting` | The VCU is waiting for required network peers. | Operational LED blinks. |
| `Operational` | Required peers are connected and the nested operational state machine is active. | Operational LED is solid. |
| `Fault` | A safety condition, connection loss, emergency order, or timeout occurred. | Fault LED blinks and brake is scheduled after 100 ms. |

For H11, keep the behavior but do not model `Fault` as an application state. Fault transitions belong to the ST-LIB fault infrastructure.

### General Transitions

| From | To | Condition in H10 |
| --- | --- | --- |
| `Connecting` | `Operational` | Control station TCP, HVSCU TCP, and PCU TCP are connected. |
| `Operational` | `Fault` | Control station, HVSCU, or PCU TCP disconnects. |
| `Connecting` or `Operational` | `Fault` | All monitored brake reeds are active after the first brake sequence has occurred. |
| `Operational` | `Fault` | SDC input is false/open. |
| `Operational` | `Fault` | Emergency tape input trips while the H10 tape-enable status is `ON`, which is the status set by `on_Disable_tapes()` on entry to `Ready` and `Demonstration`. |

BMSL, LCU, and BCU connection checks existed as commented code in the inspected H10 snapshot. Their intent should be revisited before treating them as required H11 peers.

### General Cyclic Work

The H10 state machine schedules these actions every 100 ms in all general states:

- mark telemetry packets for sending;
- mark sensors for reading.

While connecting, it also periodically reconnects HVSCU and PCU sockets if they are disconnected. BMSL and LCU reconnect paths were present but commented.

## Operational State Machine

The operational state machine is nested under `Operational` and represents the vehicle's usable modes.

| State | Meaning |
| --- | --- |
| `Idle` | Initial state. Contactors are considered open and motion actions are not allowed. |
| `Energized` | Contactors are considered closed, but brakes are still active. |
| `Ready` | Contactors are closed and brakes are released. Motion-related commands can be accepted. |
| `Demonstration` | At least one demonstration/run action is active. |
| `Recovery` | Recovery mode requested or emergency tape condition observed from idle. |
| `EndOfRun` | Declared in H10, but no transitions were implemented in the inspected snapshot. |

### Operational Transitions

| From | To | Condition |
| --- | --- | --- |
| `Idle` | `Energized` | VCU local `contactors_closed` flag becomes true after HVSCU acknowledgement. |
| `Energized` | `Idle` | `contactors_closed` becomes false. |
| `Ready` | `Idle` | `contactors_closed` becomes false. |
| `Demonstration` | `Idle` | `contactors_closed` becomes false. |
| `Energized` | `Ready` | Brakes are not active. |
| `Ready` | `Energized` | Brakes are active. |
| `Ready` | `Demonstration` | Demonstration bitfield is nonzero. |
| `Demonstration` | `Ready` | Demonstration bitfield returns to zero. |
| `Idle` | `Recovery` | Emergency tape input is off, or a recovery order was received. |

### Operational Entry and Exit Actions

| State | Action |
| --- | --- |
| Enter `Ready` | Disable tapes in H10 naming. |
| Exit `Ready` | Enable tapes in H10 naming. |
| Enter `Demonstration` | Disable tapes in H10 naming. |
| Exit `Demonstration` | Enable tapes in H10 naming. |
| Enter `Recovery` | Enable tapes in H10 naming and set recovery status to `1`. |

The H10 tape naming is confusing because the function names, pin state, and vehicle intent are tightly coupled to old wiring. Preserve the state-machine intent, but confirm active levels on H11 hardware before implementing outputs.

## Demonstration Bitfield

H10 uses `order_demonstration_bitfield` to know whether `Demonstration` should remain active. Each remote action sets or clears one bit:

| Bit | Meaning |
| --- | --- |
| 0 | Levitation active. |
| 1 | Propulsion or motor-control action active. |
| 2 | Low-voltage charging active, declared but not implemented in checked handlers. |
| 3 | High-voltage charging active, declared but not implemented in checked handlers. |
| 4 | Horizontal levitation active, declared but not implemented in checked handlers. |
| 5 | Booster active. |
| 6 | Brake active, declared but not used in checked handlers. |
| 7 | Contactors closed, declared but not used in checked handlers. |
| 8 | End of run, declared but not used in checked handlers. |
