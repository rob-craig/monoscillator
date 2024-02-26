# monoscillator

simple monosynth for JACK

converts midi input to audio output

## compiling

```
gcc `pkg-config --cflags gtk+-3.0` monoscillator.c -o monoscillator `pkg-config --libs gtk+-3.0 jack` -lm
```

gtk 3 and JACK headers are required
