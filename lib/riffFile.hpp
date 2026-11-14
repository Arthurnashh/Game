#ifndef _RIFF_FILE_HPP
#define _RIFF_FILE_HPP

/* general waveform format structure (information common to all formats) */
struct WAVEFORMAT_STRUCT {
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
} ;

/* flags for wFormatTag field of WAVEFORMAT */
#define WAVE_FORMAT_PCM     1

#endif 
