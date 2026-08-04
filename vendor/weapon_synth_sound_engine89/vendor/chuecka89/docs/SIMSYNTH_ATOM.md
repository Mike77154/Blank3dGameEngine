# SimSynth preset translation

The reference preset is interpreted as two separate triggered noise voices.

## Voice 1 — SH

```text
source       white noise
attack       clearly audible, medium for a short one-shot
body         short decay with a low residual level
release      small tail
role         scrape, hand movement, shell or forearm friction
processing   distortion, moving six-band EQ, light chorus
```

## Voice 2 — ECKT

```text
source       white noise
attack       zero
body         very short decay
release      almost zero
start        placed after SH has already developed
role         stop, latch, bolt contact, rim crossing a retainer
processing   stronger distortion, rapidly darkening six-band EQ
```

## Combined atom

```text
SHHHHHH
      ECKT
```

The closure may overlap the last part of SH, but it must not begin at the same time as SH. Repeating the combined atom with different EQ weight produces `chik-chok`, `shuk`, and `sheke-sheke` families without stored recordings.
