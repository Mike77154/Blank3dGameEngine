# Real-reference calibration 1.6

Version 1.6 changes no source family: three seeded white-noise streams remain the
only excitation. The revision calibrates mechanism order and decay behavior.

## Reference logic

- Beretta APX A1 documentation describes trigger-bar/cocking-lever movement,
  striker release, forward travel and striker-return-spring rebound. That maps
  to release, main stop and weak rebound contacts.
- The RemArms Model 700 manual separates bolt-handle lift, rear travel, forward
  travel and handle-down locking. The bolt preset preserves those direction
  changes and exposes slow/normal/fast timing.
- Heckler & Koch describes the MP5 as closed-bolt with roller-delayed blowback.
  The dry mechanism remains compact but receives more body than a pistol.
- Mossberg pump-action documentation separates the manual safety and action
  controls. The shotgun dry-fire and safety remain separate presets.
- Marine Corps M72 documentation lists transport safety, cocking lever, covers,
  sights and the tubular disposable weapon body. Launcher handling therefore
  uses small controls followed by darker tubular contacts.
- Public real-gun recordings were used only to study relative timing, contact
  count and room contamination. No recording or sample is included in the code
  or generated WAVs.

## Runtime variation

For the same seed, preset, variant and speed, output is bit-repeatable.

Three variants apply bounded changes:

```text
contact offset  roughly +/- 1.5 ms
peak/sustain    96% .. 104%
cutoff          95% .. 105%
release         90% .. 110%
metal body      about 94% .. 106%
```

Offsets are clamped to preserve mechanism order.

## Per-contact body

Each contact owns a four-mode inharmonic delay bank. Body profiles distinguish:

- edge / detent / safety: short and bright;
- striker / lock / stop: medium body;
- hammer / frame: heavier and darker;
- rail: low wet level and short feedback;
- tube: highest wet level and longest dark decay.

## Room separation

`gwf89_process_sample_stems()` returns:

- `dry`: mechanical path after chorus;
- `room`: wet-only small-room return;
- function return: dry plus room scaled by `room_send_q15`.

## Sources consulted

- Beretta APX A1 manual:
  https://www.beretta.com/content/dam/beretta-usa/user-manuals/APX_A1_Manual.pdf
- RemArms Model 700 manuals:
  https://www.remarms.com/support/owners-manuals-%28remarms%29
- Heckler & Koch MP5 product documentation:
  https://www.heckler-koch.com/en/Products/Military%20and%20Law%20Enforcement/Submachine%20guns/MP5
- Mossberg pump-action manual:
  https://resources.mossberg.com/hubfs/manuals/12173-Owners-Manual-Pumps-English.pdf
- USMC Squad Weapons / M72 LAW:
  https://www.trngcmd.marines.mil/Portals/207/Docs/TBS/B2E2657%20Squad%20Weapons.pdf
- Bersa BP9CC real dry-fire recording metadata:
  https://freesound.org/s/467183/
- Rigid-body modal resonator research:
  https://arxiv.org/abs/2210.15306
