#include <log.h>

#include <fs/vfs-types.h>
#include <fs/vfs-core.h>
#include <module/core.h>

static fs_mount_t *mount(fs_type_t *type, char *mount_point, uint16_t flags, void *params) {
    return NULL;
}

FS_TYPE_MODULE(pseudofs, mount);



#ifdef CONFIG_TEST_CORE_FS_PSEUDOFS
#include __FILE__
#endif
