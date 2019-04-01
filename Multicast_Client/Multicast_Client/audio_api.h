#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>
#pragma comment( lib, "Winmm.lib" )


void initialize_audio_device();
void CALLBACK waveOutProc(HWAVEOUT, UINT, DWORD, DWORD, DWORD);
WAVEHDR* allocateBlocks(int size, int count);
void freeBlocks(WAVEHDR* blockArray);
void writeAudio(LPSTR data, int size);

#define AUDIO_BLOCK_SIZE	8912
#define BLOCK_COUNT			10000