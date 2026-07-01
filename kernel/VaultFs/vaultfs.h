#ifndef VAULTFS_H
#define VAULTFS_H
#include <cstdint>
#include <stdint.h>

typedef enum {
    VAULT_FILE,
    VAULT_DIR,
    VAULT_SYMLINK
} VaultNodeType;

typedef struct {
    char name[128];
    VaultNodeType type;
    uint8_t *data;
    uint64_t size;
} VaultNode;

typedef struct {
    VaultNode nodes[128];
    uint64_t  count;
} VaultIndex;

typedef struct {
    uint64_t profile_id;
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

#endif
