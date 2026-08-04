# Heavy provider pool

A 256-voice logical manager should not automatically imply 256 copies of a large synthesizer.

Use a small caller-owned pool of physical DSP states:

1. Each logical event stores a lightweight recipe, elapsed frame and seed.
2. `physical_state_changed(user, 1)` acquires a free DSP state and reconstructs or seeks it to the logical elapsed frame.
3. `process_mono` renders through that bound state.
4. `physical_state_changed(user, 0)` writes back any required state and releases the DSP state.
5. `advance_frames` advances the lightweight recipe while virtual.

This lets 256 logical weapon events share, for example, 64 full synthesis contexts without heap allocation.
