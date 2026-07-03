#include <stdint.h>
#include "memory/pmm.h"
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

  for(uint8_t i = 0; i < fs->profile_count; i++) {
        if (fs->profiles[i].profile_id == profile_id) {
            if (fs->profiles[i].layer2.count >= 128) return NULL;

            VaultNode *node = &fs->profiles[i].layer2.nodes[fs->profiles[i].layer2.count];

            memcpy(node->name, name, 127);
            node->name[127] = '\0';
            node->type = type;
            node->data = NULL;
            node->size = 0;
            node->owner_profile_id = 0;
            fs->profiles[i].layer2.count++;
            return node;
        }
    }
    return NULL;
}

int vaultfs_write(VaultFs *fs, uint64_t profile_id, const char *path, uint8_t *data, uint64_t size) {
    // Retourne 0 si succès, -1 si erreur
    VaultNode *node = vaultfs_resolve(fs, profile_id, path);
    int in_c0 = 0;
    int in_c1 = 0;
    if (node != NULL) {
        in_c0 = (node >= &fs->layer0.nodes[0] && node < &fs->layer0.nodes[128]);
        in_c1 = (node >= &fs->layer1.nodes[0] && node < &fs->layer1.nodes[128]);
    }
    if (node != NULL && node->type == VAULT_DIR) return -1;
    if (in_c0) return -1;

    int was_in_c2 = 0;
    if (node == NULL) {
        node = vaultfs_create(fs, profile_id, path, VAULT_FILE);
        if (node == NULL) return -1;
    } else if (in_c1) {
        node = vaultfs_create(fs, profile_id, path, node->type); // copy on write        if (node == NULL) return -1;
    } else {
        was_in_c2 = 1;
    }

    if (size > PAGE_SIZE) return -1; // Temporaire (V0.1 de Three States Filesystem)
    uint64_t phys = pmm_alloc_page();
    if (phys == 0) return -1;

    if (was_in_c2 && node->data != NULL) {
        pmm_free_page((uint64_t)node->data);
    }

    uint8_t *ptr = (uint8_t *)phys_to_virt(phys);
    memcpy(ptr, data, size);
    node->data = (uint8_t *)phys;
    node->size = size;
    return 0;
}

int vaultfs_delete(VaultFs *fs, uint64_t profile_id, const char *path) {

    for (uint8_t i = 0; i < fs->profile_count; i++) {
        if (fs->profiles[i].profile_id == profile_id) {
            for (uint8_t j = 0; j < fs->profiles[i].layer2.count; j++) {
                if (strcmp(fs->profiles[i].layer2.nodes[j].name, path) == 0) {
                    VaultNode *node = &fs->profiles[i].layer2.nodes[j];
                    if (node->data != NULL) {
                        pmm_free_page((uint64_t)node->data);
                    }
                    fs->profiles[i].layer2.nodes[j] = fs->profiles[i].layer2.nodes[fs->profiles[i].layer2.count - 1];
                    fs->profiles[i].layer2.count--;
                    return 0;
                }
            }
        }
    }

    for (uint8_t j = 0; j < fs->layer1.count; j++) {
        if (strcmp(fs->layer1.nodes[j].name, path) == 0) {
            if (fs->layer1.nodes[j].owner_profile_id != profile_id) {
                return -1;
            }
            VaultNode *node = &fs->layer1.nodes[j];
            if (node->data != NULL) {
                pmm_free_page((uint64_t)node->data);
            }
            fs->layer1.nodes[j] = fs->layer1.nodes[fs->layer1.count - 1];
            fs->layer1.count--;
            return 0;
        }
    }

    for (uint8_t j = 0; j < fs->layer0.count; j++) {
        if (strcmp(fs->layer0.nodes[j].name, path) == 0) {
            return -1;
        }
    }

    return -1;
}

int vaultfs_publish(VaultFs *fs, uint64_t profile_id, const char *path) {

}
