#include <hal.h>
#include <ch.h>
#include <string.h>
#include "cmp_mem_access/cmp_mem_access.h"
#include "serial-datagram/serial_datagram.h"
#include "communication.h"


static mutex_t send_lock;

static void _stream_values_sndfn(void *arg, const void *p, size_t len)
{
    if (len > 0) {
        chSequentialStreamWrite((BaseSequentialStream*)arg, (const uint8_t*)p, len);
    }
}

static int send_imu(cmp_ctx_t *cmp)
{
    float gyro[3] = {1.1, 2.2, 3.3};
    float acc[3] = {4.4, 5.5, 6.6};
    float t = (float)chVTGetSystemTimeX() / CH_CFG_ST_FREQUENCY;

    //imu_get_gyro(gyro);
    //imu_get_acc(acc);
    bool err = false;

    err = err || !cmp_write_map(cmp, 3);
    const char *gyro_id = "gyro";
    err = err || !cmp_write_str(cmp, gyro_id, strlen(gyro_id));
    err = err || !cmp_write_array(cmp, 3);
    err = err || !cmp_write_float(cmp, gyro[0]);
    err = err || !cmp_write_float(cmp, gyro[1]);
    err = err || !cmp_write_float(cmp, gyro[2]);
    const char *acc_id = "acc";
    err = err || !cmp_write_str(cmp, acc_id, strlen(acc_id));
    err = err || !cmp_write_array(cmp, 3);
    err = err || !cmp_write_float(cmp, acc[0]);
    err = err || !cmp_write_float(cmp, acc[1]);
    err = err || !cmp_write_float(cmp, acc[2]);
    const char *time_id = "time";
    err = err || !cmp_write_str(cmp, time_id, strlen(time_id));
    err = err || !cmp_write_float(cmp, t);
    return err;
}

static THD_WORKING_AREA(comm_tx_stream_wa, 1024);
static THD_FUNCTION(comm_tx_stream, arg)
{
    BaseSequentialStream *out = (BaseSequentialStream*)arg;

    static char dtgrm[100];
    static cmp_mem_access_t mem;
    static cmp_ctx_t cmp;
    while (1) {
        cmp_mem_access_init(&cmp, &mem, dtgrm, sizeof(dtgrm));
        if (send_imu(&cmp) == 0) {
            chMtxLock(&send_lock);
            serial_datagram_send(dtgrm, cmp_mem_access_get_pos(&mem), _stream_values_sndfn, out);
            chMtxUnlock(&send_lock);
        }

        chThdSleepMilliseconds(1000);
    }
}



// static char reply_buf[100];
// static cmp_mem_access_t reply_mem;
// static cmp_ctx_t reply_cmp;

int ping_cb(cmp_ctx_t *cmp, void *arg)
{
    (void)cmp;
    (void)arg;
    // if (!cmp_read_nil(cmp)) {
    //     return -1;
    // }
    // cmp_mem_access_init(&reply_cmp, &reply_mem, reply_buf, sizeof(reply_buf));
    // const char *ping_resp = "ping";
    // if (cmp_write_str(cmp, ping_resp, strlen(ping_resp))) {
    //     chMtxLock(&send_lock);
    //     serial_datagram_send(reply_buf, cmp_mem_access_get_pos(&reply_mem), _stream_imu_values_sndfn, arg);
    //     chMtxUnlock(&send_lock);
    // }
    return 0;
}

void datagram_dispatcher_cb(const void *dtgrm, size_t len, void *arg)
{
    struct dispatcher_entry_s *dispatcher_tab = arg;
    cmp_ctx_t cmp;
    cmp_mem_access_t mem;
    cmp_mem_access_ro_init(&cmp, &mem, dtgrm, len);
    uint32_t map_size;
    if (!cmp_read_map(&cmp, &map_size)) {
        return;
    }
    uint32_t i;
    for (i = 0; i < map_size; i++) {
        uint32_t id_size;
        if (!cmp_read_str_size(&cmp, &id_size)) {
            return;
        }
        size_t str_pos = cmp_mem_access_get_pos(&mem);
        cmp_mem_access_set_pos(&mem, str_pos + id_size);
        const char *str = cmp_mem_access_get_ptr_at_pos(&mem, str_pos);
        while (dispatcher_tab->id != NULL) {
            if (strncmp(dispatcher_tab->id, str, id_size) == 0) {
                if (dispatcher_tab->cb(&cmp, dispatcher_tab->arg) != 0) {
                    return; // parsing error, stop parsing this datagram
                }
                break;
            }
            dispatcher_tab++;
        }
    }
}


static THD_WORKING_AREA(comm_rx_wa, 1024);
static THD_FUNCTION(comm_rx, arg)
{
    struct dispatcher_entry_s dispatcher_table[] = {
        {"ping", ping_cb, arg},
        {NULL, NULL, NULL}
    };
    static serial_datagram_rcv_handler_t rcv_handler;
    static char rcv_buffer[2000];

    chRegSetThreadName("comm rx");

    BaseSequentialStream *in = (BaseSequentialStream*)arg;
    serial_datagram_rcv_handler_init(&rcv_handler,
                                     rcv_buffer,
                                     sizeof(rcv_buffer),
                                     datagram_dispatcher_cb,
                                     dispatcher_table);
    while (1) {
        char c = chSequentialStreamGet(in);
        serial_datagram_receive(&rcv_handler, &c, 1);
    }
}


void communication_start(BaseSequentialStream *out)
{
    chMtxObjectInit(&send_lock);
    chThdCreateStatic(comm_tx_stream_wa, sizeof(comm_tx_stream_wa), LOWPRIO, comm_tx_stream, out);
    chThdCreateStatic(comm_rx_wa, sizeof(comm_rx_wa), LOWPRIO, comm_rx, out);
}
