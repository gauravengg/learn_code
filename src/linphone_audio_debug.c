#include <stdio.h>
#include <stdlib.h>
#include <linphone/linphonecore.h>
#include <mediastreamer2/mediastream.h>
#include <mediastreamer2/msvolume.h>

void set_playback_gain_db(AudioStream *st, float gain) {
    if (st && st->volrecv) {
        ms_filter_call_method(st->volrecv, MS_VOLUME_SET_DB_GAIN, &gain);
        printf("Playback gain increased by %.2f dB\n", gain);
    } else {
        printf("Could not apply playback gain: No active audio stream.\n");
    }
}

void check_and_fix_audio(LinphoneCall *call) {
    if (!call) {
        printf("No active call.\n");
        return;
    }

    LinphoneCore *lc = linphone_call_get_core(call);
    const LinphoneCallParams *params = linphone_call_get_current_params(call);

    // ✅ 1. Check if audio is enabled
    bool_t is_audio_enabled = linphone_call_params_audio_enabled(params);
    printf("Audio enabled: %s\n", is_audio_enabled ? "Yes" : "No");
    if (!is_audio_enabled) {
        linphone_call_enable_audio(call, TRUE);
        printf("Audio has been enabled.\n");
    }

    // ✅ 2. Verify active audio stream
    AudioStream *audio_stream = linphone_call_get_audio_stream(call);
    if (!audio_stream) {
        printf("No active audio stream! The RTP stream may not be set up correctly.\n");
    } else {
        printf("Audio stream is active.\n");
    }

    // ✅ 3. Check selected audio codec
    const LinphonePayloadType *codec = linphone_call_params_get_used_audio_codec(params);
    if (codec) {
        printf("Using audio codec: %s\n", linphone_payload_type_get_mime_type(codec));
    } else {
        printf("No audio codec selected!\n");

        // Force enable a codec
        const MSList *codecs = linphone_core_get_audio_codecs(lc);
        if (codecs) {
            linphone_core_enable_payload_type(lc, (LinphonePayloadType *)codecs->data, TRUE);
            printf("Default codec enabled.\n");
        }
    }

    // ✅ 4. Check RTP port
    int rtp_port = linphone_core_get_audio_port(lc);
    printf("RTP Port: %d\n", rtp_port);
    if (rtp_port == 0) {
        printf("Warning: RTP port is not set correctly!\n");
    }

    // ✅ 5. Check if microphone is muted
    bool_t is_muted = linphone_core_is_mic_muted(lc);
    printf("Microphone muted: %s\n", is_muted ? "Yes" : "No");
    if (is_muted) {
        linphone_core_enable_mic(lc, TRUE);
        printf("Microphone unmuted.\n");
    }

    // ✅ 6. Increase playback gain
    set_playback_gain_db(audio_stream, 6.0f);
}

int main() {
    LinphoneCore *lc;
    LinphoneCall *call;

    // Initialize Linphone Core
    LinphoneCoreVTable vtable = {0};
    lc = linphone_core_new(&vtable, NULL, NULL, NULL);
    if (!lc) {
        fprintf(stderr, "Failed to initialize Linphone core!\n");
        return -1;
    }

    printf("Linphone initialized successfully!\n");

    // Simulating a call retrieval (replace with actual call logic)
    call = linphone_core_get_current_call(lc);
    if (!call) {
        printf("No active call found.\n");
        linphone_core_destroy(lc);
        return -1;
    }

    // Check and fix audio
    check_and_fix_audio(call);

    // Cleanup
    linphone_core_destroy(lc);
    return 0;
}
