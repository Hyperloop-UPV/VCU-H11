# Local Subsystems

## Actuators

The H10 `Actuators` class groups local non-brake hardware: pressure regulators, pumps, flow inputs, SDC input, and pressure sensors.

### Pressure Regulator

H10 had two regulator command outputs and two regulator feedback inputs.

- Each regulator command used PWM.
- PWM frequency was set to 30 kHz.
- Requested pressure was clamped to maximum `6`.
- Pressure command was converted to duty with `pressure * (100 / 6)`.
- Regulator pressure commands were included in telemetry.

H11 intentionally does not control regulator pressure from firmware. A manual regulator will be used instead, so the H11 firmware must not request the old regulator PWM pin, expose a set-regulator order, or publish a commanded-regulator duty. The available regulator feedback ADC may still be read and published so operators can observe the manually set pressure.

### Cooling Pumps

H10 exposed two pump outputs:

- pump 1 represented one selected cooling circuit;
- pump 2 represented the other selected cooling circuit;
- the old code treated duty `100` as on and all other duty values as off.

The packet name `Potencia_refri` should be translated to an English firmware concept such as cooling pump power or cooling pump command.

### Flow Inputs

H10 read two flow inputs as digital sensors. H11 pinout assigns the flow sensors to timer input-capture channels, so H11 should model them as measured pulse/frequency sources rather than simple digital states once the ST-LIB abstraction is selected.

### SDC Input

The SDC signal is a safety-critical input. In H10, the general state machine faults from `Operational` when the SDC value is false/open.

H11 already models SDC through an EXTI-backed interrupt/sensor path. Preserve the behavior that an open SDC is not compatible with normal operation.

### Pressure Sensors

H10 tracked four pressure values:

| Value | H10 calibration note |
| --- | --- |
| High pressure | Linear gain `0.00763`, offset `0.318`. |
| Regulator pressure | Linear gain `0.256`, offset `0.375`. |
| Brake pressure | Linear gain `0.256`, offset `0.375`. |
| Capsule pressure | Declared as uncharacterized and not read in the inspected H10 loop. |

H11 currently tracks high pressure, low pressure, and manual-regulator feedback. Final calibration and units remain open decisions.

## Brakes

The H10 `Brakes` class owns the local brake actuator, reed sensors, emergency tape input, and tape-enable output.

### Brake Actuator

H10 behavior:

- startup initializes the brake actuator output off and considers brakes active;
- `brake()` turns the actuator output off and sets `Active_brakes = true`;
- `unbrake()` turns the actuator output on and sets `Active_brakes = false`;
- the first brake command clears a `breaks_first_time` flag used by fault logic.

The active level must be confirmed on H11 before implementing. The important behavior is that fault handling commands the brake into the safe braking condition.

### Reed Sensors

H10 pinout declared eight reed pins, but the inspected brake code reads four:

- reed 1;
- reed 2;
- reed 3;
- reed 4.

It computes `All_reeds` as the logical AND of the four read values. The general state machine faults when `All_reeds` is true after the first brake sequence has occurred. The safety intent of this condition should be confirmed before porting because the old code includes a warning comment near that transition.

### Emergency Tape and Tape Enable

H10 has:

- one emergency tape input;
- one tape-enable output;
- a cached tape-enable status used by the state machine.

The operational state machine changes tape behavior:

- entering `Ready` or `Demonstration` disables tapes in H10 naming;
- exiting `Ready` or `Demonstration` enables tapes;
- entering `Recovery` enables tapes.

The H10 function names and pin states are legacy wiring concepts. Porting should start from the intended safety behavior, not the old active-level names.

## LEDs

The H10 LED logic is simple vehicle-status feedback:

| Mode | LED behavior |
| --- | --- |
| Connecting | Operational LED toggles every 500 ms. |
| Operational | Operational LED is solid on. |
| Fault | Fault LED toggles every 500 ms. |

Other LEDs were declared but not meaningfully used in the inspected H10 logic.

H11 has separate sleep, CAN, connecting, fault, and operational LED aliases. Preserve the state indication intent with the H11 LED names.
