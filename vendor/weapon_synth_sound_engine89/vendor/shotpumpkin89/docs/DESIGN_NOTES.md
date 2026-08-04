# shotpumpkin89 1.1 - design and research notes

## 1. Clap-derived synthesis

The Roland TR-808 hand-clap circuit is useful here as a minimal model:
filtered noise is articulated by several short attack events and a longer
noise tail. `shotpumpkin89` keeps the noise and clustered envelopes, but
compresses the pattern into a quick restrained `crrr` under each mechanical
stroke. The end of each stroke has a separate bright impact envelope.

## 2. Real action sequence

The mechanical model is deliberately split:

```text
rearward stroke
  unlock/open action
  move bolt rearward
  extract/eject
  rear endpoint impact

short hand reversal

forward stroke
  carrier/feed movement
  chambering
  bolt/locking movement
  forward endpoint impact
```

An AES forensic example shows the action as two distinct racking clusters
and shows the spent-shell pavement `clink` as a later, separate event.
Therefore shell-on-floor audio is not baked into this library.

## 3. Mechanism-informed presets

### Remington 870

Manufacturer information describes a receiver machined from solid steel
and twin action bars intended to prevent binding and twisting. The preset
therefore emphasizes low-mid receiver body while keeping rail chatter
controlled and the cycle relatively smooth.

### Mossberg 590

Manufacturer information describes twin action bars, dual extractors,
an anti-jam elevator, steel-to-steel lockup and an aluminum receiver. The
preset adds a little more high-mid activity for multiple small contacts and
slightly less deep receiver weight than the steel-bodied 870 interpretation.

### Winchester 1300 / SXP family

The official manuals describe an inertia-assisted rearward motion and a
rotary bolt whose lugs lock into the barrel extension. The preset is the
fastest in the pack, with a bright short closing edge and reduced smear.

### Ithaca Model 37

The manufacturer highlights bottom ejection and a receiver made from one
block of steel. The preset is darker and denser, with restrained top-end.
That tonal choice is an acoustic design inference, not a measured frequency
response of the firearm.

### Benelli Nova

The official manual describes action bars coupled to the bolt guide and a
rotating-head closure system. Benelli also describes the classic Nova's
steel skeletal framework overmolded with polymer. The preset uses a focused
lock edge but shorter ringing and less room tail.

## 4. Comparison targets

Published real recordings used as listening/structure references include:

- a Winchester 1300 mechanical cycling/chambering recording captured at
  96 kHz/24-bit stereo;
- multiple unprocessed Remington 870 pump recordings, including a studio
  recording made with a Rode NT1-A;
- a Remington 870 rack followed by an empty hull striking concrete;
- several short mono/stereo pump-action rack recordings.

These pages document real recording subjects and capture formats. Because
some hosts gate original downloads behind account access, version 1.1 does
not claim laboratory spectral matching. The synthesis was tuned to the
shared audible structure: broad-band scrape, clustered small contacts,
two directional strokes, asymmetric endpoint impacts, and a short local
resonance rather than a long cinematic reverb.

## 5. Six-band EQ

Five one-pole low-pass filters are run in parallel. Their differences form
six bands:

```text
b0 = lp0
b1 = lp1 - lp0
b2 = lp2 - lp1
b3 = lp3 - lp2
b4 = lp4 - lp3
b5 = input - lp4
```

Each band is multiplied by a signed Q8 gain. The result is summed and
clamped to PCM16. No coefficient tables, heap objects or floating-point
math are required.

## 6. Tiny LFO

The additional `chirr` source is another white-noise stream shaped by a
high-pass and low-pass pair. A triangle LFO in the approximate 9-14 Hz
range only changes this layer by a few percent. The purpose is not an
obvious vibrato; it is a tiny fluctuation in rail/contact texture.

## 7. Scope boundary

This is game-audio Foley synthesis. It models sound structure only and does
not provide instructions for operating, modifying or constructing a firearm.
