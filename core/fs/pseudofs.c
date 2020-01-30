#include <log.h>

#include <fs/vfs-types.h>
#include <fs/vfs-core.h>

fs_type_t pseudofs = {
   .id = "pseudofs",
   .mount = NULL,
};

int pseudofs_init(void) {
    return vfs_register_fs_type(&pseudofs);
}



#ifdef CONFIG_TEST_CORE_FS_PSEUDOFS
#include __FILE__
#endif
