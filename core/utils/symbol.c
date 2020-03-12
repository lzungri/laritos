//#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <string.h>
#include <printf.h>
#include <strtoxl.h>
#include <utils/symbol.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>

static int get_symbol_info(char *query, bool query_by_name, char *name_result, size_t namelen, void **addr_result) {
    fs_file_t *f = vfs_file_open("/sys/kinfo/symbols", FS_ACCESS_MODE_READ);
    if (f == NULL) {
        error_async("Couldn't open kernel symbols file");
        return -1;
    }

    char saddr[17] = { 0 };
    char prev_saddr[17] = { 0 };
    char sname[128] = { 0 };
    char prev_sname[128] = { 0 };

    uint32_t offset = 0;

    int ret = -1;

    // tokens: 0=addr, 1=type, 2=name
    uint8_t token = 0;
    uint8_t tokenpos = 0;
    while (true) {
        char buf[256];
        int nbytes = vfs_file_read(f, buf, sizeof(buf), offset);
        if (nbytes <= 0) {
            break;
        }

        int i;
        for (i = 0; i < nbytes; i++) {
            if (buf[i] == '\n') {
                // Check if we have all the previous tokens, if not, return error;
                if (token != 2) {
                    goto end;
                }

                insane_async("symbol=%s, addr=0x%s", sname, saddr);

                // If query by symbol name, then compare query with current symbol name, otherwise
                // compare symbol addresses
                if ((query_by_name && strncmp(sname, query, sizeof(sname)) == 0) ||
                        (!query_by_name && strncmp(saddr, query, sizeof(saddr)) >= 0) ||
                        (!query_by_name && i == nbytes - 1 && nbytes < sizeof(buf))) {
                    verbose_async("Found symbol=%s, addr=0x%s", sname, saddr);

                    char *name_to_return = sname;
                    char *addr_to_return = saddr;
                    // If this is a fuzzy query (i.e. not the exact symbol address), then
                    // return the closest and lowest symbol to the given address
                    if (!query_by_name && strncmp(saddr, query, sizeof(saddr)) > 0) {
                        name_to_return = prev_sname;
                        addr_to_return = prev_saddr;
                    }

                    if (name_result != NULL) {
                        strncpy(name_result, name_to_return, namelen);
                    }
                    if (addr_result != NULL) {
                        *addr_result = (void *) strtoul(addr_to_return, NULL, 16);
                    }
                    ret = 0;
                    goto end;
                }

                token = 0;
                tokenpos = 0;
                if (addr_result != NULL) {
                    memcpy(prev_saddr, saddr, sizeof(saddr));
                }
                if (name_result != NULL) {
                    memcpy(prev_sname, sname, sizeof(sname));
                }
                memset(saddr, 0, sizeof(saddr));
                memset(sname, 0, sizeof(sname));
            } else if (buf[i] == ' ') {
                token++;
                tokenpos = 0;
            } else {
                if (token == 0) {
                    if (tokenpos < sizeof(saddr) - 1) {
                        saddr[tokenpos++] = buf[i];
                    }
                } else if (token == 2) {
                    if (tokenpos < sizeof(sname) - 1) {
                        sname[tokenpos++] = buf[i];
                    }
                }
            }
        }

        offset += nbytes;
    }

end:
    vfs_file_close(f);
    return ret;
}

void *symbol_get(char *name) {
    verbose_async("Searching for symbol '%s'", name);
    void *addr_result = NULL;
    if (get_symbol_info(name, true, NULL, 0, &addr_result) < 0) {
        error("Could not get symbol '%s'", name);
    }
    return addr_result;
}

int symbol_get_name_at(void *addr, char *buf, size_t blen) {
    verbose_async("Searching for symbol at 0x%p", addr);
    char addr_query[17] = { 0 };
    snprintf(addr_query, sizeof(addr_query), "%08x", addr);
    if (get_symbol_info(addr_query, false, buf, blen - 1, NULL) < 0) {
        error("Could not get symbol at 0x%p", addr);
        return -1;
    }
    return 0;
}



#ifdef CONFIG_TEST_CORE_UTILS_SYMBOL
#include __FILE__
#endif
