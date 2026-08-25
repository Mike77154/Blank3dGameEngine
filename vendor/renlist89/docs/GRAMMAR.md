# RenList89 grammar 0.1

Header:

`image <asset> [clip]:`

Frame entries:

- `"request"`
- `frame "request" for <milliseconds>`
- `pause <seconds>` modifies the previous frame
- `pause_ms <milliseconds>` modifies the previous frame
- `fps <integer>` changes default duration for subsequent frames

Loop directives:

- `once`
- `repeat` / `loop`
- `reverse`
- `pingpong` / `ping-pong`
- `hold` / `hold_last`

Every other non-empty `key value` or `key=value` line is preserved as an
animation property. Comments start with `#` or `;` when they are the first
non-whitespace character on a line.
