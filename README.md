# SArch32

SArch32 (StupidArch32) is a custom Instruction Set Architecture (ISA), that was created as purely educational project with no intentions to synthetise a real hardware.

The SArch32 is a 32-bit architecture with 16 registers (12 general purpose, stack pointer, return address, flags, program counter), 32-bit memory bus, supporting several classic peripherals.

This repositor contains a core project (containing all ISA-related definitions), assembler project and emulator project.

## Features

|Feature|Support|Note|
|---|---|---|
|Basic ISA|✅||
|Assembler|✅||
|Memory bus|✅||
|Emulator core|✅||
|300x200 emulated display|✅|video memory mapped to bus|
|Peripheral mappings|❌||
|Extended debugger|⏳|Run+pause|
|Debugger support for instruction decoder|❌||
|Modular emulator|❌|support for module connection|
|Configuration|❌||
|ISA documentation|❌||
|CPU exceptions|⏳|No standard flow established|
|Interrupts|⏳||
|CPU modes|❌|System and user mode|
|Paging|❌||

Legend: ✅ - full support, ⏳ - partial support, ❌ - not yet implemented

## Core

The core project defines three main modules:

* ISA - all related to instructions parsing, generating and executing
* Machine - a support for machine emulation
* SObjFile - custom object file format definitions (loaders and parsers)

## Assembler

The assembler project defines a simple assembler program, that assembles all input files, performs linkage and generates the memory object file (SObjFile).

Algorithms used and module decompositions are not the most effective ones. The design is chosen to be as simple as possible for the readers, so that they can understand the underlying principles of such software/hardware design.

## Emulator

The emulator project defines a simple emulator, that creates a machine object and uses a Qt 5 libraries to display a simple GUI.

![Emulator screenshot](misc/emulator_screenshot.png?raw=true "Screenshot of the emulator")

## License

This software is distributed under the MIT license. Please, see attached LICENSE file for more information.