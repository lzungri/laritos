#include <log.h>

#include <string.h>
#include <board/core.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <driver/core.h>
#include <component/blockdev.h>
#include <generated/autoconf.h>

static int process(board_comp_t *comp) {
    char mntpoint[CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES];
    if (board_get_str_attr(comp, "mntpoint", mntpoint) < 0) {
        error("No mntpoint was specified in the board info for '%s'", comp->id);
        return -1;
    }

    char type[CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES];
    if (board_get_str_attr(comp, "type", type) < 0) {
        error("No type was specified in the board info for '%s'", comp->id);
        return -1;
    }

    char modestr[CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES];
    if (board_get_str_attr(comp, "mode", modestr) < 0) {
        error("No mode was specified in the board info for '%s'", comp->id);
        return -1;
    }
    fs_access_mode_t mode = 0;
    if (strchr(modestr, 'r') != NULL) {
        mode |= FS_MOUNT_READ;
    }
    if (strchr(modestr, 'w') != NULL) {
        mode |= FS_MOUNT_WRITE;
    }

    blockdev_t *dev;
    if (board_get_component_attr(comp, "dev", (component_t **) &dev) < 0) {
        error("No dev was specified in the board info for '%s'", comp->id);
        return -1;
    }

    fs_mount_t *mnt = vfs_mount_fs(type, mntpoint, mode, (fs_param_t []) { { "dev", dev }, { NULL } });
    if (mnt == NULL) {
        error("Error mounting system image");
        return -1;
    }

    return 0;
}

DRIVER_MODULE(static_fs_mount, process);
