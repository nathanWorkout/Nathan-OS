#ifndef VAULTFS_H
#define VAULTFS_H
#include <stdint.h>

typedef enum {
    VAULT_FILE,
    VAULT_DIR,
    VAULT_DELETED
} VaultNodeType;

typedef struct {
    char name[128];
    VaultNodeType type;
    uint8_t *data;
    uint64_t size;
    uint64_t owner_profile_id;
} VaultNode;

typedef struct {
    VaultNode nodes[128];
    uint64_t  count;
} VaultIndex;

typedef struct {
    uint64_t profile_id;
    char name[64];
    VaultIndex layer2;
} VaultProfile;

typedef struct {
    VaultIndex layer0;
    VaultIndex layer1;
    VaultProfile profiles[16];
    uint8_t profile_count;
} VaultFs;

void vaultfs_init(VaultFs *fs);
VaultNode *vaultfs_resolve(VaultFs *fs, uint64_t profile_id, const char *path);
uint8_t *vaultfs_read(VaultFs *fs, uint64_t profile_id, const char *path, uint64_t *size_out);
VaultNode *vaultfs_create(VaultFs *fs, uint64_t profile_id, const char *name, VaultNodeType type);
int vaultfs_write(VaultFs *fs, uint64_t profile_id, const char *path, uint8_t *data, uint64_t size);
int vaultfs_mkdir(VaultFs *fs, uint64_t profile_id, const char *path);
int vaultfs_delete(VaultFs *fs, uint64_t profile_id, const char *path);
int vaultfs_publish(VaultFs *fs, uint64_t profile_id, const char *path);
int vaultfs_depublish(VaultFs *fs, uint64_t profile_id, const char *path);
int vault_destroy_profile(VaultFs *fs, uint64_t profile_id);
int vaultfs_create_profile(VaultFs *fs, uint64_t profile_id, const char *name);
VaultProfile *vaultfs_find_profile_by_name(VaultFs *fs, const char *name);

#endif
