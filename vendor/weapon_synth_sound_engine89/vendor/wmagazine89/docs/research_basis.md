# Research basis

This file records the design references used for `wmagazine89`. No external
sample is embedded in the library or its previews; all preview audio is emitted
by the C synthesizer.

## TR-style clap topology

### Roland TR-808 Service Notes, 15 June 1981

Reference copy:

https://manuals.fdiskc.com/flat/Roland%20TR-808%20Service%20Manual.pdf

Relevant service-note description:

- white noise is passed through a band-pass filter;
- the signal is applied to two VCAs with different envelopes;
- the main component uses a unique sawtooth envelope;
- a comparator-driven sequence produces three pulses over roughly 30 ms;
- a longer VCA envelope contributes the reverberant component.

Implementation consequence:

- one xorshift white-noise source;
- broad mid-focused filtering;
- three short pulses near 0, 10, and 20 ms;
- near-vertical attack and short linear decay;
- a separate short noise tail.

### Roland TR-909 Service Notes, 15 June 1984

Reference copy:

https://notebook.zoeblade.com/Downloads/Documentation/Roland/TR-909_service_notes.pdf

The measured hand-clap waveform in the service notes shows the characteristic
clustered transient rather than one continuous noise burst. The TR-909 service
notes also document a quasi-random shift-register noise generator. The library
uses a compact xorshift source for the same deterministic, sample-free role.

## Contact and modal synthesis

### van den Doel et al., Physically-based Sound Effects for Interactive Simulation and Animation

https://www.cs.ubc.ca/labs/lci/papers/docs2001/van-foleyautomatic.pdf

The paper models a vibrating solid as a bank of damped harmonic oscillators
excited by an external stimulus. Frequencies, decay rates, and gains represent
geometry, material, and contact location. It also notes that hard contacts can
contain fast micro-collisions and that scraping can be represented by a rough
noise profile exciting a resonance model.

Implementation consequence:

- four inexpensive damped wavetable modes;
- separate modal frequency, decay, and gain sets per material preset;
- short impulse clusters rather than one mathematically perfect impulse;
- noisy roughness modulation for sliding contact;
- event speed represented mainly by sequence duration.

## Firearm magazine mechanics

### Daniel Defense DD4 / DDM4 / DD5 manual

https://danieldefense.com/media/asset/d/d/dd4_ddm4_dd5_manual_1.2-single.pdf

The loading procedure distinguishes:

- extraction from the magazine well;
- upward insertion until the magazine stops and is locked by the catch;
- a downward pull to verify seating;
- bolt release as a separate mechanism.

Implementation consequence:

- remove, insert, catch, and tug-check are separate events;
- charging handle and bolt release are intentionally outside this library;
- the insert action ends in a distinct catch transient.

### Auto-Ordnance Thompson manual

https://www.auto-ordnance.com/PDF/lg_manual.pdf

The stick magazine is pushed through guide rails, the catch stud mates with a
hole in the magazine tube, and the base may need a bump to lock securely. The
drum magazine instead slides through receiver slots and contains rotor, spring,
cover, guides, and cartridges.

Implementation consequence:

- the SMG steel preset uses a longer scrape/rail component;
- seat tap is a separate action;
- the drum preset has lower modes, stronger rattle, and longer decay.

### Magpul PMAG 30 AR/M4 GEN M3 product sheet

https://magpul.com/media/wysiwyg/GIS/MAG557_PMAG_30_AR_M4_GEN_M3_GIS_01.pdf

The product sheet describes an impact- and crush-resistant polymer body and a
self-lubricating anti-tilt follower. This supports treating polymer magazines as
more strongly damped and less bright than thin metal bodies, without pretending
that every real magazine shares one exact spectrum.

## Listening references

These recordings were used only as qualitative references for sequence,
duration, and material contrast. They are not redistributed or sampled.

- Freesound, `Gun-Mag Insert.wav` by MGA95: airsoft Beretta M92A1 insertion
  https://freesound.org/people/MGA95/sounds/555754/
- Freesound, `Mag Reload 1.mp3` by Pjkasinski3: S&W 9 mm magazine insertion
  https://freesound.org/people/Pjkasinski3/sounds/171614/
- Freesound, `AR15 Inserting an Empty Magazine 1` by DrinkingWindGames
  https://freesound.org/people/DrinkingWindGames/sounds/678872/
- Freesound, `AR15 Inserting an Empty Magazine 2` by DrinkingWindGames
  https://freesound.org/people/DrinkingWindGames/sounds/678873/
- Freesound, `AKM Inserting Loaded Polymer Magazine` by DrinkingWindGames
  https://freesound.org/people/DrinkingWindGames/sounds/851748/
- Freesound, `AR Reload.wav` by Bunny_Clark: includes a dropped empty PMAG
  https://freesound.org/people/Bunny_Clark/sounds/377551/

Several listed references are replicas or mixed recordings, so they were not
used as absolute physical calibration. Official mechanism descriptions and the
contact-synthesis literature define the model; recordings provide perceptual
checks.
