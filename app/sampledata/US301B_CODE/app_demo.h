
#ifndef __APPDEMO_H__
#define __APPDEMO_H__

#include <btengine/upgrade_offline_ota.h>
#include <btengine/bt_priv_ma.h>
#include <btengine/ota_tws_transmit.h>

typedef struct
{
	u8_t           status;
    u8_t           mac_addr[6];
	
	void *link_hdl;
	loop_buffer_t  tx_record_buf;

	u8_t  *rx_buf;
	int left_len;
	/******************************/
	u32_t voice_index;
	u8_t  interupt_opus;

    /***********ota start************/
	loop_buffer_t  ota_read_buf;
    /***********ota end************/
} app_context_t;

void app_va_resource_release(void);
void app_upload_speech_stop_request(void);
void app_upload_speech_request(void);

/*************************/
int appdemo_init_cb(u8_t ble_init);
void app_talk_start(void);
void app_talk_end(void);
// sagereal 20230327 by ccmeta Begin
int app_cmd_check_off_sagereal(u8_t *data);
void app_context_init_sagereal(void);
int app_cmd_send_sagereal(void);
// sagereal 20230327 by ccmeta End
/*************************/

#endif /* __APPDEMO_H__ */

