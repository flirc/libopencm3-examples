# README

This example shows how to implement an interactive shell using the UART.

Commands are added with macros using a necessary addition to the linker file,
which creates a special section for our commands, keeping them in an
aalphebetically ordered list.

To define a command, we need only four items:

* String Name: representing our command.
* Function Pointer: Function called when typing command string
* Short Help: Shown when typing help by itself
* Long Help: Shown when typing: help `command`

# Requirements

To use the shell, include <libopencm3/ushell/ushell.h> and call cmd_loop() in
the main loop. The loop is non blocking, and all commands registered should be
non-blocking.

Ensure CFLAGS include -nodefaultlibs -ffreestanding. A small footprint printf
is included in shell. Register your putc and getc functions. Should these CFLAGS
not be included, system printf will be linked, which can bloat your firmware,
and may not use your correct IO interface.

# Configuration Options

## USHELL_MAX_ARGC

These are the maximum amount of arguments for any given command. The default is
10 if not defined.

## USHELL_LINEBUF_SIZE

This is the total length of the shell line. The default character amount if not defined is 512.

## CONSOLE_HISTORY_LEN

This is the history of the entered commands. Pressing up on the keyboard will
go back in time to previously entered commands. If not defined, the history
length is 10 lines.

A great convenience, but eats RAM. 10 lines of 512 bytes is 5120 bytes.

Disable this in the library make file, as this is enabled by default.
