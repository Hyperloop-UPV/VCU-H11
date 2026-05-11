# VCU-H11 Agent Instructions

This repository contains the Vehicle Control Unit firmware for Hyperloop UPV. The VCU is also called the Master in project conversations.

Before making firmware changes, read [docs/ai/vcu-context.md](docs/ai/vcu-context.md). Keep that file updated when you learn durable project facts, especially about VCU responsibilities, hardware pinout, ST-LIB `Board` usage, protections, diagnostics, naming conventions, or validated build/debug workflows.

Project rules:

- Use English names for firmware concepts, variables, types, files, and comments.
- Use ST-LIB for hardware access. Do not add direct STM32 HAL peripheral access in application code unless the user explicitly asks and the ST-LIB path is not available.
- Follow the ST-LIB `Board` contract: declare compile-time device requests at namespace scope, pass them into `ST_LIB::Board<...>`, call `Board::init()`, and retrieve runtime instances with `Board::instance_of<request>()`.
- Do not model `Fault` as an application state. Fault transitions are managed by ST-LIB infrastructure. Use protections, diagnostics, `FaultController::check_transitions()`, `Board::evaluate_protections()`, and an appropriate fault policy instead.
- Keep Ethernet RMII pins owned by `ST_LIB::EthernetDomain::PINSET_H11` for the VCU board. Do not duplicate the RMII pins in `Pinout.hpp`.
- Do not reset or restore `Core/Src/Runes/generated_metadata.cpp` after builds. It is generated metadata and should remain as produced.
- Prefer `./hyper` for project workflows, for example `./hyper build main --preset board-debug --board-name VCU`.
