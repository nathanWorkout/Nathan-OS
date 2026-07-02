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
        }

	break;
    }

    for(uint8_t j = 0; j < fs->layer1.count; j++) {
        if(strcmp(fs->layer1.nodes[j].name, path) == 0) {
	    return &fs->layer1.nodes[j];
	}

	break;
    }

    for(uint8_t j = 0; j < fs->layer0.count; j++) {
	if(strcmp(fs->layer0.nodes[j].name, path) == 0) {
	    return &fs->layer0.nodes[j];
	}

	break;
    }

    return NULL;
}

void test() {}
