#include <stdint.h>
#include "memory/pmm.h"
#include "vaultfs.h"
#include "string.h"

void vaultfs_init(VaultFs *fs) { memset(fs, 0, sizeof(VaultFs)); }

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

uint8_t *vaultfs_read(VaultFs *fs, uint64_t profile_id, const char *path, uint64_t *size_out) {
    VaultNode *node = vaultfs_resolve(fs, profile_id, path);
    if (node == NULL) return NULL;
    if (node->data == NULL) return NULL;
    *size_out = node->size;
    return node->data;
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
    VaultNode *target_node = vaultfs_resolve(fs, profile_id, path);

    int in_c0 = 0, in_c1 = 0;
    if (target_node != NULL) {
        in_c0 = (target_node >= &fs->layer0.nodes[0] && target_node < &fs->layer0.nodes[128]);
        in_c1 = (target_node >= &fs->layer1.nodes[0] && target_node < &fs->layer1.nodes[128]);
        if (target_node->type == VAULT_DIR) return -1;
        if (in_c0) return -1;
    }

    if (target_node == NULL) {
        target_node = vaultfs_create(fs, profile_id, path, VAULT_FILE);
        if (target_node == NULL) return -1;
    } else if (in_c1) {
        target_node = vaultfs_create(fs, profile_id, path, target_node->type);
        if (target_node == NULL) return -1;
    }

    if (size > PAGE_SIZE) return -1;
    uint64_t phys = pmm_alloc_page();
    if (phys == 0) return -1;

    if (!in_c0 && !in_c1 && target_node->data != NULL)
        pmm_free_page((uint64_t)target_node->data);

    memcpy((void *)phys_to_virt(phys), data, size);
    target_node->data = (uint8_t *)phys;
    target_node->size = size;

    return 0;
}

int vaultfs_mkdir(VaultFs *fs, uint64_t profile_id, const char *path) {
    VaultNode *existing = vaultfs_resolve(fs, profile_id, path);
    if (existing != NULL) return -1;

    VaultNode *node = vaultfs_create(fs, profile_id, path, VAULT_DIR);
    if (node == NULL) return -1;

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
    // syscall privilegie later
    for(uint8_t i = 0; i < fs->profile_count; i++) {
        if(fs->profiles[i].profile_id == profile_id) {
            for(uint8_t j = 0; j < fs->profiles[i].layer2.count; j++) {
                if(strcmp(fs->profiles[i].layer2.nodes[j].name, path) == 0) {
                    VaultNode *node = &fs->profiles[i].layer2.nodes[j];
                    if(node->type == VAULT_DELETED) return -1;
                    if(fs->layer1.count >= 128) return -1;
                    fs->layer1.nodes[fs->layer1.count] = *node;
                    fs->layer1.nodes[fs->layer1.count].owner_profile_id = profile_id;
                    fs->layer1.count++;
                    fs->profiles[i].layer2.nodes[j] = fs->profiles[i].layer2.nodes[fs->profiles[i].layer2.count - 1];
                    fs->profiles[i].layer2.count--;
                    return 0;
                }
            }
        }
    }
    return -1;
}

int vaultfs_depublish(VaultFs *fs, uint64_t profile_id, const char *path) {
    // syscall privilegie later
    for(uint8_t i = 0; i < fs->layer1.count; i++) {
        if(strcmp(fs->layer1.nodes[i].name, path) == 0) {
            if(fs->layer1.nodes[i].owner_profile_id != profile_id) return -1;
            for (uint8_t j = 0; j < fs->profile_count; j++) {
              if (fs->profiles[j].profile_id != profile_id) continue;
              if (fs->profiles[j].layer2.count >= 128) return -1;
              fs->profiles[j].layer2.nodes[fs->profiles[j].layer2.count] = fs->layer1.nodes[i];
              fs->profiles[j].layer2.nodes[fs->profiles[j].layer2.count].owner_profile_id = profile_id;
              fs->profiles[j].layer2.count++;

              fs->layer1.nodes[i] = fs->layer1.nodes[fs->layer1.count - 1];
              fs->layer1.count--;

              return 0;
            }
        }
    }

    return -1;
}

int vaultfs_create_profile(VaultFs *fs, uint64_t profile_id, const char *name) {
    if(fs->profile_count >= 16) return -1;
    fs->profiles[fs->profile_count].profile_id = profile_id;
    fs->profiles[fs->profile_count].layer2.count = 0;
    memcpy(fs->profiles[fs->profile_count].name, name, 63);
    fs->profiles[fs->profile_count].name[63] = '\0';
    fs->profile_count++;

    return 0;
}

int vault_destroy_profile(VaultFs *fs, uint64_t profile_id) {
    for (uint8_t i = 0; i < fs->profile_count; i++) {
        if (fs->profiles[i].profile_id == profile_id) {
            for (uint8_t j = 0; j < fs->profiles[i].layer2.count; j++) {
              VaultNode *node = &fs->profiles[i].layer2.nodes[j];
                if (node->data != NULL && node->type != VAULT_DELETED) {
                  pmm_free_page((uint64_t)node->data);
                }
            }

            fs->profiles[i] = fs->profiles[fs->profile_count - 1];
            fs->profile_count--;

            return 0;
        }
    }

    return -1;
}

VaultProfile *vaultfs_find_profile_by_name(VaultFs *fs, const char *name) {
    for (uint8_t i = 0; i < fs->profile_count; i++) {
        if (strcmp(fs->profiles[i].name, name) == 0)
            return &fs->profiles[i];
    }
    return NULL;
}
