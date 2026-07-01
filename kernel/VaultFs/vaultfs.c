#include <stdint.h>
#include "vaultfs.h"
#include "string.h"

void vaultfs_init(VaultFs *fs) {
    memset(fs, 0, sizeof(VaultFs));
}

VaultNode *vaultfs_resolve(VaultFs *fs, uint64_t profile_id, const char *path) {
    for(uint8_t i = 0; i < fs->profile_count; i++) {
        if(fs->profiles[i].profile_id == profile_id) {
            for(uint8_t j = 0; j < fs->profiles[i].layer2.count; j++) {
                if (strcmp(fs->profiles[i].layer2.nodes[j].name, path) == 0) {
                    return &fs->profiles[i].layer2.nodes[j];
                }
            }

	    for(uint8_t k = 0; k < fs->profiles[i].layer1.count; k++) {

	    }
        }
    }

    return NULL;
}
