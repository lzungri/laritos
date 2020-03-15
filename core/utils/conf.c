//#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <string.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>

int conf_readline(fs_file_t *f, char **tokens, uint32_t *tokens_size, uint8_t num_tokens, uint32_t *offset) {
    uint8_t token = 0;
    uint8_t tokenpos = 0;

    int i;
    for (i = 0; i < num_tokens; i++) {
        memset(tokens[i], 0, tokens_size[i]);
    }

    while (true) {
        char buf[32];
        int nbytes = vfs_file_read(f, buf, sizeof(buf), *offset);
        if (nbytes <= 0) {
            break;
        }

        for (i = 0; i < nbytes; i++) {
            (*offset)++;

            if (buf[i] == '\n') {
                // Check if we have all the previous tokens, if not, return error;
                return token != num_tokens - 1 || tokenpos == 0 ?  -1 : 1;
            } else if (buf[i] == ' ') {
                token++;
                tokenpos = 0;
            } else {
                if (token < num_tokens && tokenpos < tokens_size[token] - 1) {
                    tokens[token][tokenpos++] = buf[i];
                }
            }
        }
    }

    return token != num_tokens - 1 || tokenpos == 0 ? 0 : 1;
}
