# ch8asm

## Overview
Simple Assembler for the Chip 8 Instruction set.
It supports varriable assignment for binary literals, hex literals using ``$`` prefixes and labels (syntax ``label:``) for use in jump or call instructions.

I tried sticking to [this](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM) reference, but renamed some instructions (as I was annoyed by LD being used so often).

The architecture and code here is very much inspired by Robert Nystrom's excellent book [Crafting Interpreters](https://craftinginterpreters.com/) (you may notice some common function names :)).

I tried using "C++-light", i.e. only using some C++ features when it was convenient but trying to stick to C.

I've included tests for all instructions, you can run a test script that tests all instructions using ``make test``.

To compile, create a folder named ``build`` and run ``make all -j`` to compile the executable ``ch8asm.x``.

## Modified Instruction Table

For a detailed explanation what each instruction does see [here](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM).

| Opcode  | Instruction |
|---|---|
| 00E0 | CLS |
| 00EE | RET |
| 1nnn  |  JP addr |
| 2nnn | CALL addr |
| 3xkk | SE Vx, byte |
| 4xkk | SNE Vx, byte |
| 5xy0 | SE Vx, Vy |
| 6xkk | LD Vx, byte |
| 7xkk | ADD Vx, byte |
| 8xy0 | LD Vx, Vy |
| 8xy1 | OR Vx, Vy |
| 8xy2 | AND Vx, Vy |
| 8xy3 | XOR Vx, Vy |
| 8xy4 | ADD Vx, Vy |
| 8xy5 | SUB Vx, Vy |
| 8xy6 | SHR Vx{, Vy} |
| 8xy7 | SUBN Vx, Vy |
| 8xyE | SHL Vx{, Vy} |
| 9xy0 | SNE Vx, Vy |
| Annn | LD I, addr |
| Bnnn | JPO addr |
| Cxkk | RND Vx, byte |
| Dxyn | DRW Vx, Vy, nibble |
| Ex9E | SKP Vx |
| ExA1 | SKNP Vx |
| Fx07 | GDT Vx |
| Fx0A | WKP Vx |
| Fx15 | SDT Vx |
| Fx18 | SST Vx |
| Fx1E | ADD I, Vx |
| Fx29 | FNT Vx |
| Fx33 | BCD Vx |
| Fx55 | STV Vx |
| Fx65 | LDV Vx |


