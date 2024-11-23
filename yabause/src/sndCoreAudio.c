/*  Copyright 2005-2010 Lawrence Sebald
    Copyright 2005-2006 Theo Berkau
 
    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

/* This file is adapted from CrabEmu's sound.c for Mac OS X as well as the
   sndsdl.c file in Yabause. */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "scsp.h"
#include "sndCoreAudio.h"

#define BUFFER_LEN (65536)

static int SNDCoreAudioInit(void);
static void SNDCoreAudioDeInit(void);
static int SNDCoreAudioReset(void);
static int SNDCoreAudioChangeVideoFormat(int vfreq);
static void SNDCoreAudioUpdateAudio(u32 *left, u32 *right, u32 cnt);
static u32 SNDCoreAudioGetAudioSpace(void);
static void SNDCoreAudioMuteAudio(void);
static void SNDCoreAudioUnMuteAudio(void);
static void SNDCoreAudioSetVolume(int volume);
#ifdef USE_SCSPMIDI
int SNDCoreAudioMidiChangePorts(int inport, int outport);
u8 SNDCoreAudioMidiIn(int *isdata);
int SNDCoreAudioMidiOut(u8 data);
#endif

SoundInterface_struct SNDCoreAudio = {
    SNDCORE_COREAUDIO,
    "Mac OS X Core Audio Sound Interface",
    &SNDCoreAudioInit,
    &SNDCoreAudioDeInit,
    &SNDCoreAudioReset,
    &SNDCoreAudioChangeVideoFormat,
    &SNDCoreAudioUpdateAudio,
    &SNDCoreAudioGetAudioSpace,
    &SNDCoreAudioMuteAudio,
    &SNDCoreAudioUnMuteAudio,
    &SNDCoreAudioSetVolume,
#ifdef USE_SCSPMIDI
    &SNDCoreAudioMidiChangePorts,
    &SNDCoreAudioMidiIn,
    &SNDCoreAudioMidiOut
#endif
};

static unsigned char buffer[BUFFER_LEN];
static int muted = 1;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static u32 read_pos = 0, write_pos = 0;
static int soundvolume = 100;

/*
static OSStatus SNDCoreAudioMixAudio(void *inRefCon,
                               AudioUnitRenderActionFlags *ioActionFlags,
                               const AudioTimeStamp *inTimeStamp,
                               UInt32 inBusNumber,
                               UInt32 inNumFrames,
                               AudioBufferList *ioData) {
    UInt32 len = ioData->mBuffers[0].mDataByteSize;
    void *ptr = ioData->mBuffers[0].mData;

    pthread_mutex_lock(&mutex);

    if(muted || (read_pos + len > write_pos && write_pos > read_pos)) {
        memset(ptr, 0, len);
    }
    else {
        memcpy(ptr, buffer + read_pos, len);

        read_pos += len;
        read_pos &= (BUFFER_LEN - 1);
    }

    pthread_mutex_unlock(&mutex);

    return noErr;
}
*/

int NativeMix(short *audio, int num_samples){

    u32 len = num_samples * sizeof(short) * 2;
    pthread_mutex_lock(&mutex);

    if(muted || (read_pos + len > write_pos && write_pos > read_pos)) {
        memset(audio, 0, len);
    }
    else {

        
        if( (read_pos+len) <  BUFFER_LEN ) {
            memcpy(audio, buffer + read_pos, len);

        }else{
            int tmplen = BUFFER_LEN - read_pos;
            int nextle = len - tmplen;

            char * p = audio;

            memcpy(p, buffer + read_pos, tmplen);
            memcpy(p + tmplen , buffer, nextle);

        }

        read_pos += len;
        read_pos &= (BUFFER_LEN - 1);

    }

    pthread_mutex_unlock(&mutex);

    return num_samples;
}


int SNDCoreAudioInit(void) {
    int rv = 0;

    /* Clear the sound to silence. */
    memset(buffer, 0, BUFFER_LEN);
    muted = 0;
    soundvolume = 100;
    return 0;
}

static void SNDCoreAudioDeInit(void) {
    return;
}

static int SNDCoreAudioReset(void) {
    /* NOP */
    return 0;
}

static int SNDCoreAudioChangeVideoFormat(int vfreq) {
    /* NOP */
    return 0;
}

static void macConvert32uto16s(s32 *srcL, s32 *srcR, s16 *dst, u32 len) {
    u32 i;
    
    for(i = 0; i < len; i++) {
        // Left Channel
        *srcL = (*srcL * soundvolume) / 100;
        if (*srcL > 0x7FFF) *dst = 0x7FFF;
        else if (*srcL < -0x8000) *dst = -0x8000;
        else *dst = *srcL;
        srcL++;
        dst++;
        // Right Channel
        *srcR = (*srcR * soundvolume) / 100;
        if (*srcR > 0x7FFF) *dst = 0x7FFF;
        else if (*srcR < -0x8000) *dst = -0x8000;
        else *dst = *srcR;
        srcR++;
        dst++;
    } 
}

static void SNDCoreAudioUpdateAudio(u32 *left, u32 *right, u32 cnt) {
    u32 copy1size=0, copy2size=0;

    pthread_mutex_lock(&mutex);

    if((BUFFER_LEN - write_pos) < (cnt << 2)) {
        copy1size = (BUFFER_LEN - write_pos);
        copy2size = (cnt << 2) - copy1size;
    }
    else {
        copy1size = (cnt << 2);
        copy2size = 0;
    }

    macConvert32uto16s((s32 *)left, (s32 *)right,
                       (s16 *)(((u8 *)buffer) + write_pos),
                       copy1size >> 2);

    if(copy2size) {
        macConvert32uto16s((s32 *)left + (copy1size >> 2),
                           (s32 *)right + (copy1size >> 2),
                           (s16 *)buffer, copy2size >> 2);
    }

    write_pos += copy1size + copy2size;   
    write_pos %= (BUFFER_LEN);

    pthread_mutex_unlock(&mutex);
}

static u32 SNDCoreAudioGetAudioSpace(void) {
    u32 fs = 0;

    if(write_pos > read_pos) {
        fs = BUFFER_LEN - write_pos + read_pos;
    }
    else {
        fs = read_pos - write_pos;
    }
    
    return (fs >> 2);
}

static void SNDCoreAudioMuteAudio(void) {
    muted = 1;
}

static void SNDCoreAudioUnMuteAudio(void) {
    muted = 0;
}

static void SNDCoreAudioSetVolume(int volume) {
    soundvolume = volume;
}

#ifdef USE_SCSPMIDI
int SNDCoreAudioMidiChangePorts(int inport, int outport) {
	return 0;
}

u8 SNDCoreAudioMidiIn(int *isdata) {
	*isdata = 0;
	return 0;
}

int SNDCoreAudioMidiOut(u8 data) {
	return 1;
}
#endif
