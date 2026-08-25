# Threading contract

- The selected backend owns callback execution.
- Control calls on the same device are not reentrant.
- The callback must not block, sleep, allocate, perform I/O, or call start/stop/close.
- Capture queries are synchronized against the internal ring.
- Status flags are sticky until cleared with `knm_audio_clear_status()`.
- Stream counters can be queried and reset independently from device lifetime.
