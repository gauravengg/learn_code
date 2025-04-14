#include <linphone/linphonecore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INCOMING TRUE

static bool_t running = TRUE;

static void stop(int signum) {
    running = FALSE;
}

// Function to make a call
static void make_call(LinphoneCore *lc, const char *sip_address) {
    printf("--------------------- Calling back the sip address %s\n", sip_address);
    static char last_sip_address[256] = {0};
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
    LinphoneCall *current_call = linphone_core_invite_with_params(lc, sip_address, params);
    
    if (current_call) {
        printf("✅ Call initiated to %s\n", sip_address);
        strncpy(last_sip_address, sip_address, sizeof(last_sip_address) - 1);
    } else {
        printf("❌ Failed to initiate call to %s\n", sip_address);
    }
    linphone_call_params_unref(params);
}
/*
* Call state notification callback
*/
static void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg) {

    printf("--------------------------------------> cstate %d\n", cstate);
    switch (cstate) {
        case LinphoneCallStatePushIncomingReceived:
            printf("✅ Accepting call...\n");
            /*----------------------------------------------------------------------*/
            // Get the incoming user SIP address and save in the variable 
            /*----------------------------------------------------------------------*/
            linphone_call_accept(call);  // Accept the call
            break;

        case LinphoneCallIncomingReceived:  // Incoming Call Detected
            LinphoneCall *current_call = linphone_core_get_current_call(lc);

            if (current_call) {
                printf("🔴 Already in a call! Putting the current call on hold...\n");
                linphone_call_pause(current_call); // Hold the current call
            }
    
            // Accept the new call
            printf("✅ Accepting new call...\n");
            linphone_core_accept_call(lc, call);
            // Mute microphone so that user can't hear noise
            linphone_call_set_microphone_muted(call, TRUE);

            /*----------------------------------------------------------------------*/
            //Replcace with uniquq random sring 
            /*----------------------------------------------------------------------*/
            
            linphone_core_set_record_file(lc, "call_record.wav");               
            break;

        case LinphoneCallReleased:
            printf("❌ Call ended.\n");
            /*----------------------------------------------------------------------*/
            //Stop recording and the save the record in the database 
            /*----------------------------------------------------------------------*/
            linphone_call_stop_recording(call);
            break;        
        case LinphoneCallOutgoingRinging:
            printf("It is now ringing remotely !\n");
            break;
        case LinphoneCallOutgoingEarlyMedia:
            printf("Receiving some early media\n");
            break;
        case LinphoneCallConnected:
            printf("✅ Call connected to %s\n", linphone_call_get_remote_address_as_string(call));
            linphone_call_start_recording(call);
            break;
        case LinphoneCallStreamsRunning:
            printf("Media streams established !\n");
            break;
        case LinphoneCallEnd:
            printf("Call is terminated.\n");
            break;
        case LinphoneCallError:
            printf("⚠️ Call error: %s\n", msg);
            break;
        case LinphoneCallIdle:
            printf("Linphone is in Idle state: %s\n", msg);
            break;
        default:
            printf("ℹ️ Call state changed: %d\n", cstate);
            break;
    }
}

int main(int argc, char *argv[]) {
    // Initialize Linphone Core
    LinphoneCoreVTable vtable = {0};
    LinphoneCall *call = NULL;
    LinphoneProxyConfig *proxy_cfg;
    LinphoneAuthInfo *auth_info;
    LinphoneAddress *identity_addr;
    signal(SIGINT, stop);
    char *username, *password, *server_address, *sip_url;
    if (argc < 5) {
        printf("please enter ./a.out  SIP Username SIP Password SIP Server sip_url\n");
        return -1;
    }
#ifdef DEBUG_LOGS
    /*enable liblinphone logs.*/
    linphone_core_enable_logs(NULL); 
    linphone_core_set_log_level(LinphoneLogLevelDebug);
#endif
    username=argv[1];
    password=argv[2];
    server_address = argv[3];
    sip_url = argv[4];

    vtable.call_state_changed = call_state_changed;
    LinphoneCore *lc = linphone_core_new(&vtable, NULL, NULL, NULL);
    if (!lc) {
        printf("❌ Failed to initialize LinphoneCore.\n");
        return -1;
    }else
        printf("✅ Linphone initialized. Setting up SIP account...\n");

    LinphoneAddress *from = linphone_address_new(sip_url);
    if (!from) {
        printf("❌ Invalid SIP URI: %s\n", sip_url);
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
    

    printf("🔄 Registering with SIP server...\n");
    while (running) {
        linphone_core_iterate(lc);
        usleep(50000); // 50ms loop delay
    }
    printf("🔻 Shutting down...\n");
    linphone_core_unref(lc);

    return 0;
}