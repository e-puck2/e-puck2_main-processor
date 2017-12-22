#include <hal.h>
#include <ch.h>
#include <string.h>
#include "cmp_mem_access/cmp_mem_access.h"
#include "serial-datagram/serial_datagram.h"
#include "communication.h"
#include "msgbus/messagebus.h"
#include "sensors/imu.h"
#include "main.h"

/*
 * This module uses C MessagePack (CMP) to form messages and serial_datagram to 
 * delimit the frame and to add to it a crc32 for verification
 * The datagram module adds a CRC32 and an END of frame identifier. It also takes into account 
 * the cases when the frame itself contains the END identifier and escapes it.
 * 
 * Each MessagePack message can be freely composed. It can only contain a number/string/array
 * It is possible to identify what is sent by addind an ID to the datas.
 * 
 * Example : We send the order "ping" and the data of this order is "a message"
 * then the message will look like that {"ping":"a message"}
 * We could add several orders in the same frame. For example here the second order 
 * contains a bool as data : {"ping":"a message", "order2": true }
 * 
 * Now to contruct this message, we need to proceed sequentially.
 * When we want to indentify the datas with an ID, we need to add an information
 * called map at the begining of the message telling how many orders are present
 * 
 * message containing :
 * 1) Only one float data :
 *    cmp_write_float(cmp, float)
 * 2) An array containing a float and a string of datas :
 *    cmp_write_array(cmp, array_size)
 *    cmp_write_float(cmp, float)
 *    cmp_write_str(cmp, string, string_size)
 * 3) An order identified by an ID with an array containg a bool and an int as datas
 *    cmp_write_map(cmp, 1)
 *    cmp_write_str(cmp, ID_string, ID_string_size)
 *    cmp_write_array(cmp, array_size)
 *    cmp_write_bool(cmp, bool)
 *    cmp_write_int(cmp, int)
 * 4) Two orders in the same message. The first contains a bool and the other the same content as example 3
 *    cmp_write_map(cmp, 2)
 *    cmp_write_str(cmp, ID1_string, ID1_string_size)
 *    cmp_write_bool(cmp, bool)
 *    cmp_write_str(cmp, ID2_string, ID2_string_size)
 *    cmp_write_array(cmp, array_size)
 *    cmp_write_bool(cmp, bool)
 *    cmp_write_int(cmp, int)
 *
 *   The same thing goes for the reading of a message. Each function used to read something
 *   in the message moved the pointer to the next element of the message. So once an element of the 
 *   message has been read, it cannot be read anymore.
*/

static mutex_t send_lock;

messagebus_t messagebus;

/*
 * a simple wrapper to the system write function
*/
static void _stream_values_sndfn(void *arg, const void *p, size_t len)
{
    if (len > 0) {
        chSequentialStreamWrite((BaseSequentialStream*)arg, (const uint8_t*)p, len);
    }
}

/*
 * Function to comtruct the message pack frame.
*/
static int send_imu(cmp_ctx_t *cmp, imu_msg_t* imu_values)
{
    
    float t = (float)chVTGetSystemTimeX() / CH_CFG_ST_FREQUENCY;

    //imu_get_gyro(gyro);
    //imu_get_acc(acc);
    bool err = false;

    err = err || !cmp_write_map(cmp, 3);
    const char *gyro_id = "gyro";
    err = err || !cmp_write_str(cmp, gyro_id, strlen(gyro_id));
    err = err || !cmp_write_array(cmp, 3);
    err = err || !cmp_write_float(cmp, imu_values->gyro[0]);
    err = err || !cmp_write_float(cmp, imu_values->gyro[1]);
    err = err || !cmp_write_float(cmp, imu_values->gyro[2]);
    const char *acc_id = "acc";
    err = err || !cmp_write_str(cmp, acc_id, strlen(acc_id));
    err = err || !cmp_write_array(cmp, 3);
    err = err || !cmp_write_float(cmp, imu_values->acceleration[0]);
    err = err || !cmp_write_float(cmp, imu_values->acceleration[1]);
    err = err || !cmp_write_float(cmp, imu_values->acceleration[2]);
    const char *time_id = "time";
    err = err || !cmp_write_str(cmp, time_id, strlen(time_id));
    err = err || !cmp_write_float(cmp, t);
    return err;
}

/*
 * Example thread to send imu datas through messagepack + datagram format
*/
static THD_WORKING_AREA(comm_tx_stream_wa, 1024);
static THD_FUNCTION(comm_tx_stream, arg)
{
    BaseSequentialStream *out = (BaseSequentialStream*)arg;

    messagebus_topic_t *imu_topic = messagebus_find_topic_blocking(&bus, "/imu");
    imu_msg_t imu_values;

    static char dtgrm[100];
    static cmp_mem_access_t mem;
    static cmp_ctx_t cmp;
    while (1) {

        messagebus_topic_wait(imu_topic, &imu_values, sizeof(imu_values));

        cmp_mem_access_init(&cmp, &mem, dtgrm, sizeof(dtgrm));
        if (send_imu(&cmp, &imu_values) == 0) {
            chMtxLock(&send_lock);
            serial_datagram_send(dtgrm, cmp_mem_access_get_pos(&mem), _stream_values_sndfn, out);
            chMtxUnlock(&send_lock);
        }

        chThdSleepMilliseconds(500);
    }
}

static char reply_buf[100];
static cmp_mem_access_t reply_mem;
static cmp_ctx_t reply_cmp;

/*
 * Function that resend the string contained in the message
*/
int ping_cb(cmp_ctx_t *cmp, void *arg)
{
    (void)cmp;
    BaseSequentialStream *out = (BaseSequentialStream*)arg;

    uint32_t size = 100;
    bool err = false;
    char ping_buf[size];

    if( cmp_read_str(cmp, ping_buf, &size)){
        cmp_mem_access_init(&reply_cmp, &reply_mem, reply_buf, sizeof(reply_buf));

        const char *ping_resp = "ping";
        err = err || !cmp_write_map(&reply_cmp, 1);
        err = err || !cmp_write_str(&reply_cmp, ping_resp, strlen(ping_resp));
        err = err || !cmp_write_str(&reply_cmp, ping_buf, strlen(ping_buf));

        if(!err){
            chMtxLock(&send_lock);
            serial_datagram_send(reply_buf, cmp_mem_access_get_pos(&reply_mem), _stream_values_sndfn, out);
            chMtxUnlock(&send_lock);
        }
        
    }

    return 0;
}

/*
 * Function used to dispatch the order. It look a the ID field of the message pack frame
 * and execute the callback correspondant to it
*/
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

/*
 * Thread dedicated to the reading of the frames received
*/
static THD_WORKING_AREA(comm_rx_wa, 1024);
static THD_FUNCTION(comm_rx, arg)
{
    //table containing all the order we must process
    //if a received order is not in this table,
    //it will be dropped
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
