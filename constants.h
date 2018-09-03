#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <alsa/asoundlib.h>
#include </usr/include/alsa/pcm.h>

const snd_pcm_format_t FORMAT = SND_PCM_FORMAT_S16_BE;
#define SAMPLING_RATE 16000
#define CHANNELS 1
#define FRAMES_PER_PERIOD 64

#endif
