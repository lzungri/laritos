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

#include <log.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sched/core.h>
#include <test/test.h>
#include <test/utils/process.h>
#include <dstruct/circbuf.h>

T(circbuf_correctly_initializes_all_its_data) {
    circbuf_t cb;
    char buf[10] = { 0 };
    circbuf_init(&cb, buf, sizeof(buf));

    tassert(cb.datalen == 0);
    tassert(cb.buf == (void *) buf);
    tassert(cb.size == 10);
    tassert(cb.data_avail_cond.blocked.next == &cb.data_avail_cond.blocked);
    tassert(cb.data_avail_cond.blocked.prev == &cb.data_avail_cond.blocked);
    tassert(cb.space_avail_cond.blocked.next == &cb.space_avail_cond.blocked);
    tassert(cb.space_avail_cond.blocked.prev == &cb.space_avail_cond.blocked);
TEND

static int reader(void *data) {
    circbuf_t *cb = (circbuf_t *) data;
    char buf[5] = { 0 };
    __attribute__((unused)) int nbytes = circbuf_read(cb, buf, sizeof(buf) - 1, true);
    debug("Read %u bytes, Data: %s", nbytes, buf);
    return 0;
}

T(circbuf_reader_blocks_when_no_data_is_available) {
    circbuf_t cb;
    char buf[10] = { 0 };
    circbuf_init(&cb, buf, sizeof(buf));

    pcb_t *p1 = process_spawn_kernel_process("reader", reader, &cb,
                        8196, process_get_current()->sched.priority - 1);
    tassert(p1 != NULL);

    schedule();

    irqctx_t pcbd_ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);
    tassert(p1->sched.status == PROC_STATUS_BLOCKED);
    tassert(is_process_in(&p1->sched.sched_node, &cb.data_avail_cond.blocked));
    spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);

    char buf2[] = "abc";
    circbuf_write(&cb, buf2, sizeof(buf2), true);

    process_wait_for(p1, NULL);
TEND

static int writer(void *data) {
    circbuf_t *cb = (circbuf_t *) data;
    char buf[] = "01";
    circbuf_write(cb, buf, sizeof(buf), true);
    return 0;
}

T(circbuf_writer_blocks_when_there_is_not_enough_space_available) {
    circbuf_t cb;
    char buf[3] = { 0 };
    circbuf_init(&cb, buf, sizeof(buf));

    char buf2[] = "ab";
    circbuf_write(&cb, buf2, sizeof(buf2), true);
    // No more space in the circbuffer

    pcb_t *p1 = process_spawn_kernel_process("writer", writer, &cb,
                        8196, process_get_current()->sched.priority - 1);
    tassert(p1 != NULL);

    sleep(2);

    irqctx_t pcbd_ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);
    tassert(p1->sched.status == PROC_STATUS_BLOCKED);
    tassert(is_process_in(&p1->sched.sched_node, &cb.space_avail_cond.blocked));
    spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);


    circbuf_read(&cb, buf2, 1, true);
    sleep(1);

    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);
    tassert(p1->sched.status == PROC_STATUS_BLOCKED);
    tassert(is_process_in(&p1->sched.sched_node, &cb.space_avail_cond.blocked));
    spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);

    circbuf_read(&cb, buf2, 1, true);
    sleep(1);

    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);
    tassert(p1->sched.status == PROC_STATUS_BLOCKED);
    tassert(is_process_in(&p1->sched.sched_node, &cb.space_avail_cond.blocked));
    spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);

    // After this, there will be enough space for the writer to store the data
    circbuf_read(&cb, buf2, 1, true);

    // Read the data stored by the writer process
    circbuf_read(&cb, buf2, sizeof(buf2), true);
    tassert(strncmp(buf2, "01", sizeof(buf2)) == 0);
TEND
