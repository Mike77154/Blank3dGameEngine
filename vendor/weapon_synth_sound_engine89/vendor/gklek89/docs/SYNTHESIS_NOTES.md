# Synthesis notes

The design follows a classic inexpensive percussion recipe:

1. excite the sound with an impulse and a tiny rebound;
2. multiply pseudo-random noise by a very short decay;
3. derive high and band energy using two one-pole states;
4. feed the transient into two short feedback delays for damped metallic modes.

No oscillator table, FFT, convolution, fractional delay, or coefficient generation is required at runtime.
