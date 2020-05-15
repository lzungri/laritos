/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

//#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <string.h>
#include <printf.h>
#include <strtoxl.h>
#include <symbol.h>
#include <utils/conf.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>

static int get_symbol_info(char *query, bool query_by_name, char *name_result, size_t namelen, void **addr_result) {
    fs_file_t *f = vfs_file_open("/kinfo/symbols", FS_ACCESS_MODE_READ);
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



#ifdef TEST_CORE_LIBC_SYMBOL
#include __FILE__
#endif
