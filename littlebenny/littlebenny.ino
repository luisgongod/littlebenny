/**
 * Adapted sketch for the modified "LittleBen" module by Quinie
 * https://www.quinie.nl/
 * 
 * modified by: Luisgongod
 * https://github.com/luisgongod/littlebenny
 * 
 * There are 3 variables that can be changed: BPM (beats per minute), ppb (pulses per beat) and Gate lenght.
 * 
 * The other Jacks are a 1/2 devision of the main clock as follow:
 * Jack 7: 1/1 of the main clock pulses
 * Jack 5: 1/2 of the main clock pulses
 * Jack 3: 1/4 of the main clock pulses
 * Jack 1: 1/8 of the main clock pulses
 * Jack 8: 1/16 of the main clock pulses
 * Jack 6: 1/32 of the main clock pulses
 * Jack 4: 1/64 of the main clock pulses
 * Jack 2: 1/128 of the main clock pulses
 * 
 * Since The beat period depends on the ppb and BPM. for example:
 * BPM = 120, ppb = 4, the beat period is 1/8 of a second, at Jack 7 And 16 seconds at Jack 2.
 * 
 * The Gate lenght will depend on the beat period (period-10). with the previous example,
 * that would be 115ms
 * 
 * For the maxium settings (BPM = 240, ppb = 8), that would be 21ms
 * 
 * For a trigger-like behaviour, the gate lenght can be set to the minimum. 
 * 
 * */
