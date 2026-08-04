#ifndef B3D_TEST_MMSYSTEM_H
#define B3D_TEST_MMSYSTEM_H
#include <windows.h>

typedef HANDLE HWAVEOUT;
typedef UINT MMRESULT;

typedef struct tWAVEFORMATEX {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
} WAVEFORMATEX;

typedef struct wavehdr_tag {
    LPSTR lpData;
    DWORD dwBufferLength;
    DWORD dwBytesRecorded;
    DWORD_PTR dwUser;
    DWORD dwFlags;
    DWORD dwLoops;
    struct wavehdr_tag *lpNext;
    DWORD_PTR reserved;
} WAVEHDR;

#define WAVE_FORMAT_PCM 1U
#define WAVE_MAPPER ((UINT)-1)
#define CALLBACK_NULL 0UL
#define MMSYSERR_NOERROR 0U
#define WHDR_DONE 0x00000001UL
#define WHDR_PREPARED 0x00000002UL

MMRESULT waveOutOpen(HWAVEOUT *, UINT, const WAVEFORMATEX *, DWORD_PTR, DWORD_PTR, DWORD);
MMRESULT waveOutPrepareHeader(HWAVEOUT, WAVEHDR *, UINT);
MMRESULT waveOutWrite(HWAVEOUT, WAVEHDR *, UINT);
MMRESULT waveOutReset(HWAVEOUT);
MMRESULT waveOutUnprepareHeader(HWAVEOUT, WAVEHDR *, UINT);
MMRESULT waveOutClose(HWAVEOUT);

#endif
