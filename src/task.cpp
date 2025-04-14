 
#include <linphone/core.h>
#include <mediastreamer2/msfilter.h>
#include <signal.h>
static bool_t running = TRUE;
static void stop(int signum) {
    running = FALSE;
}
/* Call state notification callback */
static void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg) {
    if (cstate == LinphoneCallConnected) {
        printf("Call connected! Playing audio file...\n");
        /* Step 1: Create and configure the file player */
        MSFilter *file_player = ms_filter_new(MS_FILE_PLAYER_ID);
        if (!file_player) {
            printf("Error: Could not create file player filter.\n");
            return;
        }
        /* Step 2: Set the file to play */
        const char *wav_file = "/home/gaurav/linphone_project/src/output_c_api_2.wav";  // Change to your actual file path
        ms_filter_call_method(file_player, MS_FILE_PLAYER_OPEN, (void *)wav_file);
        /* Step 3: Start playback */
        int start = 1;
        ms_filter_call_method(file_player, MS_FILE_PLAYER_START, &start);  
        /* Step 4: Attach the file player to Linphone’s media stream */
        LinphoneCallParams *params = linphone_call_get_current_params(call);
        if (params) {
            LinphoneAudioStream *audio_stream = linphone_call_params_get_audio_stream(params);
            if (audio_stream) {
                ms_filter_link(file_player, 0, linphone_audio_stream_get_encoder(audio_stream), 0);
            }
        }
        /* Step 5: Mute microphone since we're using the file */
        linphone_core_enable_mic(lc, FALSE);
        printf("Audio file is now playing to the remote party.\n");
    }
}
int main(int argc, char *argv[]) {
    LinphoneCoreVTable vtable = {0};
    LinphoneCore *lc;
    LinphoneCall *call = NULL;
    const char *dest;
    if (argc < 2) {
        printf("Usage: %s sip:user@sipserver\n", argv[0]);
        return -1;
    }
    dest = argv[1];
    signal(SIGINT, stop);
    linphone_core_enable_logs(NULL);
    vtable.call_state_changed = call_state_changed;
    lc = linphone_core_new(&vtable, NULL, NULL, NULL);
    /* Initiate the call */
    call = linphone_core_invite(lc, dest);
    if (!call) {
        printf("Could not place call to %s\n", dest);
        goto cleanup;
    }
    linphone_call_ref(call);
    while (running) {
        linphone_core_iterate(lc);
        ms_usleep(50000);
    }
cleanup:
    printf("Shutting down...\n");
    linphone_core_destroy(lc);
    printf("Exited.\n");

    void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg) {
        if (cstate == LinphoneCallConnected) {
            printf("Call connected! Playing audio file...\n");
    
            LinphonePlayer *player = linphone_core_create_player(lc);
            if (!player) {
                printf("Error: Could not create player.\n");
                return;
            }
    
            const char *wav_file = "/home/gaurav/linphone_project/src/output_c_api_2.wav";  // Change to actual file path
            int result = linphone_player_open(player, wav_file);
            if (result < 0) {
                printf("Error: Failed to open audio file.\n");
                return;
            }
    
            linphone_player_start(player);
            linphone_core_enable_mic(lc, FALSE);  // Mute microphone
    
            printf("Audio file is now playing to the remote party.\n");
        }
    }
    
    return 0;
}
 
 