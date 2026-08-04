# chuecka89 v2.4 physics/reference pass

## Goal

Keep the approved `SH -> ECKT` atom while making the `SH` slightly drier and rebuilding each preset from a short sequence of physically plausible contact events rather than one generic repeated noise gesture.

## Physical synthesis model

The library treats handling Foley as three event families:

1. **Friction / sliding contact** — colored noise with a finite attack and short tail.
2. **Constraint or latch contact** — a short broadband transient with zero attack.
3. **Structural response** — small low-mid emphasis, distortion, and a very short shared tail rather than a large room reverb.

This is a lightweight game-audio approximation of contact-sound ideas: sustained contact is noise-like, while rigid-body impacts are short acceleration/contact pulses that excite the structure.

## Approved base atom retained

```text
SH attack      30 ms
SH release     12 ms
ECKT delay     82 ms
ECKT attack     0 ms
```

The v2.4 pass does not redesign that timing. It reduces loose high-frequency air and wetness around `SH`:

- less upper-band gain in the SH EQ trajectory;
- lower chorus depth/mix;
- lower reverb feedback/mix;
- no change to ECKT placement.

## Mechanical event maps

### Pump shotgun

Rearward stroke:

```text
fore-end slide -> action bars/bolt travel -> extraction/ejection contact -> rear stop
```

Forward stroke:

```text
fore-end slide -> elevator/feed contact -> chambering drag -> lock/battery stop
```

Magazine insertion:

```text
shotshell body/rim slide -> rim passes cartridge stop -> latch snap
```

### Pistol / heavy semiautomatic action

```text
rear slide travel -> extraction/ejection contact -> rear stop
return spring travel -> strip from magazine -> chambering drag -> battery contact
```

The heavy-action preset lengthens travel and strengthens the return/battery contact rather than merely lowering pitch.

### Revolver

```text
ratchet/index micro-clicks -> chamber alignment -> cylinder stop / latch contact
```

### Grenade launcher

The current preset follows a break/slide-open handling gesture:

```text
release contact -> barrel/breech opening slide -> round insertion -> close slide -> lock
```

### Rocket launcher handling

The preset is treated as launcher preparation, not a fictional reload. The event map follows cover movement and cocking-lever motion:

```text
sight-cover slide -> lever unfold -> forward push/rotation -> rearward settle
```

### SMG and machine-gun feed textures

Repeated bursts no longer retrigger an identical atom at exact spacing. They alternate:

```text
feed/link/pawl micro-contact -> heavier bolt/contact pulse
```

with small deterministic timing variation. The machine-gun version has slower/heavier contacts and more low-mid structural weight.

## Reference-use note

Official manuals were used to identify the order and role of moving parts. Official DVIDS footage was located as qualitative visual/auditory reference for shotgun, grenade-launcher, and belt-fed handling. No external recording, waveform, or copyrighted sample is copied into the library. The browser workflow did not provide calibrated extraction or spectral measurement of those tracks, so this pass is a physically informed synthesis pass, not a sample-accurate clone.

## Validation

- ISO C89 build with `-O2 -Wall -Wextra -pedantic -std=c89`
- Q15 fixed-point path
- no `malloc`, `realloc`, `free`, `float`, or `double` in the core
- deterministic noise and deterministic micro-jitter
- smoke tests at 44.1 kHz and 48 kHz
- rendered WAV audit found no full-scale clipped samples
