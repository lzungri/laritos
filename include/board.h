#pragma once

#include <board-types.h>
#include <component.h>

int board_init(board_info_t *binfo);
int board_parse_info(char *bi_start_addr, board_info_t *bi);
int board_parse_and_initialize(board_info_t *bi);
void board_dump_board_info(board_info_t *bi);

int board_get_ptr_attr(board_comp_t *bc, char *attr, void **buf);
void board_get_ptr_attr_def(board_comp_t *bc, char *attr, void **buf, void *def);
int board_get_int_attr(board_comp_t *bc, char *attr, int *buf);
void board_get_int_attr_def(board_comp_t *bc, char *attr, int *buf, int def);
int board_get_str_attr(board_comp_t *bc, char *attr, char *buf);
void board_get_str_attr_def(board_comp_t *bc, char *attr, char *buf, char *def);
int board_get_str_attr_idx(board_comp_t *bc, char *attr, char *buf, uint8_t index);
void board_get_str_attr_idx_def(board_comp_t *bc, char *attr, char *buf, uint8_t index, char *def);
int board_get_bool_attr(board_comp_t *bc, char *attr, bool *buf);
void board_get_bool_attr_def(board_comp_t *bc, char *attr, bool *buf, bool def);
int board_get_irq_trigger_attr(board_comp_t *bc, char *attr, irq_trigger_mode_t *buf);
void board_get_irq_trigger_attr_def(board_comp_t *bc, char *attr, irq_trigger_mode_t *buf, irq_trigger_mode_t def);
int board_get_component_attr(board_comp_t *bc, char *attr, component_t **buf);
