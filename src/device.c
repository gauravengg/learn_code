#include <linphone/linphonecore.h>
#include <stdio.h>

int main() {
    LinphoneCore *lc;
    LinphoneCoreVTable vtable = {0};

    // Set log level to DEBUG (numeric value 3)
    //linphone_core_set_log_level(3);  // 3 corresponds to DEBUG

    // Initialize LinphoneCore
    lc = linphone_core_new(&vtable, NULL, NULL, NULL);
    if (lc == NULL) {
        printf("❌ Failed to initialize LinphoneCore.\n");
        return -1;
    }

    // Get the list of available audio devices
    MSSndCardManager *snd_card_manager = ms_factory_get_snd_card_manager(linphone_core_get_ms_factory(lc));
    const bctbx_list_t *list = ms_snd_card_manager_get_list(snd_card_manager);

    if (list == NULL) {
        printf("❌ No audio devices found! Please check your system's audio setup.\n");
    } else {
        printf("🎧 Available Audio Devices:\n");
        int i = 0;
        for (const bctbx_list_t *it = list; it != NULL; it = bctbx_list_next(it)) {
            MSSndCard *card = (MSSndCard *)bctbx_list_get_data(it);
            printf("Audio Device %d: %s\n", i++, ms_snd_card_get_string_id(card));
        }
    }

    // Clean up
    linphone_core_destroy(lc);
    return 0;
}
 