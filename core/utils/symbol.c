//#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <string.h>
#include <printf.h>
#include <strtoxl.h>
#include <utils/symbol.h>
#include <utils/conf.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>

static int get_symbol_info(char *query, bool query_by_name, char *name_result, size_t namelen, void **addr_result) {
    fs_file_t *f = vfs_file_open("/sys/kinfo/symbols", FS_ACCESS_MODE_READ);
    if (f == NULL) {
        error_async("Couldn't open kernel symbols file");
        return -1;
    }

    bool found = false;

    char prev_saddr[17] = { 0 };
    char prev_sname[128] = { 0 };

    char saddr[17];
    char stype;
    char sname[128];
    char *tokens[] = { saddr, &stype, sname };
    uint32_t tokens_size[] = { sizeof(saddr), sizeof(char), sizeof(sname) };

    uint32_t offset = 0;
    int ret;
    while ((ret = conf_readline(f, tokens, tokens_size, ARRAYSIZE(tokens), &offset)) != 0) {
        if (ret < 0) {
            continue;
        }

        // If query by symbol name, then compare query with current symbol name, otherwise
        // compare symbol addresses
        if ((query_by_name && strncmp(sname, query, sizeof(sname)) == 0) ||
                (!query_by_name && strncmp(saddr, query, sizeof(saddr)) >= 0)) {
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
            found = true;
            break;
        }

        if (addr_result != NULL) {
            memcpy(prev_saddr, saddr, sizeof(saddr));
        }
        if (name_result != NULL) {
            memcpy(prev_sname, sname, sizeof(sname));
        }
    }

    // If it is a fuzzy query and the symbol hasn't been found yet, then the last symbol
    // in the file (i.e. the one with the higher addr) will match the query
    if (found == false && !query_by_name) {
        if (name_result != NULL) {
            strncpy(name_result, prev_sname, namelen);
        }
        if (addr_result != NULL) {
            *addr_result = (void *) strtoul(prev_saddr, NULL, 16);
        }
        found = true;
    }

    vfs_file_close(f);
    return found ? 0 : -1;
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
