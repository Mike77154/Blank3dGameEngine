# Full Capacity Weapon Sequences — v1.8.0

This release adds six long-form procedural showcases. Each one follows the same listening structure:

1. one isolated reference shot;
2. a short pause;
3. loading/chambering;
4. the requested full capacity;
5. extraction/ejection and a final reload/ready action.

## Layer usage

- **Report:** `gpaah89`
- **Weapon body:** `gweaponbody89`
- **Muzzle gas:** `gmuzzlegas89`
- **Ballistic crack:** `gballisticcrack89`
- **Low-frequency weight:** `gcinemathump89`
- **Late tail:** `glatetail89`
- **Receiver resonance:** `wsoundreceiver89`
- **General mechanisms:** `chuecka89` + `gweaponfoley89`
- **Magazine / loose ammunition:** `wsoundammo89`
- **Belt and feed texture:** `wsoundbeltfeed89`
- **Shotgun pump:** `shotpumpkin89` + `chuecka89` + `gpump89` + `gklek89` + `gshotguneq89`
- **Cases and shells:** `gtinkle89`
- **Rocket impact:** `wsound_rocketblast89` with rumble/crackle/SVF/LFO
- **Post-impact debris:** `gfire89`

## Sequence counts

| Weapon | Reference | Full capacity | Ejection / cycle |
|---|---:|---:|---|
| Pistol | 1 | 20 | 21 slide cycles and 21 cases |
| Magnum | 1 | 6 | cylinder open, six manual case ejections, reload |
| Shotgun | 1 | 10 | 11 pumps and 11 shells |
| Machine gun | 1 | 40 | 41 feed cycles and 41 cases |
| Sniper rifle | 1 | 4 | 5 bolt cycles and 5 cases |
| Rocket launcher | 1 | 4 | 5 launches, 5 remote impacts, reload per round |

## Hierarchy

The mechanisms remain legible, but the pressure event stays dominant. Measured peak advantages over the strongest reload section:

- pistol: +4.09 dB;
- Magnum: +5.71 dB;
- shotgun: +9.95 dB;
- machine gun: +2.31 dB peak and +12.27 dB RMS;
- sniper: +5.71 dB;
- rocket ignition: +7.65 dB;
- rocket impact: +13.41 dB.

## Long timeline correction

The millisecond-to-frame helper now avoids 32-bit multiplication overflow by splitting seconds and remainder before multiplication. This keeps the 106-second combined showcase in chronological order.

## Validation

- strict C89 build with `-pedantic -Wall -Wextra -Werror`;
- 9 existing test suites passed;
- ASan/UBSan passed on the modified renderer;
- deterministic WAV hashes;
- zero rejected events;
- zero clipped samples in all seven new WAV files.
