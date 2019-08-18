#define MODULE "core.board"

#include <stddef.h>
#include <board.h>
#include <log.h>
#include <string.h>

int board_init(board_info_t *bi) {

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
    int in_component = 1;
    int found_keyword = 0;

    while (1) {
        if (*token == '\0') {
            return 0;
        }

        comp_info_t *ci = &bi->components[bi->len];
        switch (*cur) {
            case ':':
                if (!in_component) {
                    goto syntax_error;
                }
                ci->name = token;
                break;
            case '|':
                if (!in_component) {
                    goto syntax_error;
                }
                ci->driver = token;
                in_component = 0;
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
                in_component = 1;
                break;
            default:
                found_keyword = 0;
                break;
        }

        if (found_keyword) {
            while(*cur == ':' || *cur == '|' || *cur == '=' || *cur == '\n' || *cur == ',') {
                *cur = '\0';
                cur++;
            }
            token = cur;
        } else {
            found_keyword = 1;
            cur++;
        }

        if (bi->len >= BOARD_MAX_COMPONENTS) {
            warn("Max number of board info components reached, max=%d", BOARD_MAX_COMPONENTS);
            return 0;
        }
    }

syntax_error:
    error("Syntax error in line %d", bi->len);
    return -1;
}
