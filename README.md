# virtualization-techniques-tum
Homework 2 of virtualization techniques at tum for winter semester 22-23

# files
`x86_gen.c` is the file to map the guest instruction without `SETL` and `BACK7`. It works with `gdb` and `disassemble /r` command to show the correct x86 instruction mappings.

`gen.c` is the library function to generate the guest instructions.

`main.c` contains all the interpreters. Compile it and run in the way described in [usage section](#usage).

# usage
The program runs by default with 50000 instructions distributed in `1-9-1-5-5(CLRA-INC3A-DECA-SETL-BACK7)` for 5 times.

You can change the input scenario in this way `./main 50000 5 1-1-1-0-0`.

Remember for x86 instruction stream interpreter, it can only handle `HALT, CLRA, INC3A and DECA`. So, the instruction should be distributed in `x-x-x-0-0`