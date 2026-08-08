#include "vault_global.h"

VaultFs g_vaultfs;

void vault_global_init(void) {
    vaultfs_init(&g_vaultfs);
}