#include <stdint.h>
#include "memory/pmm.h"
#include "vaultfs.h"
#include "string.h"
#include "../kernel/Graphic/gui/wallpaper_loader/png.h"

void vaultfs_init(VaultFs *fs) {
    memset(fs, 0, sizeof(VaultFs));
    fs->next_inode = 1;

    uint64_t phys = pmm_alloc_page();
    VaultNode *root = (VaultNode *)phys_to_virt(phys);
    memset(root, 0, sizeof(VaultNode));

    root->inode_id     = 0;
    root->parent_inode = 0;
    root->type         = VAULT_DIR;
    root->name[0]      = '/';
    root->name[1]      = '\0';
    root->next         = NULL;

    fs->layer0.head  = root;
    fs->layer0.count = 1;
}

VaultNode *vaultfs_resolve(VaultFs *fs, uint64_t profile_id, const char *path) {
    int deleted = 0;

    for (uint8_t i = 0; i < fs->profile_count; i++) {
        if (fs->profiles[i].profile_id != profile_id) continue;
        VaultNode *n = fs->profiles[i].layer2.head;
        while (n) {
            if (strcmp(n->name, path) == 0) {
                if (n->type == VAULT_DELETED) { deleted = 1; break; }
                return n;
            }
            n = n->next;
        }
    }

    if (!deleted) {
        VaultNode *n = fs->layer1.head;
        while (n) {
            if (strcmp(n->name, path) == 0) return n;
            n = n->next;
        }
    }

    if (!deleted) {
        VaultNode *n = fs->layer0.head;
        while (n) {
            if (strcmp(n->name, path) == 0) return n;
            n = n->next;
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

VaultNode *vaultfs_create(VaultFs *fs, uint64_t profile_id, const char *name, uint64_t parent_inode, VaultNodeType type) {
    for (uint8_t i = 0; i < fs->profile_count; i++) {
        if (fs->profiles[i].profile_id != profile_id) continue;

        uint64_t phys = pmm_alloc_page();
        if (phys == 0) return NULL;
        VaultNode *node = (VaultNode *)phys_to_virt(phys);

        memset(node, 0, sizeof(VaultNode));
        node->inode_id         = fs->next_inode++;
        node->parent_inode     = parent_inode;
        node->type             = type;
        node->data             = NULL;
        node->size             = 0;
        node->owner_profile_id = profile_id;
        memcpy(node->name, name, 127);
        node->name[127]        = '\0';

        node->next                  = fs->profiles[i].layer2.head;
        fs->profiles[i].layer2.head = node;
        fs->profiles[i].layer2.count++;

        return node;
    }
    return NULL;
}

int vaultfs_write(VaultFs *fs, uint64_t profile_id, const char *path, uint8_t *data, uint64_t size) {
    VaultNode *target_node = vaultfs_resolve(fs, profile_id, path);

    int in_c0 = 0, in_c1 = 0;
    if (target_node != NULL) {
        VaultNode *n = fs->layer0.head;
        while (n) {
            if (n == target_node) { 
                in_c0 = 1; 
                break; 
            }
            n = n->next;
        }
        n = fs->layer1.head;
        while (n) {
            if (n == target_node) { 
                in_c1 = 1; 
                break; 
            }
            n = n->next;
        }

        if (target_node->type == VAULT_DIR) return -1;
        if (in_c0) return -1;
    }

    uint64_t cwd = 0;
    for (uint8_t i = 0; i < fs->profile_count; i++) {
        if (fs->profiles[i].profile_id == profile_id) {
            cwd = fs->profiles[i].cwd_inode;
            break;
        }
    }

    if (target_node == NULL) {
        target_node = vaultfs_create(fs, profile_id, path, cwd, VAULT_FILE);
        if (target_node == NULL) return -1;
    } else if (in_c1) {
        target_node = vaultfs_create(fs, profile_id, path, cwd, VAULT_FILE);
        if (target_node == NULL) return -1;
    }

    uint64_t pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    // si size = 5000
    // (5000 + 4095) / 4096
    // = 9095 / 4096
     // = 2 avec juste size / PAGE_SIZE c'est pasw assez
    if (pages_needed == 0) pages_needed = 1;

    uint64_t phys = pmm_alloc_pages_contig(pages_needed);
    if (phys == 0) return -1;

    if (!in_c0 && !in_c1 && target_node->data != NULL)
        pmm_free_pages_contig(virt_to_phys((uint64_t)target_node->data), target_node->data_pages);

    target_node->data       = (uint8_t *)phys_to_virt(phys);
    target_node->size       = size;
    target_node->data_pages = pages_needed;
    memcpy(target_node->data, data, size);

    return 0;
}

int vaultfs_mkdir(VaultFs *fs, uint64_t profile_id, const char *path) {
    VaultNode *existing = vaultfs_resolve(fs, profile_id, path);
    if (existing != NULL) return -1;

    uint64_t cwd = 0;
    for (uint8_t i = 0; i < fs->profile_count; i++) {
        if (fs->profiles[i].profile_id == profile_id) {
            cwd = fs->profiles[i].cwd_inode;
            break;
        }
    }
    VaultNode *node = vaultfs_create(fs, profile_id, path, cwd, VAULT_DIR);
    if (node == NULL) return -1;

    return 0;
}

int vaultfs_delete(VaultFs *fs, uint64_t profile_id, const char *path) {
    for (uint8_t i = 0; i < fs->profile_count; i++) {
        if (fs->profiles[i].profile_id != profile_id) continue;

        VaultNode *prev = NULL;
        VaultNode *curr = fs->profiles[i].layer2.head;
        while (curr) {
            if (strcmp(curr->name, path) == 0) {
                if (curr->data != NULL) pmm_free_pages_contig(virt_to_phys((uint64_t)curr->data), curr->data_pages);
                if (prev) {
                    prev->next = curr->next;
                } else {
                    fs->profiles[i].layer2.head = curr->next;
                }     
                pmm_free_page(virt_to_phys((uint64_t)curr));
                fs->profiles[i].layer2.count--;
                return 0;
            }
            prev = curr;
            curr = curr->next;
        }
    }

    VaultNode *prev = NULL;
    VaultNode *curr = fs->layer1.head;
    while (curr) {
        if (strcmp(curr->name, path) == 0) {
            if (curr->owner_profile_id != profile_id) return -1;
            if (curr->data != NULL) pmm_free_pages_contig(virt_to_phys((uint64_t)curr->data), curr->data_pages);
            if (prev) {
                prev->next = curr->next;
            } else {
                fs->layer1.head = curr->next;
            }     
            pmm_free_page(virt_to_phys((uint64_t)curr));
            fs->layer1.count--;
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }

    curr = fs->layer0.head;
    while (curr) {
        if (strcmp(curr->name, path) == 0) return -1;
        curr = curr->next;
    }

    return -1;
}

int vaultfs_publish(VaultFs *fs, uint64_t profile_id, const char *path) {
    for (uint8_t i = 0; i < fs->profile_count; i++) {
        if (fs->profiles[i].profile_id != profile_id) continue;

        VaultNode *prev = NULL;
        VaultNode *curr = fs->profiles[i].layer2.head;
        while (curr) {
            if (strcmp(curr->name, path) == 0) {
                if (curr->type == VAULT_DELETED) return -1;

                if (prev) {
                    prev->next = curr->next;
                }
                else {
                    fs->profiles[i].layer2.head = curr->next;
                }    
                fs->profiles[i].layer2.count--;

                curr->owner_profile_id = profile_id;
                curr->next = fs->layer1.head;
                fs->layer1.head = curr;
                fs->layer1.count++;
                return 0;
            }
            prev = curr;
            curr = curr->next;
        }
    }
    return -1;
}

int vaultfs_depublish(VaultFs *fs, uint64_t profile_id, const char *path) {
    VaultNode *prev = NULL;
    VaultNode *curr = fs->layer1.head;
    while (curr) {
        if (strcmp(curr->name, path) == 0) {
            if (curr->owner_profile_id != profile_id) return -1;

            for (uint8_t j = 0; j < fs->profile_count; j++) {
                if (fs->profiles[j].profile_id != profile_id) continue;

                if (prev) {
                    prev->next = curr->next;
                } else {
                    fs->layer1.head = curr->next;
                }

                fs->layer1.count--;

                curr->next = fs->profiles[j].layer2.head;
                fs->profiles[j].layer2.head = curr;
                fs->profiles[j].layer2.count++;
                return 0;
            }
        }
        prev = curr;
        curr = curr->next;
    }
    return -1;
}

int vaultfs_create_profile(VaultFs *fs, uint64_t profile_id, const char *name) {
    if (fs->profile_count >= 16) return -1;
    fs->profiles[fs->profile_count].profile_id    = profile_id;
    fs->profiles[fs->profile_count].layer2.count  = 0;
    fs->profiles[fs->profile_count].layer2.head   = NULL;
    fs->profiles[fs->profile_count].cwd_inode = 0;
    memcpy(fs->profiles[fs->profile_count].name, name, 63);
    fs->profiles[fs->profile_count].name[63] = '\0';
    fs->profile_count++;
    return 0;
}

int vault_destroy_profile(VaultFs *fs, uint64_t profile_id) {
    for (uint8_t i = 0; i < fs->profile_count; i++) {
        if (fs->profiles[i].profile_id != profile_id) continue;

        VaultNode *curr = fs->profiles[i].layer2.head;
        while (curr) {
            VaultNode *next = curr->next;
            if (curr->data != NULL)
                pmm_free_pages_contig(virt_to_phys((uint64_t)curr->data), curr->data_pages);
            pmm_free_page(virt_to_phys((uint64_t)curr));
            curr = next;
        }

        VaultNode *prev = NULL;
        curr = fs->layer1.head;
        while (curr) {
            VaultNode *next = curr->next;
            if (curr->owner_profile_id == profile_id) {
                if (prev) prev->next = next;
                else      fs->layer1.head = next;
                if (curr->data != NULL)
                    pmm_free_pages_contig(virt_to_phys((uint64_t)curr->data), curr->data_pages);// supprime les data
                pmm_free_page(virt_to_phys((uint64_t)curr)); // supprime le noeud
                fs->layer1.count--;
            } else {
                prev = curr;
            }
            curr = next;
        }

        fs->profile_count--;
        fs->profiles[i] = fs->profiles[fs->profile_count];

        return 0;
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

// Fonctions pour le cd
VaultNode *vaultfs_resolve_inode(VaultFs *fs, uint64_t profile_id, uint64_t inode_id) {
    for (uint8_t i = 0; i < fs->profile_count; i++) {
        if (fs->profiles[i].profile_id != profile_id) continue;
        VaultNode *n = fs->profiles[i].layer2.head;
        while (n) {
            if (n->inode_id == inode_id) {
                if (n->type == VAULT_DELETED) return NULL;
                return n;
            }
            n = n->next;
        }
    }

    VaultNode *n = fs->layer1.head;
    while (n) {
        if (n->inode_id == inode_id) return n;
        n = n->next;
    }

    n = fs->layer0.head;
    while (n) {
        if (n->inode_id == inode_id) return n;
        n = n->next;
    }

    return NULL;
}

VaultNode *vaultfs_resolve_path(VaultFs *fs, uint64_t profile_id, const char *path) {
    uint64_t current_inode = 0;

    if (path[0] == '/') {
        current_inode = 0; 
        path++;            
    } else {
        for (uint8_t i = 0; i < fs->profile_count; i++) {
            if (fs->profiles[i].profile_id == profile_id) {
                current_inode = fs->profiles[i].cwd_inode;
                break;
            }
        }
    }

    if (path[0] == '\0') return vaultfs_resolve_inode(fs, profile_id, current_inode);

    char segment[128];
    const char *p = path;

    while (*p != '\0') {
        uint8_t len = 0;
        while (*p != '/' && *p != '\0' && len < 127)
            segment[len++] = *p++;
        segment[len] = '\0';

        if (*p == '/') p++;
        VaultNode *found = NULL;

        for (uint8_t i = 0; i < fs->profile_count; i++) {
            if (fs->profiles[i].profile_id != profile_id) continue;
            VaultNode *n = fs->profiles[i].layer2.head;
            while (n) {
                if (n->parent_inode == current_inode && strcmp(n->name, segment) == 0) {
                    if (n->type == VAULT_DELETED) return NULL;
                    found = n;
                    break;
                }
                n = n->next;
            }
        }

        if (!found) {
            VaultNode *n = fs->layer1.head;
            while (n) {
                if (n->parent_inode == current_inode && strcmp(n->name, segment) == 0) {
                    found = n;
                    break;
                }
                n = n->next;
            }
        }

        if (!found) {
            VaultNode *n = fs->layer0.head;
            while (n) {
                if (n->parent_inode == current_inode && strcmp(n->name, segment) == 0) {
                    found = n;
                    break;
                }
                n = n->next;
            }
        }

        if (found == NULL) return NULL;
        if (*p != '\0' && found->type != VAULT_DIR) return NULL;

        current_inode = found->inode_id;
    }

    return vaultfs_resolve_inode(fs, profile_id, current_inode);
}

int vaultfs_cd(VaultFs *fs, uint64_t profile_id, const char *path) {
    VaultNode *node = vaultfs_resolve_path(fs, profile_id, path);
    if (node == NULL) return -1;
    if (node->type != VAULT_DIR) return -1;
    
    for (uint8_t i = 0; i < fs->profile_count; i++) {
        if (fs->profiles[i].profile_id != profile_id) continue;
        fs->profiles[i].cwd_inode = node->inode_id;
        return 0;
    }

    return -1;
}

void vaultfs_get_cwd_path(VaultFs *fs, uint64_t profile_id, char *out, uint64_t max) {
    uint64_t cwd = 0;
    for (uint8_t i = 0; i < fs->profile_count; i++) {
        if (fs->profiles[i].profile_id == profile_id) {
            cwd = fs->profiles[i].cwd_inode;
            break;
        }
    }

    if (cwd == 0) {
        out[0] = '/';
        out[1] = '\0';
        return;
    }

    char segments[32][128];
    uint8_t depth = 0;
    uint64_t current = cwd;

    while (current != 0) {
        VaultNode *node = vaultfs_resolve_inode(fs, profile_id, current);
        if (node == NULL) break;

        memcpy(segments[depth], node->name, 127);
        segments[depth][127] = '\0';
        depth++;

        current = node->parent_inode;
    }

    uint64_t pos = 0;
    for (int8_t i = depth - 1; i >= 0; i--) {
        if (pos + 1 >= max) break;
        out[pos++] = '/';

        uint64_t len = strlen(segments[i]);
        if (pos + len >= max) break;
        memcpy(out + pos, segments[i], len);
        pos += len;
    }
    out[pos] = '\0';
}