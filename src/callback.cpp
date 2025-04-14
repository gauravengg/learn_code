#include "linphone/core.h"
#include <signal.h>
#include <mediastreamer2/msfilter.h>

static bool_t running = TRUE;

static void stop(int signum) {
    running = FALSE;
}

static void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg) {
    switch (cstate) {
        case LinphoneCallConnected:
            printf("Call connected! Starting playback...\n");
            {
                const char *wav_file = "recorded_audio.wav"; // Change to your file path
                MSFactory *ms_factory = linphone_core_get_ms_factory(lc);
                MSFilter *file_player = ms_factory_create_filter(ms_factory, MS_FILE_PLAYER_ID);
                if (!file_player) {
                    printf("Failed to create file player filter!\n");
                    return;
                }

                ms_filter_call_method(file_player, MS_FILTER_SET_FILE, (void *)wav_file);
                int start = 1;
                ms_filter_call_method(file_player, MS_FILTER_START, &start);

                LinphoneCallParams *params = linphone_call_get_current_params(call);
                LinphoneAudioStream *audio_stream = linphone_call_params_get_audio_stream(params);
                
                MSFilter *encoder = linphone_audio_stream_get_encoder(audio_stream);
                if (!encoder) {
                    printf("Failed to get encoder!\n");
                    return;
                }

                ms_filter_link(file_player, 0, encoder, 0);
            }
            break;
        case LinphoneCallEnd:
            printf("Call ended. Stopping playback...\n");
            running = FALSE;
            break;
        case LinphoneCallError:
            printf("Call error!\n");
            break;
        default:
            printf("Unhandled call state: %i\n", cstate);
    }
}

int main(int argc, char *argv[]) {
    LinphoneCoreCbs *cbs;
    LinphoneCore *lc;
    LinphoneFactory *factory;
    LinphoneCall *call = NULL;
    const char *dest = NULL;

    if (argc > 1) {
        dest = argv[1];
    }

    signal(SIGINT, stop);
    factory = linphone_factory_get();
    cbs = linphone_core_cbs_new();
    linphone_core_cbs_set_call_state_changed(cbs, call_state_changed);

    lc = linphone_factory_create_core(factory, cbs, NULL, NULL);
    linphone_core_set_sip_port(lc, 5061);

    if (dest) {
        call = linphone_core_invite(lc, dest);
        if (!call) {
            printf("Could not place call to %s\n", dest);
            goto end;
        } else {
            printf("Call to %s in progress...\n", dest);
        }
    }
    
    while (running) {
        linphone_core_iterate(lc);
        ms_usleep(50000);
    }

end:
    printf("Shutting down...\n");
    linphone_core_destroy(lc);
    linphone_core_cbs_unref(cbs);
    printf("Exited\n");
    return 0;
}