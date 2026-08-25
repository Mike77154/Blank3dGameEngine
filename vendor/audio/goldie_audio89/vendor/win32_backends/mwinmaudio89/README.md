# mwinmaudio89

WinMM `waveOut` provider for the unmodified `maudio89` facade supplied in
`vendoraudio.zip`. It owns no sample memory. The caller must keep PCM bytes
alive until playback is stopped or completed.
