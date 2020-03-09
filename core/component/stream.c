#include <log.h>

#include <stdbool.h>
#include <printf.h>
#include <core.h>
#include <dstruct/circbuf.h>
#include <component/component.h>
#include <component/stream.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>


int stream_component_init(stream_t *s, board_comp_t *bcomp, char *id, component_type_t type,
        int (*read)(stream_t *s, void *buf, size_t n, bool b), int (*write)(stream_t *s, const void *buf, size_t n, bool b)) {
    if (component_init((component_t *) s, id, bcomp, type, NULL, NULL) < 0) {
        error("Failed to initialize '%s' stream component", bcomp->id);
        return -1;
    }
    s->ops.read = read;
    s->ops.write = write;
    return 0;
}

static int sysfs_data_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    stream_t *s = f->data0;
    return s->ops.read(s, buf, blen, true);
}

static int sysfs_data_write(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    stream_t *s = f->data0;
    return s->ops.write(s, buf, blen, true);
}

static int create_instance_sysfs(stream_t *s) {
    fs_dentry_t *root = vfs_dentry_lookup_from(_laritos.fs.comp_type_root, "stream");
    fs_dentry_t *dir = vfs_dir_create(root, s->parent.id, FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (dir == NULL) {
        error("Error creating '%s' sysfs directory", s->parent.id);
        return -1;
    }

    if (pseudofs_create_custom_rw_file_with_dataptr(dir, "data", sysfs_data_read, sysfs_data_write, s) == NULL) {
        error("Failed to create 'read' sysfs file");
        return -1;
    }

    return 0;
}

int stream_component_register(stream_t *s) {
    if (component_register((component_t *) s) < 0) {
        error("Couldn't register '%s'", s->parent.id);
        return -1;
    }
    create_instance_sysfs(s);
    return 0;
}

SYSFS_COMPONENT_TYPE_MODULE(stream)
