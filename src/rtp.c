#include <linphone/linphonecore.h>
#include <mediastreamer2/mscommon.h>
#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/msfileplayer.h>
#include <mediastreamer2/msfilewriter.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int running = 1;
static LinphoneCore *lc = NULL;
static LinphoneCall *current_call = NULL;
static MSFilter *my_audio_tap = NULL;

// Signal handler
void stop(int signum) {
    printf("❗ SIGINT received. Exiting...\n");
    running = 0;
}

// Custom callback to process decoded remote audio
static void custom_audio_cb(MSFilter *f, void *data) {
    mblk_t *m;
    while ((m = ms_queue_get(f->inputs[0]))) {
        // Example: write raw PCM to a file
        write((intptr_t)data, m->b_rptr, m->b_wptr - m->b_rptr);
        freemsg(m);
    }
}

// Create a custom MSFilter to tap remote audio
static MSFilter *create_audio_tap_filter(int fd) {
    MSFilter *f = ms_filter_new_from_desc(ms_filter_get_encoder("raw"));  // Dummy type
    f->desc = ms_filter_get_decoder("genericplc"); // Hack to give dummy desc

    f->inputs = ms_new0(MSQueue *, 1);
    f->outputs = ms_new0(MSQueue *, 1);
    f->inputs[0] = ms_queue_new();
    f->outputs[0] = ms_queue_new();

    f->process = custom_audio_cb;
    f->data = (void *)(intptr_t)fd;
    return f;
}

// When call connects: hook remote audio stream
void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState state, const char *msg) {
    printf("📞 Call state: %s\n", linphone_call_state_to_string(state));
    if (state == LinphoneCallConnected) {
        printf("✅ Call connected\n");

        // Get the audio stream
        const LinphoneCallParams *params = linphone_call_get_current_params(call);
        if (linphone_call_params_audio_enabled(params)) {
            AudioStream *stream = linphone_call_get_audio_stream(call);

            int audio_fd = open("remote_audio.raw", O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (audio_fd < 0) {
                perror("❌ Could not open output file");
                return;
            }

            // Create and insert tap
            my_audio_tap = create_audio_tap_filter(audio_fd);
            audio_stream_insert_filter(stream, my_audio_tap, MS_FILTER_OUTPUT, 0);
            printf("🔊 Remote audio tap attached!\n");
        }
    } else if (state == LinphoneCallEnd || state == LinphoneCallReleased) {
        printf("📴 Call ended\n");
        if (my_audio_tap) {
            ms_filter_destroy(my_audio_tap);
            my_audio_tap = NULL;
        }
        running = 0;
    }
}

void account_registration_state_changed(LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState state, const char *msg) {
    printf("🔐 Registration state: %s\n", linphone_registration_state_to_string(state));
    if (state == LinphoneRegistrationOk) {
        printf("✅ Registration successful\n");

        // Dial the number
        const char *dest = "sip:receiver@sip.linphone.org";
        LinphoneCallParams *params = linphone_core_create_call_params(lc, NULL);
        linphone_core_invite_with_params(lc, dest, params);
        linphone_call_params_unref(params);
    }
}

int main(int argc, char *argv[]) {
    signal(SIGINT, stop);

    LinphoneCoreVTable vtable = {0};
    vtable.call_state_changed = call_state_changed;
    vtable.account_registration_state_changed = account_registration_state_changed;

    lc = linphone_core_new(&vtable, NULL, NULL, NULL);
    if (!lc) {
        printf("❌ Failed to initialize LinphoneCore\n");
        return -1;
    }

    const char *identity = "sip:gauravpg@sip.linphone.org";
    const char *password = "Gp@123";
    const char *server = "sip.linphone.org";

    LinphoneAddress *addr = linphone_address_new(identity);
    LinphoneAuthInfo *auth_info = linphone_auth_info_new(linphone_address_get_username(addr), NULL, password, NULL, NULL, linphone_address_get_domain(addr));
    linphone_core_add_auth_info(lc, auth_info);

    LinphoneProxyConfig *proxy_cfg = linphone_core_create_proxy_config(lc);
    linphone_proxy_config_set_identity(proxy_cfg, identity);
    linphone_proxy_config_set_server_addr(proxy_cfg, server);
    linphone_proxy_config_enable_register(proxy_cfg, TRUE);
    linphone_core_add_proxy_config(lc, proxy_cfg);
    linphone_core_set_default_proxy(lc, proxy_cfg);
    linphone_address_unref(addr);

    printf("📞 Waiting for registration and call...\n");
    while (running) {
        linphone_core_iterate(lc);
        usleep(50000);
    }

    linphone_core_destroy(lc);
    return 0;
}
