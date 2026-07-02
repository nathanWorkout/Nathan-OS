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
                    if (fs->profiles[i].layer2.nodes[j].type == VAULT_DELETED) return NULL;
                    return &fs->profiles[i].layer2.nodes[j];
                }
            }
        }
    }

    for(uint8_t j = 0; j < fs->layer1.count; j++) {
        if(strcmp(fs->layer1.nodes[j].name, path) == 0) {
	          return &fs->layer1.nodes[j];
        }
    }

    for(uint8_t j = 0; j < fs->layer0.count; j++) {
        if(strcmp(fs->layer0.nodes[j].name, path) == 0) {
            return &fs->layer0.nodes[j];
        }
    }

    return NULL;
}

VaultNode *vaultfs_create(VaultFs *fs, uint64_t profile_id, const char *name, VaultNodeType type) {

  for (uint8_t i = 0; i < fs->profile_count; i++) {
        if (fs->profiles[i].profile_id == profile_id) {
            if (fs->profiles[i].layer2.count >= 128) return NULL;

            VaultNode *node = &fs->profiles[i].layer2.nodes[fs->profiles[i].layer2.count];

            memcpy(node->name, name, 127);
            node->name[127] = '\0';
            node->type = type;
            node->data = NULL;
            node->size = 0;
            fs->profiles[i].layer2.count++;
            return node;
        }
    }
    return NULL;
}
