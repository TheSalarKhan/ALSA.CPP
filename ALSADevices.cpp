
#include "ALSADevices.hpp"

bool ALSAPCMDevice::open() {
    snd_pcm_hw_params_t *params;
    /* Open PCM device. */
    int rc = snd_pcm_open(&handle, device_name.c_str(), type, 0);
    if (rc < 0) {
        fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
        return false;
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(handle, params);

    /* Set the desired hardware parameters. */

    /* Interleaved mode */
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

    /* Two channels (stereo) */
    snd_pcm_hw_params_set_channels(handle, params, 1);

    /* 44100 bits/second sampling rate (CD quality) */
    snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, NULL);

    /* Set period size to 32 frames. */
    snd_pcm_hw_params_set_period_size_near(handle,
                                params, &frames_per_period, NULL);

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) {
        fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
        return false;
    }

    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params, &frames_per_period, NULL);
}

void ALSAPCMDevice::close() {
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
}

char* ALSAPCMDevice::allocate_buffer() {
    unsigned int size_of_one_frame = (snd_pcm_format_width(format)/8) * channels;
    return (char*) calloc(frames_per_period, size_of_one_frame);
}

unsigned int ALSAPCMDevice::get_frames_per_period() {
    return frames_per_period;
}

unsigned int ALSAPCMDevice::get_bytes_per_frame() {
    unsigned int size_of_one_frame = (snd_pcm_format_width(format)/8) * channels;
    return size_of_one_frame;
}

unsigned int ALSACaptureDevice::capture_into_buffer(char* buffer, snd_pcm_uframes_t frames_to_capture) {
    if(frames_to_capture != frames_per_period) {
        fprintf(stderr, "frames_to_read must equal frames in period <%lu>\n", frames_per_period);
        return 0;
    }

    snd_pcm_sframes_t frames_read = snd_pcm_readi(handle, buffer, frames_to_capture);

    if(frames_read == 0) {
        fprintf(stderr, "End of file.\n");
        return 0;
    }

    if(frames_read != frames_per_period) {
        fprintf(stderr, "Short read: we read <%ld> frames\n", frames_read);
        // A -ve return value means an error.
        if(frames_read < 0) {
            fprintf(stderr, "error from readi: %s\n", snd_strerror(frames_read));
            return 0;
        }
        return frames_read;
    }
}

unsigned int ALSAPlaybackDevice::play_from_buffer(char* buffer, snd_pcm_uframes_t frames_to_play) {
    if(frames_to_play != frames_per_period) {
        fprintf(stderr, "frames_to_play must equal frames in period <%lu>\n", frames_per_period);
        return 0;
    }

    bool done = false;

    snd_pcm_sframes_t frames_written = snd_pcm_writei(handle, buffer, frames_to_play);

    if (frames_written == -EPIPE) {
        /* EPIPE means underrun */
        fprintf(stderr, "underrun occurred\n");
        snd_pcm_prepare(handle);
    } else if (frames_written < 0) {
        fprintf(stderr, "error from writei: %s\n", snd_strerror(frames_written));
    }  else if (frames_written != frames_per_period) {
        fprintf(stderr, "short write, write %ld frames\n", frames_written);
    }

    return frames_written;
}