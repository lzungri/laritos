#pragma once

#include <stdint.h>
#include <fs/vfs/types.h>

/**
 * @return 0 when EOF
 *        -1 when line doesn't contain all the tokens
 *         1 if all the tokens were populated
 */
int conf_readline(fs_file_t *f, char **tokens, uint32_t *tokens_size, uint8_t num_tokens, uint32_t *offset);
