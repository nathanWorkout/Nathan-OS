#ifndef VAULTFS_H
#define VAULTFS_H
#include <stdint.h>

typedef enum {
    VAULT_FILE,
    VAULT_DIR,
    VAULT_DELETED
} VaultNodeType;

typedef struct VaultNode {
    uint64_t inode_id;
    uint64_t parent_inode; 
    char name[128];
    VaultNodeType type;
    uint8_t *data;
    uint64_t size;
    uint64_t owner_profile_id;
    struct VaultNode *next; 
} VaultNode;

typedef struct {
    VaultNode *head;
    uint64_t count;
} VaultIndex;

typedef struct {
    uint64_t profile_id;
    char name[64];
    VaultIndex layer2;
    uint64_t cwd_inode; 
} VaultProfile;

typedef struct {
    VaultIndex layer0;
    VaultIndex layer1;
    VaultProfile profiles[16];
    uint8_t profile_count;
    uint64_t next_inode;  
} VaultFs;

void vaultfs_init         (VaultFs *fs);
VaultNode *vaultfs_resolve(VaultFs *fs, uint64_t profile_id, const char *path);
uint8_t *vaultfs_read     (VaultFs *fs, uint64_t profile_id, const char *path, uint64_t *size_out);
int vaultfs_write         (VaultFs *fs, uint64_t profile_id, const char *path, uint8_t *data, uint64_t size);
VaultNode *vaultfs_create (VaultFs *fs, uint64_t profile_id, const char *name, uint64_t parent_inode, VaultNodeType type);
int vaultfs_mkdir         (VaultFs *fs, uint64_t profile_id, const char *path);
int vaultfs_delete        (VaultFs *fs, uint64_t profile_id, const char *path);
int vaultfs_publish       (VaultFs *fs, uint64_t profile_id, const char *path);
int vaultfs_depublish     (VaultFs *fs, uint64_t profile_id, const char *path);
int vault_destroy_profile (VaultFs *fs, uint64_t profile_id);
int vaultfs_create_profile(VaultFs *fs, uint64_t profile_id, const char *name);
VaultProfile *vaultfs_find_profile_by_name(VaultFs *fs, const char *name);
VaultNode *vaultfs_resolve_inode(VaultFs *fs, uint64_t profile_id, uint64_t inode_id);
VaultNode *vaultfs_resolve_path(VaultFs *fs, uint64_t profile_id, const char *path);
int vaultfs_cd(VaultFs *fs, uint64_t profile_id, const char *path);
void vaultfs_get_cwd_path(VaultFs *fs, uint64_t profile_id, char *out, uint64_t max);

#endif