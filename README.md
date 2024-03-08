# Keyboard Heat Map

## Problem

Maybe you're a developer and you use the keyboard excessively for vimming. Maybe you are a hardcore gamer. Either way, if the shortcuts setup is suboptimal, your wrist might start to hurt.

To address this, I created a program that combines a minimalistic keylogger and an interface to visualize its output. It will help you setup your keyboard shortcuts as even as possible. You just leave it running for a work-session or two, pass the aggregated data to the GUI and adjust your setup accordingly.

There *will be* 2 components in this repository: the keylogger and the graphical application. 

P.S.
The keylogger only collects the number and the total time for each key, without saving information about the order.

## Build

To build you need `libxi-dev, libx11-dev` installed.

`gcc x11_key_counter.c -o keylogger.out -lXi -lX11`

## Todo
- [x] Keylogger for X server
- [ ] Keylogger for Windows
- [ ] Heat map generation from csv
- [ ] GUI: create keyboard layout
- [ ] GUI: visualize the heat map

