# Shotgun Pump Master — Physics and Listening Notes (v1.6.2)

## Scope

This release masters only the pump-action mechanical bus. It does not EQ the
shot report, muzzle blast, ballistic crack, room or exterior acoustics.

Signal order:

```text
gpump89 structural stages
+ chuecka89 bright articulated contacts
+ shotpumpkin89 rail friction/chatter
+ gklek89 carrier/locking articulation
+ optional external tek (or built-in Foley tik fallback)
        -> gshotguneq89 six-band master
        -> gshotgunsequence89 master gain
```

## Physical references

The profiles are physically informed perceptual tunings, not claims that a
specific serial-numbered firearm has an invariant spectrum. Lubrication, wear,
forend furniture, handling speed, microphone, room and ammunition can change
the recording substantially.

### Remington 870

The official manual shows paired action bars/rails, bolt/slide assembly and
carrier involvement. The profile therefore keeps the continuous rail texture
controlled and emphasizes a compact low-mid structural closure.

Reference:
- RemArms Model 870 Owner's Manual:
  https://www.remarms.com/sites/default/files/documents/870%20Manual%20Final.pdf

### Mossberg 500/590

Mossberg documents non-binding twin action bars, steel-to-steel lockup,
dual extractors and an anti-jam elevator, while the receiver is anodized
aluminum. The profile exposes more separated upper-mid contacts, carrier detail
and worn-service chatter without removing the structural body.

References:
- Mossberg 500/590 operation animation:
  https://resources.mossberg.com/journal/how-mossberg-500-590-pump-action-shotguns-work
- Mossberg loading/unloading operation video:
  https://resources.mossberg.com/journal/mossberg-500-590-series-shotguns-loading-and-unloading

### Benelli Nova

Benelli describes a steel skeletal framework overmolded with polymer. The
manual shows rearward fore-end movement dropping a cartridge to the carrier and
forward travel pushing it into the chamber and locking the bolt. The profile
retains strong internal mechanical mass but reduces exposed high-frequency ring.

References:
- Benelli Nova product/manual material:
  https://www.benelliusa.com/sites/default/files/content/media/manuals/2019-10/Nova%20Pump%20Field%20Shotgun%20Product%20Manual.pdf
- Benelli Nova construction overview:
  https://www.benelliusa.com/resources/benelli-adds-a-dynamic-duo-to-the-nova-pump-line

### Winchester SXP

Winchester documents recoil-inertia assistance at the beginning of rearward
travel and a rotary bolt whose lugs engage the barrel extension. The profile
uses the shortest cycle in this release, controlled rail noise and a dry,
high-definition terminal lock articulation.

Reference:
- Winchester SXP Owner's Manual:
  https://www.winchesterguns.com/support/owners-manuals/sxp-shotgun.html

## Real-operation listening

Manufacturer operation videos and manuals were used as qualitative sequence and
mechanism references. This environment did not ingest their audio for numerical
spectral matching. No third-party recording was embedded, copied or
spectrally cloned. The generated audio remains fully procedural and CC0.

## Six master bands

Approximate crossovers at 44.1 kHz:

```text
0: below 120 Hz        structural thump
1: 120–320 Hz          receiver/forend mass
2: 320–900 Hz          rails and carrier body
3: 900–2400 Hz         bolt and shell articulation
4: 2.4–6 kHz           extractor/ejector/lock definition
5: above 6 kHz         chatter and sharp metal edge
```

A transient path preserves short contacts. Soft drive adds density before a
saturating output stage, but all shipped previews remain below PCM16 clipping.

## gklek89 and optional tek

`gklek89` was already part of the pump sequence and remains mandatory. It is
retimed per model to represent carrier/rear-stop/lock articulation.

No independent `tek` library was present in the supplied v1.6.1 package. v1.6.2
therefore exposes three optional callbacks:

```c
gss89_set_tek_provider(ctx, trigger, process, active, user);
```

When a provider is connected it replaces the small built-in Foley particle.
Without one, behavior remains deterministic and backward compatible.
