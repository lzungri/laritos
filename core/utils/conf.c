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
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>

int conf_readline(fs_file_t *f, char **tokens, uint32_t *tokens_size, uint8_t num_tokens, uint32_t *offset) {
    int i;
    for (i = 0; i < num_tokens; i++) {
        memset(tokens[i], 0, tokens_size[i]);
    }

    uint8_t token = 0;
    uint8_t tokenpos = 0;
    bool comment = false;
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
            } else if (comment) {
                continue;
            }

            if (buf[i] == ' ') {
                token++;
                tokenpos = 0;
            } else if (buf[i] == '#') {
                comment = true;
            } else if (token < num_tokens && tokenpos < tokens_size[token] - 1) {
                tokens[token][tokenpos++] = buf[i];
            }
        }
    }

    return token != num_tokens - 1 || tokenpos == 0 ? 0 : 1;
}
