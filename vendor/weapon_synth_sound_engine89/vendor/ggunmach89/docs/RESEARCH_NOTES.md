# Physical and acoustic research notes

## What was treated as physical

The library uses published barrel counts and nominal rates to derive two
separate clocks:

```text
cam_cycle_hz = shots_per_minute / 60
rotor_hz = cam_cycle_hz / barrel_count
```

This matters because a six-barrel system at 3000 shots per minute produces
50 chamber/firing-station events per second, while the barrel cluster itself
turns only about 8.33 revolutions per second.

Patents describing Gatling mechanisms show bolts guided longitudinally in a
rotor, with rollers/followers driven through a stationary helical cam. That
supports a repeated cam/bolt transient synchronized to the event clock,
rather than using only a smooth motor oscillator.

Manufacturer material also describes external hydraulic, electric or
pneumatic drive systems, and the M197 page specifically notes continuous
rotary motion and lower power requirements. The synthesizer therefore
models a continuously rotating machine with acceleration, drag-like
spin-down and load dip, rather than a chain of isolated sample triggers.

## What was treated as acoustic reference

Official DVIDS footage identifies recordings containing natural sound of
GAU-8 firing, impacts and shock waves. An Air Force article describes a
GAU-8 test as resembling a foghorn at a distance and emphasizes strong
percussion and structural vibration.

Those references guided these choices:

- GAU-8 preset: stronger low and low-mid body, slower envelope, less top end.
- M61 and GAU-22: stronger high mechanical whine.
- M197: more separated cam pulses because its event rate is lower.
- M134D: compact electric whine with a dense 50 Hz event texture.
- Reverb remains restrained because real recordings vary heavily with
  distance, terrain, aircraft and microphone placement.

The source videos could be identified and inspected as references, but their
media downloads were access-restricted in the research environment.
Accordingly, this package does not claim measured FFT matching, impulse
response matching or sample cloning.

## Sources

Manufacturer and official technical pages:

- Dillon Aero, M134D:
  https://dillonaero.com/m134d-standard-7-62-x-51mm/
- Dillon Aero, M134D gun pod:
  https://dillonaero.com/gun-pod-system/
- General Dynamics OTS, M61A1/M61A2:
  https://www.gd-ots.com/armaments/aircraft-guns-gun-systems/m61a1/
- General Dynamics OTS, M197:
  https://www.gd-ots.com/armaments/aircraft-guns-gun-systems/m197/
- General Dynamics OTS, GAU-8/A:
  https://www.gd-ots.com/armaments/aircraft-guns-gun-systems/gau8a/
- General Dynamics OTS, GAU-22/A:
  https://www.gd-ots.com/armaments/aircraft-guns-gun-systems/gau22a/
- National Museum of the U.S. Air Force, M61A1:
  https://www.nationalmuseum.af.mil/Visit/Museum-Exhibits/Fact-Sheets/Display/Article/579640/m61a1-vulcan-cannon/
- U.S. Air Force Flight Test Center, GAU-8 testing:
  https://www.aftc.af.mil/About-Us/History/On-This-Day-in-Test-History/Article-Display-Test-History/Article/2482833/february-26-1974-live-firing-tests-of-a-30mm-gau-8-cannon-mounted-in-a-10/
- Eglin Air Force Base, GAU-8 acoustic description:
  https://www.eglin.af.mil/News/Article-Display/Article/392753/testing-titanic-a-10-cannon/

Mechanism patents:

- US4166407A, drive mechanism for a Gatling gun:
  https://patents.google.com/patent/US4166407A/en
- US4359927A, high-rate revolving battery gun:
  https://patents.google.com/patent/US4359927A/en
- US4481859A, Gatling gun control system:
  https://patents.google.com/patent/US4481859A/en
- US10871336B1, electronically controlled Gatling subsystems:
  https://patents.google.com/patent/US10871336B1/en

Official audiovisual references:

- DVIDS, TACP Pilsung Range Training:
  https://www.dvidshub.net/video/781028/tacp-pilsung-range-training
- DVIDS, Tank Buster: A-10 Thunderbolt II:
  https://www.dvidshub.net/video/506773/tank-buster-10-thunderbolt-ii
- DVIDS, A-10 Thunderbolt II Gatling Gun Maintenance:
  https://www.dvidshub.net/video/895528/10-thunderbolt-ii-gatling-gun-maintenance

## Parameter disclaimer

Spin-up/down times and all timbral values are game-audio presets. They are
not certified weapon measurements and should be tuned to the camera
distance, mix, environment and other synthetic layers in the host engine.
