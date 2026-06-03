# Protections and Faults

H11 uses ST-LIB protections, diagnostics, and `FaultPolicyNoMachine` as the
fault trigger infrastructure. The product FSM also exposes `Fault` as a formal
state and telemetry value.

## Active Fault Triggers

| Trigger | Type | Behavior |
| --- | --- | --- |
| `sdc_closed == false` | ST-LIB protection and FSM check | Enters `Fault`. |
| `brake_fault_detected == true` | ST-LIB protection and FSM check | Enters `Fault`. |
| `tapes_reached == true` | ST-LIB protection and FSM check | Enters `Fault`. |
| Control station disconnect after required peers were connected | FSM check | Enters `Fault` outside `Idle`. |
| `FAULT` order | Operator/order path | Enters `Fault` and propagates fault. |
| `Emergency stop` order | Operator/order path | Enters `Fault` and propagates fault. |
| Remote order send failure | Runtime fault path | Calls `FAULT(...)`. |

`tapes_reached` is currently a formal software signal. H11 has no tape input pin
defined in `Pinout.hpp` yet, so the protection is ready but needs a hardware or
remote-data source to set the variable.

## Warnings

| Condition | Behavior |
| --- | --- |
| `high_pressure < 50 bar` | Emit a non-fatal warning. |

The pressure warning is intentionally not a fault. The current implementation
uses the high-pressure sensor because the low-pressure sensor range is not
compatible with a 50 bar threshold.

## Fault Entry Actions

On fault entry, VCU currently:

- exposes the product FSM state as `Fault`;
- turns off operational and connecting LEDs;
- turns on the fault LED;
- disables both cooling pumps;
- disables the electrovalve;
- marks contactors open and sends a best-effort open-contactors order when the
  HVBMS socket is connected;
- engages the brake;
- synchronizes telemetry.

`open_sdc` remains a formal action only. H11 currently exposes `sdc_closed` as
an input/protection, not as a controllable output.

## Open Safety Work

- Wire the real tape/end-of-track source into `tapes_reached`.
- Confirm active levels for `brake_fault`, `sdc_closed`, and the future tape
  signal on hardware.
- Define pressure, temperature, flow, and remote-acknowledgement thresholds if
  they should become warnings or faults.
- Assign final diagnostic IDs/severities if the diagnostics backend requires
  stable IDs.
