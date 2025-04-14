#include <linphone/linphonecore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int running = 1;
static LinphoneCore *lc = NULL;
static LinphoneCall *current_call = NULL;

static void stop(int signum) {
    printf("\nSIGINT received! Stopping...\n");
    running = 0;
}

// Registration callback
static void account_registration_state_changed(LinphoneCore *lc, LinphoneProxyConfig *cfg,
                                               LinphoneRegistrationState state, const char *message) {
    printf("🔗 Registration state: %s - %s\n",
           linphone_registration_state_to_string(state), message);
}

// Call state callback
static void call_state_changed(LinphoneCore *lc, LinphoneCall *call,
                               LinphoneCallState state, const char *message) {
    printf("📞 Call state: %s - %s\n",
           linphone_call_state_to_string(state), message);

    switch (state) {
        case LinphoneCallIncomingReceived:
            printf("📞 Incoming call from: %s\n",
                   linphone_call_get_remote_address_as_string(call));
            printf("Press 'a' to accept, 'r' to reject:\n");

            char choice;
            scanf(" %c", &choice);

            if (choice == 'a') {
                linphone_call_accept(call);
            } else {
                linphone_call_decline(call, LinphoneReasonDeclined);
            }
            break;

        case LinphoneCallConnected:
            current_call = call;
            break;

        case LinphoneCallEnd:
        case LinphoneCallReleased:
            current_call = NULL;
            break;

        case LinphoneCallError:
            fprintf(stderr, "⚠️ Call error: %s\n", message);
            break;

        default:
            break;
    }
}

int main() {
    signal(SIGINT, stop);

    LinphoneCoreVTable vtable = {0};
    vtable.call_state_changed = call_state_changed;
    vtable.registration_state_changed = account_registration_state_changed;

    lc = linphone_core_new(&vtable, NULL, NULL, NULL);
    if (!lc) {
        fprintf(stderr, "Failed to initialize LinphoneCore.\n");
        return 1;
    }

    printf("✅ Linphone initialized. Setting up SIP account...\n");

    const char *identity = "sip:gauravpg@sip.linphone.org";  // Change this
    const char *password = "Gp@123";                         // Change this

    LinphoneAddress *from = linphone_address_new(identity);
    if (!from) {
        fprintf(stderr, "Invalid SIP URI: %s\n", identity);
        return 1;
    }

    const char *username = linphone_address_get_username(from);
    const char *domain = linphone_address_get_domain(from);

    // Add auth info
    LinphoneAuthInfo *info = linphone_auth_info_new(username, NULL, password, NULL, NULL, domain);
    linphone_core_add_auth_info(lc, info);

    // Create proxy config
    LinphoneProxyConfig *proxy_cfg = linphone_proxy_config_new();
    linphone_proxy_config_set_identity(proxy_cfg, identity);
    char server_addr[256];
    snprintf(server_addr, sizeof(server_addr), "sip:%s", domain);
    linphone_proxy_config_set_server_addr(proxy_cfg, server_addr);
    linphone_proxy_config_enable_register(proxy_cfg, TRUE);

    linphone_core_add_proxy_config(lc, proxy_cfg);
    linphone_core_set_default_proxy_config(lc, proxy_cfg);

    printf("🔄 Registering...\n");

    while (running) {
        linphone_core_iterate(lc);
        usleep(50000);  // 50ms
    }

    linphone_core_destroy(lc);
    linphone_address_unref(from);
    printf("👋 Exiting.\n");

    return 0;
}
