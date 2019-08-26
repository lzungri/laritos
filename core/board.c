#include <stddef.h>
#include <board.h>
#include <log.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <strtoxl.h>

int board_init(board_info_t *bi) {
    debug("Initializing board");
    return 0;
}

int board_parse_and_initialize(board_info_t *bi) {
    info("Parsing board info data");
    if (board_parse_info(_binary_boardinfo_start, bi) < 0) {
        error("Error parsing board info");
        return -1;
    }

    info("Initializing %s board", BOARD.name);
    if (BOARD.board_init(bi) < 0) {
        error("Failed to initialize board");
        return -1;
    }

    return 0;
}

int board_parse_info(char *bi_start_addr, board_info_t *bi) {
    if (bi_start_addr == NULL) {
        error("Board info address is NULL");
        return -1;
    }

    memset(bi, 0, sizeof(*bi));

    char *cur = bi_start_addr;
    char *token = cur;
    bool in_component = true;
    bool found_keyword = false;

    while (1) {
        if (*token == '\0') {
            return 0;
        }

        board_comp_t *ci = &bi->components[bi->len];
        switch (*cur) {
            case ':':
                if (!in_component) {
                    goto syntax_error;
                }
                ci->id = token;
                break;
            case '|':
                if (!in_component) {
                    goto syntax_error;
                }
                ci->driver = token;
                in_component = false;
                break;
            case '=':
                if (in_component) {
                    goto syntax_error;
                }
                ci->attr[ci->attrlen].name = token;
                break;
            case ',':
                if (in_component) {
                    goto syntax_error;
                }
                ci->attr[ci->attrlen].value = token;
                ci->attrlen++;
                break;
            case '\n':
            case '\0':
                if (in_component) {
                    ci->driver = token;
                } else {
                    ci->attr[ci->attrlen].value = token;
                    ci->attrlen++;
                }
                bi->len++;
                if (*cur == '\0') {
                    return 0;
                }
                in_component = true;
                break;
            default:
                found_keyword = false;
                break;
        }

        if (found_keyword) {
            while(*cur == ':' || *cur == '|' || *cur == '=' || *cur == '\n' || *cur == ',') {
                *cur = '\0';
                cur++;
            }
            token = cur;
        } else {
            found_keyword = true;
            cur++;
        }

        if (bi->len >= CONFIG_BOARD_MAX_COMPONENTS) {
            warn("Max number of board info components reached, max=%d", CONFIG_BOARD_MAX_COMPONENTS);
            return 0;
        }
    }

syntax_error:
    error("Syntax error in line %d", bi->len);
    return -1;
}

int board_get_ptr_attr(board_comp_t *bc, char *attr, void **buf, void *def) {
    int v;
    if (board_get_int_attr(bc, attr, &v, -1) < 0) {
        return -1;
    }
    *buf = v >= 0 ? (void *) v : def;
    return 0;
}

int board_get_int_attr(board_comp_t *bc, char *attr, int *buf, int def) {
    char str[BOARD_MAX_ATTR_VALUE_LEN_BYTES] = { 0 };
    if (board_get_str_attr(bc, attr, str, "") < 0) {
        return -1;
    }
    *buf = strnlen(str, sizeof(str)) > 0 ? strtol(str, NULL, 0) : def;
    return 0;
}

int board_get_str_attr(board_comp_t *bc, char *attr, char *buf, char *def) {
    int i;
    char *value = def;
    for (i = 0; i < bc->attrlen; i++) {
        if (strncmp(bc->attr[i].name, attr, BOARD_MAX_ATTR_NAME_LEN_BYTES) == 0) {
            value = bc->attr[i].value;
            break;
        }
    }
    strncpy(buf, value, BOARD_MAX_ATTR_VALUE_LEN_BYTES - 1);
    return 0;
}

int board_get_bool_attr(board_comp_t *bc, char *attr, bool *buf, bool def) {
    return -1;
}
