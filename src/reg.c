#include <linphone/linphonecore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int running = 1;
static LinphoneCore *lc = NULL;
static LinphoneCall *current_call = NULL;

// Handle SIGINT for graceful shutdown
static void stop(int signum) {
    printf("\nSIGINT received! Stopping...\n");
    running = 0;
}
printf("🎤 Capture device: %s\n", linphone_core_get_capture_device(lc));
printf("🔊 Playback device: %s\n", linphone_core_get_playback_device(lc));


// SIP Registration Callback
static void account_registration_state_changed(LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState state, const char *message) {
    printf("🔗 Registration state: %s\n", linphone_registration_state_to_string(state));

    if (state == LinphoneRegistrationOk) {
        printf("✅ Registration successful! Waiting for incoming calls...\n");
    } else if (state == LinphoneRegistrationFailed) {
        printf("❌ Registration failed: %s\n", message);
    }
}

// Call State Callback (Handles Incoming Calls)
static void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState state, const char *message) {
    printf("📞 Call state: %s\n", linphone_call_state_to_string(state));

    switch (state) {
        case LinphoneCallIncomingReceived:  // Incoming Call Detected
            printf("📞 Incoming call from: %s\n", linphone_call_get_remote_address_as_string(call));
            printf("Press 'a' to accept, 'r' to reject:\n");

            char choice;
            scanf(" %c", &choice);  // Get user input

            if (choice == 'a') {
                printf("✅ Accepting call...\n");
                linphone_call_accept(call);  // Accept the call
            } else {
                printf("❌ Rejecting call...\n");
                linphone_call_decline(call, LinphoneReasonDeclined);  // Reject the call
            }
            break;

        case LinphoneCallConnected:
            printf("✅ Call connected to %s\n", linphone_call_get_remote_address_as_string(call));
            current_call = call;  // Store active call
            break;

        case LinphoneCallEnd:
        case LinphoneCallReleased:
            printf("❌ Call ended.\n");
            current_call = NULL;
            break;

        case LinphoneCallError:
            printf("⚠️ Call error: %s\n", message);
            break;

        default:
            printf("ℹ️ Call state changed: %d\n", state);
            break;
    }
}

int main() {
    signal(SIGINT, stop);

    LinphoneCoreVTable vtable = {0};
    vtable.account_registration_state_changed = account_registration_state_changed;
    vtable.call_state_changed = call_state_changed;

    lc = linphone_core_new(&vtable, NULL, NULL, NULL);
    if (!lc) {
        printf("❌ Failed to initialize LinphoneCore.\n");
        return -1;
    }

    printf("✅ Linphone initialized. Setting up SIP account...\n");

    // SIP Account Configuration
    const char *identity = "sip:gauravpg@sip.linphone.org";  // Your SIP address
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

    // Main loop
    while (running) {
        linphone_core_iterate(lc);
        usleep(50000); // 50ms loop delay
    }

    printf("🔻 Shutting down...\n");
    linphone_core_unref(lc);
    return 0;
}
 