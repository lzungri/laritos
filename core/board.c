#include <log.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <strtoxl.h>
#include <irq.h>
#include <board-types.h>
#include <board.h>
#include <component/component.h>

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

/**
 * Parses the board information and populates the board_info_t
 *
 * Board info syntax:
 *      component1:driver1|attr1=attr1value,attrN=attrNvalue\n
 *      component2:driver2|attr1=attr1value\n
 *      component3:driver3\n
 *
 * @param bi_start_addr: Start address of the plain-text board info
 * @param bi: Pointer to a board info struct
 *
 * @return 0 on success, < 0 on error
 */
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
        // Check for EOF
        if (*token == '\0') {
            return 0;
        }

        if (bi->len >= CONFIG_MAX_COMPONENTS) {
            error("Max number of board info components reached, max=%d", CONFIG_MAX_COMPONENTS);
            return -1;
        }

        board_comp_t *ci = &bi->components[bi->len];

#define check_attr_limit() \
    if (ci->attrlen >= CONFIG_BOARD_MAX_COMPONENT_ATTRS) { \
        error("Max number of attributes for component '%s' reached", ci->id); \
        return -1; \
    }

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
                check_attr_limit();
                ci->attr[ci->attrlen].name = token;
                break;
            case ',':
                if (in_component) {
                    goto syntax_error;
                }
                check_attr_limit();
                ci->attr[ci->attrlen].value = token;
                ci->attrlen++;
                break;
            case '\n':
            case '\0':
                if (in_component) {
                    ci->driver = token;
                } else {
                    check_attr_limit();
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
            // Check current token length limit
            if (strlen(token) > CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES - 1) {
                error("Token '%s' exceeds maximum token length", token);
                goto syntax_error;
            }
            token = cur;
        } else {
            found_keyword = true;
            cur++;
        }
    }

syntax_error:
    error("Syntax error in line %d", bi->len);
    return -1;
}

void board_dump_board_info(board_info_t *bi) {
    log_always("%s board information:", BOARD.name);
    int i;
    for (i = 0; i < bi->len; i++) {
        board_comp_t *c = &bi->components[i];
        log_always("   %s (driver=%s)", c->id, c->driver);
        int j;
        for (j = 0; j < c->attrlen; j++) {
            log_always("      %s: %s", c->attr[j].name, c->attr[j].value);
        }
    }
}

int board_get_ptr_attr(board_comp_t *bc, char *attr, void **buf) {
    int v;
    if (board_get_int_attr(bc, attr, &v) < 0 || v <= 0) {
        return -1;
    }
    *buf = (void *) v;
    return 0;
}

void board_get_ptr_attr_def(board_comp_t *bc, char *attr, void **buf, void *def) {
    if (board_get_ptr_attr(bc, attr, buf) < 0) {
        *buf = def;
    }
}

int board_get_int_attr(board_comp_t *bc, char *attr, int *buf) {
    char str[CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES] = { 0 };
    if (board_get_str_attr(bc, attr, str) < 0 || strnlen(str, sizeof(str)) < 0) {
        return -1;
    }
    *buf = strtol(str, NULL, 0);
    return 0;
}

void board_get_int_attr_def(board_comp_t *bc, char *attr, int *buf, int def) {
    if (board_get_int_attr(bc, attr, buf) < 0) {
        *buf = def;
    }
}

int board_get_str_attr_idx(board_comp_t *bc, char *attr, char *buf, uint8_t index) {
    int i;
    for (i = 0; i < bc->attrlen; i++) {
        if (strncmp(bc->attr[i].name, attr, CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES) == 0 && index-- == 0) {
            strncpy(buf, bc->attr[i].value, CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES - 1);
            buf[CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES - 1] = '\0';
            return 0;
        }
    }
    return -1;
}

void board_get_str_attr_idx_def(board_comp_t *bc, char *attr, char *buf, uint8_t index, char *def) {
    if (board_get_str_attr_idx(bc, attr, buf, index) < 0) {
        strncpy(buf, def, CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES - 1);
        buf[CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES - 1] = '\0';
    }
}

int board_get_str_attr(board_comp_t *bc, char *attr, char *buf) {
    return board_get_str_attr_idx(bc, attr, buf, 0);
}

void board_get_str_attr_def(board_comp_t *bc, char *attr, char *buf, char *def) {
    board_get_str_attr_idx_def(bc, attr, buf, 0, def);
}

int board_get_bool_attr(board_comp_t *bc, char *attr, bool *buf) {
    char str[CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES] = { 0 };
    if (board_get_str_attr(bc, attr, str) < 0 || strnlen(str, sizeof(str)) <= 0) {
        return -1;
    }
    *buf = strncmp(str, "yes", sizeof(str)) == 0 || strncmp(str, "true", sizeof(str)) == 0
            || strncmp(str, "y", sizeof(str)) == 0 || strncmp(str, "1", sizeof(str)) == 0;
    return 0;
}

void board_get_bool_attr_def(board_comp_t *bc, char *attr, bool *buf, bool def) {
    if (board_get_bool_attr(bc, attr, buf) < 0) {
        *buf = def;
    }
}

int board_get_irq_trigger_attr(board_comp_t *bc, char *attr, irq_trigger_mode_t *buf) {
    char str[CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES] = { 0 };
    if (board_get_str_attr(bc, attr, str) < 0 || strnlen(str, sizeof(str)) <= 0) {
        return -1;
    }
    if (strncmp(str, "level_hi", sizeof(str)) == 0) {
        *buf = IRQ_TRIGGER_LEVEL_HIGH;
    } else if (strncmp(str, "level_low", sizeof(str)) == 0) {
        *buf = IRQ_TRIGGER_LEVEL_LOW;
    } else if (strncmp(str, "edge_hi_lo", sizeof(str)) == 0) {
        *buf = IRQ_TRIGGER_EDGE_HIGH_LOW;
    } else if (strncmp(str, "edge_lo_hi", sizeof(str)) == 0) {
        *buf = IRQ_TRIGGER_EDGE_LOW_HIGH;
    } else {
        return -1;
    }
    return 0;
}

void board_get_irq_trigger_attr_def(board_comp_t *bc, char *attr, irq_trigger_mode_t *buf, irq_trigger_mode_t def) {
    if (board_get_irq_trigger_attr(bc, attr, buf) < 0) {
        *buf = def;
    }
}

int board_get_component_attr(board_comp_t *bc, char *attr, component_t **buf) {
    char str[CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES] = { 0 };
    if (board_get_str_attr(bc, attr, str) < 0 || strnlen(str, sizeof(str)) <= 0) {
        return -1;
    }
    *buf = component_get_by_id(str);
    return *buf != NULL ? 0 : -1;
}
