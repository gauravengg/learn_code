#include <linphone/linphonecore.h>
#include <linphone/factory.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define INCOMING TRUE

static bool_t running = TRUE;

static void stop(int signum) {
    running = FALSE;
}

/*
* Call state notification callback
*/
static void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg) {
    printf("[%p] --------------------------------------> cstate %d\n", call, cstate);
    const LinphoneAddress *remote_address_obj = NULL;
    const char *remote_address_str = NULL;

    switch (cstate) {
        case LinphoneCallStatePushIncomingReceived:
            printf("[%p] ✅ Accepting call...\n", call);
            LinphoneCallParams *accept_params = linphone_core_create_call_params(lc, call);
            linphone_call_accept_with_params(call, accept_params);
            linphone_call_params_unref(accept_params);
            break;

        case LinphoneCallIncomingReceived:  // Incoming Call Detected
            printf("[%p] ✅ Accepting new call...\n", call);
            LinphoneCallParams *accept_params_incoming = linphone_core_create_call_params(lc, call);
            linphone_call_accept_with_params(call, accept_params_incoming);
            linphone_call_params_unref(accept_params_incoming);
            linphone_call_set_microphone_muted(call, TRUE);

            // Generate a unique filename for recording
            time_t timer;
            char buffer[26];
            struct tm* tm_info;
            time(&timer);
            tm_info = localtime(&timer);
            strftime(buffer, 26, "%Y%m%d_%H%M%S", tm_info);

            remote_address_obj = linphone_call_get_remote_address(call);
            if (remote_address_obj) {
                remote_address_str = linphone_address_as_string(remote_address_obj);
                char filename[512];
                snprintf(filename, sizeof(filename), "call_record_%s_%s.wav", buffer, remote_address_str);
                linphone_core_set_record_file(lc, filename);
                linphone_call_start_recording(call);
                printf("[%p] ⏺️ Started recording to: %s\n", call, filename);
            }
            break;

        case LinphoneCallReleased:
            printf("[%p] ❌ Call ended.\n", call);
            linphone_call_stop_recording(call);
            break;
        case LinphoneCallOutgoingRinging:
            printf("[%p] It is now ringing remotely !\n", call);
            break;
        case LinphoneCallOutgoingEarlyMedia:
            printf("[%p] Receiving some early media\n", call);
            break;
        case LinphoneCallConnected:
            remote_address_obj = linphone_call_get_remote_address(call);
            if (remote_address_obj) {
                remote_address_str = linphone_address_as_string(remote_address_obj);
                printf("[%p] ✅ Call connected to %s\n", call, remote_address_str);
            }
            break;
        case LinphoneCallStreamsRunning:
            printf("[%p] Media streams established !\n", call);
            break;
        case LinphoneCallEnd:
            printf("[%p] Call is terminated.\n", call);
            break;
        case LinphoneCallError:
            printf("[%p] ⚠️ Call error: %s\n", call, msg);
            break;
        case LinphoneCallIdle:
            printf("[%p] Linphone is in Idle state: %s\n", call, msg);
            break;
        default:
            printf("[%p] ℹ️ Call state changed: %d\n", call, cstate);
            break;
    }
    if (remote_address_obj) {
        linphone_address_unref(remote_address_obj);
    }
}

int main(int argc, char *argv[]) {
    // Initialize Linphone Core
    LinphoneCoreVTable vtable = {0};
    signal(SIGINT, stop);
    char *username, *password, *server_address;
    char *sip_url_to_register = NULL; // Define the SIP URL for registration

    if (argc < 4) {
        printf("please enter ./your_executable SIP_Username SIP_Password SIP_Server\n");
        return -1;
    }
#ifdef DEBUG_LOGS
    /*enable liblinphone logs.*/
    linphone_core_enable_logs(NULL);
    linphone_core_set_log_level(LinphoneLogLevelDebug);
#endif
    username = argv[1];
    password = argv[2];
    server_address = argv[3];
    sip_url_to_register = (char*)malloc(256);
    if (sip_url_to_register == NULL) {
        perror("Failed to allocate memory for SIP URL");
        return -1;
    }
    snprintf(sip_url_to_register, 256, "sip:%s@%s", username, linphone_address_get_domain(linphone_address_new(server_address)));

    vtable.call_state_changed = call_state_changed;

    // Updated LinphoneCore initialization
    LinphoneFactory *factory = linphone_factory_get();
    LinphoneCore *lc = linphone_factory_create_core(factory, &vtable, NULL);
    if (!lc) {
        printf("❌ Failed to initialize LinphoneCore.\n");
        free(sip_url_to_register);
        return -1;
    } else
        printf("✅ Linphone initialized. Setting up SIP account for %s...\n", sip_url_to_register);

    LinphoneAddress *from = linphone_address_new(sip_url_to_register);
    if (!from) {
        printf("❌ Invalid SIP URI: %s\n", sip_url_to_register);
        linphone_core_unref(lc);
        free(sip_url_to_register);
        return -1;
    }
    LinphoneAuthInfo *info = linphone_auth_info_new(linphone_address_get_username(from), NULL, password, NULL, NULL, NULL);
    linphone_core_add_auth_info(lc, info);
    LinphoneAccountParams *account_params = linphone_account_params_new(lc);
    linphone_account_params_set_identity_address(account_params, from);
    linphone_account_params_set_register_enabled(account_params, TRUE);
    linphone_account_params_set_server_addr(account_params, linphone_address_get_domain(from));

    LinphoneAccount *account = linphone_core_create_account(lc, account_params);
    linphone_core_add_account(lc, account);
    linphone_core_set_default_account(lc, account);

    printf("🔄 Registering %s with SIP server...\n", sip_url_to_register);
    while (running) {
        linphone_core_iterate(lc);
        usleep(50000); // 50ms loop delay
    }
    printf("🔻 Shutting down...\n");
    linphone_core_unref(lc);
    free(sip_url_to_register);
    linphone_factory_get();

    return 0;
}