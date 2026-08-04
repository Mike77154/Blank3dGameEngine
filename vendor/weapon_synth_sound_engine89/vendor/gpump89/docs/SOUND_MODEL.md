# Why v1 sounded synthetic

The first implementation excited several long-lived sine modes for almost every
mechanical event. Those frequencies remained individually audible, so the ear heard
a collection of pitched tones plus broadband noise.

Version 2 changes the source model:

- impacts are aperiodic, band-limited contact bursts;
- resonant tails are short and noisy rather than stable sinusoidal notes;
- slide friction is low-mid, irregular and much quieter than end-stop impacts;
- rear and forward strokes have distinct grains;
- the forward battery event has the largest low-frequency body component;
- shell ejection is optional and disabled in the default dry presets.

The mechanism is scheduled as:

unlock -> rear rail motion -> extractor -> rear stop ->
forward rail motion -> carrier/chamber -> battery -> lock

The event scheduler may be driven automatically or each stage may be triggered by
the host animation/weapon state machine.
