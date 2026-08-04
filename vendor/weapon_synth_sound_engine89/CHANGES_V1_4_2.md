# v1.4.2 — Rocket launch / impact event pairing

- Adds the `gpaah89` rocket-launcher ignition to the full firefight demo.
- Triggers `wsound_rocketblast89` later at a separate impact position.
- Adds `audio/06_rocket_launcher_launch_to_impact.wav` focused preview.
- Updates the unified event showcase to dispatch `REPORT(ROCKET_LAUNCHER)` and
  `ROCKET_BLAST` as distinct timeline events.
- No ABI break; public facade version patch bumped to 1.4.2.
