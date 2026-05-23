# Purpose and Responsibilities

## Overall Goal

The Vehicle Control Unit is the master controller for the pod. Its goal is to keep the vehicle in a coherent, safe operational mode while coordinating operator commands, local VCU hardware, and remote boards.

The VCU is responsible for deciding whether a command is allowed now. When a command belongs to another board, the VCU forwards it only from valid states, waits for the remote board to report the expected result, and faults if the acknowledgement does not arrive in time.

## Responsibilities

### Vehicle Supervision

The VCU owns the top-level view of vehicle readiness:

- whether required network peers are connected;
- whether the pod is idle, energized, ready to move, in demonstration/run behavior, or in recovery;
- whether contactors are considered closed;
- whether brakes are active;
- whether SDC and emergency tape signals are acceptable;
- whether a remote command has completed.

### Command Arbitration

Incoming orders are not executed blindly. The H10 logic follows this pattern:

1. A packet callback stores requested arguments and raises a command flag.
2. The periodic VCU update checks the flag.
3. The command handler verifies the current operational state.
4. If the command is local, the VCU acts locally.
5. If the command belongs to another board, the VCU sends a remote order.
6. The VCU waits for the remote board's reported state.
7. If the expected state is observed, the flag is cleared and local VCU state is updated.
8. If the expected state is not observed before timeout, the VCU enters fault handling and propagates the fault.

### Local Hardware Control

The H10 VCU directly controlled or read:

- brake actuator command;
- brake reed sensors;
- emergency tape input and tape-enable output;
- pressure regulator command outputs;
- pressure-regulator feedback inputs;
- cooling pump outputs;
- flow inputs;
- SDC input;
- pressure sensors;
- status LEDs.

The H11 hardware differs, but these responsibilities still define the VCU behavior that should be implemented. H11 specifically does not command a pressure regulator from firmware: regulator pressure is set manually outside the VCU, and firmware only keeps the available regulator-pressure feedback as telemetry.

### Remote Board Coordination

The H10 logic treats these boards as coordinated peers:

| Board | VCU responsibility |
| --- | --- |
| Control station | Receive operator orders and send VCU telemetry. |
| HVSCU | Request contactor close/open and wait for HVSCU state acknowledgement. |
| PCU | Forward run and motor-control orders and wait for propulsion/stop state acknowledgement. |
| LCU | Forward levitation and booster orders and wait for levitation/booster state acknowledgement. |
| BMSL, BCU, BLCU | Referenced by legacy code, but not fully active in the inspected H10 snapshot. |

### Telemetry

The H10 VCU reported:

- general and operational states;
- brake reed states and combined reed status;
- flow inputs;
- H10 high, regulator, brake, and capsule pressure values;
- H11 high, low, and manual-regulator feedback pressure values;
- tape enable and emergency tape states;
- SDC state;
- selected remote-board states;
- VCU recovery status.

H11 currently has a smaller generated telemetry set. The H10 list is the broader behavior reference.

### Safety and Fault Handling

The H10 VCU faults on connectivity loss, unsafe brake/tape/SDC conditions, explicit emergency stop, and failed remote acknowledgements. On fault, it changes LED indication and schedules a brake action.

In H11, this behavior should be implemented through the ST-LIB fault, protection, diagnostics, and scheduler APIs. `Fault` should not be added as an application operational state.

## Non-Responsibilities

The VCU is not the low-level controller for every subsystem:

- HVSCU owns the physical contactor actuation.
- PCU owns propulsion and motor control.
- LCU owns levitation and booster behavior.
- The control station owns operator UI and command generation.

The VCU's role is to authorize, sequence, observe, and fail safe.
