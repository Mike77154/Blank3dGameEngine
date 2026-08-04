# Preset map — v2 SH/ECKT atom

| Preset | Mechanical image | Main syllable | Timing |
|---|---|---|---|
| shotgun pump | rearward travel, then forward chamber/lock | chik-chok | two full SHEKT atoms, 156 ms apart |
| shotgun insert | shell slides into magazine and crosses its stop | shuk | one dark compact SHEKT |
| shotgun multi insert | repeated shell insertions | sheke-sheke | 128 ms spacing |
| pistol slide | hand pull plus spring return | chik-klak | two compact atoms |
| magnum heavy action | heavier moving-mass approximation | shok-klok | slower and darker |
| revolver cylinder | indexing notches and final closure | tik-tik-tik-klok | six ticks by default |
| grenade launcher | breech open, round insertion, breech close | shik-shuk-klok | three atoms |
| rocket launcher | tube, cap, or deployment hardware | shrrk-klonk | hollow low-mid profile |
| SMG feed burst | compact cyclic action | sheke-sheke | 72 ms spacing |
| machine-gun feed burst | heavier cyclic feed/action | shok-shok | 101 ms spacing |
| atom SH | isolated friction component | shhh | medium attack and short tail |
| atom ECKT | isolated closure component | eckt | zero attack and tiny decay |
| atom SHEKT | complete base atom | shekt | ECKT begins near the SH tail |

## Main customization points

```text
stroke.start_ms
stroke.sh.envelope.*
stroke.sh.eq_start_q15[6]
stroke.sh.eq_end_q15[6]
stroke.sh.eq_sweep_ms
stroke.sh.distortion_q15
stroke.eckt.envelope.*
stroke.eckt.eq_start_q15[6]
stroke.eckt.eq_end_q15[6]
stroke.eckt.eq_sweep_ms
stroke.eckt.distortion_q15
gesture.chorus_wet_q15
gesture.reverb_wet_q15
gesture.output_gain_q15
```

## Syllable controls

For a longer `SH`, raise its attack, decay, and release while keeping its upper-mid EQ bands present. For a harder `ECKT`, keep zero attack, shorten decay, raise distortion, and move energy toward the middle and low-middle bands. To change `chik` into `chok`, use the same atom with a darker EQ pair and slightly longer closure decay.
