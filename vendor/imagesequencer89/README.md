# ImageSequencer89

Fixed-capacity, renderer-agnostic image sequence playback for strict C89.

Frames may reference different logical images (loose sequence) or the same image
with different source rectangles (atlas/cell sequence). Every frame owns its
own duration, transform hints, flags and user tag. The library performs only
clock/loop selection; it never decodes, loads or draws an image.

Loop modes: once, forward, reverse, ping-pong and hold-last.
