#include <mmsystem.h>

MMRESULT waveOutOpen(HWAVEOUT *out, UINT device, const WAVEFORMATEX *format,
                     DWORD_PTR callback, DWORD_PTR instance, DWORD flags)
{
    (void)device; (void)format; (void)callback; (void)instance; (void)flags;
    if (!out) return 1U;
    *out = (HWAVEOUT)1;
    return MMSYSERR_NOERROR;
}

MMRESULT waveOutPrepareHeader(HWAVEOUT out, WAVEHDR *header, UINT size)
{
    (void)out; (void)size;
    if (!header) return 1U;
    header->dwFlags |= WHDR_PREPARED | WHDR_DONE;
    return MMSYSERR_NOERROR;
}

MMRESULT waveOutWrite(HWAVEOUT out, WAVEHDR *header, UINT size)
{
    (void)out; (void)size;
    if (!header || !header->lpData || header->dwBufferLength == 0U) return 1U;
    header->dwFlags |= WHDR_PREPARED | WHDR_DONE;
    return MMSYSERR_NOERROR;
}

MMRESULT waveOutReset(HWAVEOUT out) { (void)out; return MMSYSERR_NOERROR; }
MMRESULT waveOutUnprepareHeader(HWAVEOUT out, WAVEHDR *header, UINT size)
{
    (void)out; (void)size;
    if (header) header->dwFlags = 0U;
    return MMSYSERR_NOERROR;
}
MMRESULT waveOutClose(HWAVEOUT out) { (void)out; return MMSYSERR_NOERROR; }
