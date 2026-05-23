# Protections and Faults

## H10 Fault Model

The H10 code uses two paths for fault behavior:

- state-machine transitions into a general `Fault` state;
- direct calls to `ProtectionManager::fault_and_propagate()`.

On fault-state entry, H10:

1. switches LED behavior to fault blinking;
2. schedules a brake command after 100 ms.

H11 should keep the same safety intent but route it through ST-LIB fault infrastructure, protections, diagnostics, and `FaultPolicyNoMachine`.

## Fault Triggers

| Trigger | H10 source behavior | Intended meaning |
| --- | --- | --- |
| Required TCP peer disconnects | `Operational -> Fault` if control station, HVSCU, or PCU TCP is disconnected. | The VCU cannot safely coordinate without required peers. |
| Required peer never connects | `Connecting` remains active until required peers connect. | The VCU should not become operational without minimum communications. |
| Brake reed condition | `Connecting` or `Operational -> Fault` when all monitored reeds are active after first brake sequence. | Brake/reed safety condition requiring confirmation before H11 implementation. |
| SDC opens | `Operational -> Fault` when SDC is false. | Safety chain opened. |
| Emergency tape condition | `Operational -> Fault` when tape emergency trips while the H10 tape-enable status is `ON`, which is the status set by `on_Disable_tapes()` on entry to `Ready` and `Demonstration`. | Tape-related emergency during active vehicle mode. |
| Emergency stop order | Order callback calls `fault_and_propagate()`. | Operator or upstream emergency request. |
| Remote acknowledgement timeout | Order handlers call `fault_and_propagate()` if expected remote state does not arrive. | A commanded remote subsystem did not reach requested state. |
| Standard protections | H10 calls `ProtectionManager::add_standard_protections()` and checks protections every loop. | Infrastructure-level protections still matter even outside explicit state-machine transitions. |

## Fault Propagation

The H10 code uses fault propagation for:

- emergency stop;
- contactor close/open acknowledgement failures;
- propulsion and motor-control acknowledgement failures;
- levitation and booster acknowledgement failures.

This means a failed command is not merely rejected. If the VCU accepted a command and sent it to a remote board, the expected acknowledgement becomes a safety condition.

## Safe Output Behavior

The only clearly implemented H10 fault action is braking:

- entering fault schedules `brakes->brake()` after 100 ms;
- local brake order turns regulator 1 command to `0`;
- unbrake requires regulator 1 command to be raised to `6` and waits 2 seconds before releasing the brake.

The regulator-related brake behavior is H10-only. H11 uses a manual regulator that is not firmware-controlled, so H11 fault and brake handling must not attempt to drive regulator pressure or duty.

For H11, confirm and document:

- safe default for each output;
- brake active level;
- cooling pump safe state;
- electrovalve safe state;
- fault LED policy;
- whether SDC opening should trigger immediate brake, diagnostic, propagated fault, or all of these.

## Open Safety Questions From H10

These are not safe to copy without review:

- The brake reed fault condition was marked with a "check this" comment in H10.
- The H10 code declared eight reed pins but read four.
- Some remote acknowledgement timeouts are only 100 ms, which may be too short for real Ethernet and actuator response.
- Close contactors has a 6000 ms timeout while open contactors has a 100 ms timeout.
- SDC faulting from `Connecting` was present but commented out.
- LCU and BCU command logic exists, but socket creation for those boards was commented out in the inspected snapshot.
- Tape-enable function names and cached pin states are confusing and should be revalidated against H11 wiring.

## H11 Rule

Do not add `Fault` to the H11 operational state enum. Represent normal operating modes in the application state machine and let ST-LIB fault infrastructure own fault transitions, diagnostics, and fault policy actions.
