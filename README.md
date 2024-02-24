# monoscillator

simple monosynth for JACK

converts midi input to audio output

TODO: avoid clicking by adding slight attack fade-in

## compiling

`gcc `pkg-config --cflags gtk+-3.0` oscillator.c -o oscillator `pkg-config --libs gtk+-3.0 jack` -lm`

gtk 3 and JACK headers are required
