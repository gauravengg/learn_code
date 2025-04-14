#include <linphone/linphonecore.h>
#include <mediastreamer2/msfactory.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int running = 1;
static LinphoneCore *lc = NULL;
static LinphoneCall *current_call = NULL;
static char last_sip_address[256] = {0};

// Handle SIGINT (Ctrl + C)
static void stop(int signum) {
    printf("\nSIGINT received! Stopping...\n");
    running = 0;
}

// Function to list available audio devices
static void list_audio_devices() {
    const bctbx_list_t *devices = linphone_core_get_sound_devices_list(lc);
    printf("🎧 Available Audio Devices:\n");
    for (const bctbx_list_t *it = devices; it != NULL; it = it->next) {
        printf(" - %s\n", (const char*)it->data);
    }
}

// Function to set the audio input/output device
static void set_audio_device(const char *input_device, const char *output_device) {
    printf("🔄 Changing Audio Devices...\n");
    
    // Set microphone (capture) device
    linphone_core_set_capture_device(lc, input_device);
    printf("🎙️ Microphone Set to: %s\n", input_device);
    
    // Set speaker (playback) device
    linphone_core_set_playback_device(lc, output_device);
    printf("🔊 Speaker Set to: %s\n", output_device);
    
    // Reload sound devices to apply changes
    linphone_core_reload_sound_devices(lc);
}

// Function to make a call
static void make_call(const char *sip_address) {
    if (!sip_address || strlen(sip_address) == 0) {
        printf("❌ No SIP address provided!\n");
        return;
    }

    // Create call parameters
    LinphoneCallParams *params = linphone_core_create_call_params(lc, NULL);  
    if (!params) {
        printf("❌ Failed to create call params!\n");
        return;
    }

    linphone_call_params_enable_audio(params, TRUE);
    linphone_call_params_set_audio_direction(params, LinphoneMediaDirectionSendRecv);

    printf("📞 Calling: %s\n", sip_address);
    current_call = linphone_core_invite_with_params(lc, sip_address, params);
    
    if (current_call) {
        printf("✅ Call initiated to %s\n", sip_address);
        strncpy(last_sip_address, sip_address, sizeof(last_sip_address) - 1);
    } else {
        printf("❌ Failed to initiate call to %s\n", sip_address);
    }

    linphone_call_params_unref(params);
}

// Callback when account registration state changes
static void account_registration_state_changed(LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState state, const char *message) {
    printf("🔗 Registration state: %s\n", linphone_registration_state_to_string(state));
    if (state == LinphoneRegistrationOk) {
        printf("✅ Registration successful!\n");
    } else if (state == LinphoneRegistrationFailed) {
        printf("❌ Registration failed: %s\n", message);
    }
}

// Callback when call state changes
static void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState state, const char *message) {
    printf("📞 Call state: %s\n", linphone_call_state_to_string(state));

    switch (state) {
        case LinphoneCallOutgoingRinging:
            printf("🔔 Ringing remotely...\n");
            break;

        case LinphoneCallConnected:
            printf("✅ Call connected to %s\n", linphone_call_get_remote_address_as_string(call));

            // Set audio devices
            set_audio_device("ALSA: default", "ALSA: default");  

            // Set the file to be played
            printf("🎵 Playing recorded audio file...\n");
            linphone_core_set_play_file(lc, "/path/to/your/file.wav");  

            // Ensure echo cancellation is disabled if needed
            linphone_core_enable_echo_cancellation(lc, FALSE);

            break;

        case LinphoneCallStreamsRunning:
            printf("🔊 Media streams established.\n");
            break;

        case LinphoneCallEnd:
            printf("🔇 Call ended.\n");
            break;

        case LinphoneCallReleased:
            printf("❌ Call released.\n");
            printf("📌 Last Call SIP Address: %s\n", last_sip_address);
            break;

        case LinphoneCallError:
            printf("⚠️ Call error: %s\n", message);
            running = 0;
            break;

        default:
            printf("ℹ️ Call state changed: %d\n", state);
            break;
    }
}

int main() {
    signal(SIGINT, stop);

    // Initialize LinphoneCore with callbacks
    LinphoneCoreVTable vtable = {0};
    vtable.account_registration_state_changed = account_registration_state_changed;
    vtable.call_state_changed = call_state_changed;

    lc = linphone_core_new(&vtable, NULL, NULL, NULL);
    if (!lc) {
        printf("❌ Failed to initialize LinphoneCore.\n");
        return -1;
    }
    printf("✅ Linphone initialized. Setting up SIP account...\n");

    // SIP Account Credentials
    const char *identity = "sip:gauravpg@sip.linphone.org";
    const char *password = "Gp@123";

    LinphoneAddress *from = linphone_address_new(identity);
    if (!from) {
        printf("❌ Invalid SIP URI: %s\n", identity);
        return -1;
    }

    if (password) {
        LinphoneAuthInfo *info = linphone_auth_info_new(linphone_address_get_username(from), NULL, password, NULL, NULL, NULL);
        linphone_core_add_auth_info(lc, info);
    }

    LinphoneAccountParams *account_params = linphone_account_params_new(NULL);
    linphone_account_params_set_identity_address(account_params, from);
    linphone_account_params_set_register_enabled(account_params, TRUE);
    linphone_account_params_set_server_addr(account_params, linphone_address_get_domain(from));
    
    LinphoneAccount *account = linphone_core_create_account(lc, account_params);
    linphone_core_add_account(lc, account);
    linphone_core_set_default_account(lc, account);

    printf("🔄 Registering with SIP server...\n");

    // List available audio devices
    list_audio_devices();

    // Set default audio device
    set_audio_device("ALSA: default", "ALSA: default");  

    // Make a test call
    make_call("sip:rp0786@sip.linphone.org");

    // Main event loop
    while (running) {
        linphone_core_iterate(lc);
        usleep(50000);
    }

    printf("🔻 Shutting down...\n");
    linphone_core_unref(lc);
    return 0;
}
