/**file: app_demo.c *
* 
This file is used as an example to access the private protocol of the user app
*/

#include <lib_app/app.h>
#include <lib_ui/ui.h>
#include <audio_player/audio_player.h>

#include <app_defines.h>
#include <sys_manager.h>
#include <bt_manager.h>

#include <player/player_api.h>

#include "app_demo.h"

#include "./../bt_manager/bt_manager_private.h"
#include "./../bt_manager/bt_manager_private.h"

#define APPDEMO_CODE

//opus
#define APP_OPUS_FRAME_LEN    (40)
#define APP_OPUS_FRAME_NUM    (4)
#define SPP_HEADER_PDLEN_LEN (0x9081)//(0x1083)
#define SEND_VOICE_BUF_SIZE		(160)

enum BLE_APP_CHARACTERISTICS
{
    BLE_APP_NOTIFY = 0,
    BLE_APP_WRITE,
    
    BLE_APP_CHARACTERISTICS_NUM,
};

typedef struct
{
    u8_t   uuid[16];
    u8_t   properties;
    
} ble_app_characteristic_t;

app_context_t*  app_context = NULL;

const ble_app_characteristic_t  ble_app_characteristics[BLE_APP_CHARACTERISTICS_NUM] =
{
    {   /*.uuid = {0xCA, 0x23, 0x59, 0x43, 
        0x18, 0x10, 0x45, 0xE6, 
        0x83, 0x26, 0xFC, 0x8C, 
        0xA3, 0xBC, 0x45, 0xCE,} ,*/
        .uuid = {
        0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, 0xe8, 0x11, 0x9a, 0xf6, 0xe1, 0x28, 0x9a, 0xe4,} ,
        .properties = GATT_PROP_READ | GATT_PROP_NOTIFY | GATT_PROP_EXTENDED_PROPERTIES},
    {   /*.uuid = {0x68, 0x74, 0x53, 0x53, 
        0x18, 0x10, 0x4B, 0x13, 
        0x83, 0xA2, 0xC1, 0xB2, 
        0x1B, 0x65, 0x2C, 0x9B, } ,*/
        .uuid = {
         0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, 0xe8, 0x11, 0x9a, 0xf6, 0xe0, 0x25, 0x9a, 0xe4,} ,
        .properties = GATT_PROP_WRITE | GATT_PROP_WRITE_WITHOUT_RESPONSE},
};
 
void app_va_resource_release(void)
{
	app_context_t*  p = app_context;

	if (!p)
		return;

	p->interupt_opus = 0;
		
	bt_ma_va_stop(p->link_hdl);
	if (p->tx_record_buf.data_buf)
	{
		free(p->tx_record_buf.data_buf);
		memset(&p->tx_record_buf, 0, sizeof(loop_buffer_t));
	}
}

void app_cmd_send(unsigned int is_response)
{
	log_debug();
	// Send private protocol data
	//bt_ma_send_data(p->link_hdl, sbuf, pack_len + sizeof(app_packet_header_t), MA_BLE_CMD);
}


int app_analyze_msg_head(uint8_t* data, int data_len, short* pPayloadOffset, short* pPayloadLen)
{
	// Parse private protocol data
    
    return 0;
}

int app_ota_data_read(void* buf, uint_t len, uint_t timeout_ms)
{
	app_context_t *p = app_context;

	if (!p)
		return 0;
	
	if (!p->ota_read_buf.data_buf)
		return 0;

	if (loop_buffer_read(&p->ota_read_buf,buf,len) != len)
	{
		return 0;
	}

	return len;
}

int app_ota_data_write(void* buf, uint_t len, uint_t timeout_ms)
{
	app_context_t *p = app_context;
	
	if ((!p) || (!buf) || (!len))
	{
		log_error("0x%x 0x%x 0x%x.",p,buf,len);
		return 0;
	}

	if (MA_RTN_OK == bt_ma_send_data(p->link_hdl ,buf, len, MA_BLE_CMD))
		return len;

	return 0;
}

void app_ota_interface_deinit(u32_t status)
{
	OSSchedLock();

	app_context_t *p = app_context;
	u32_t value = status;

	if (!p)
	{
		OSSchedUnlock();
		return;
	}
	
	if (p->ota_read_buf.data_buf)
	{
    	free(p->ota_read_buf.data_buf);
		//sys_manager_api->filter_key(NO);
		if (PRIVMA_STATUS_OTA_RUNING == bt_ma_status_get())
			bt_ma_status_set(PRIVMA_STATUS_LINKED);

		//sys_set_cpufreq_by_module(SET_CPUFREQ_MODULE_SYS_MANAGER_2, 0);
	}

	memset (&p->ota_read_buf, 0 ,sizeof(loop_buffer_t));

	if (OFFLINE_OTA_OK != status)
    	offline_ota_report_status(value);
	
	offline_ota_exit();

	OSSchedUnlock();

	return;
}


void app_ota_interface_init(u8_t cmd)
{
	bt_manager_context_t*  bt_manager = bt_manager_get_context();

	app_context_t *p = app_context;
	CFG_Type_OTA_Settings*	cfg = NULL;
	
    cfg = zalloc(sizeof(CFG_Type_OTA_Settings));
    
    app_config_read
    (
        CFG_ID_SYS_RESERVED_DATA,
        cfg,
        offsetof(CFG_Struct_Sys_Reserved_Data, OTA_Settings),
        sizeof(CFG_Type_OTA_Settings)
    );
	
    if ((bt_manager->bt_common.local_dev_role == TWS_NONE) && 
		(!cfg->Enable_Single_OTA_Without_TWS))
    {
		// app_ota_interface_deinit(OFFLINE_OTA_ERROR);
		free(cfg);
		return;
    }

	free(cfg);

	if (OTA_CMD_H2D_REQUEST_UPGRADE == cmd)
	{
		app_va_resource_release();

		app_ota_interface_deinit(OFFLINE_OTA_RESTART);
		
		if (!p)
			return;
		
		if (p->ota_read_buf.data_buf)
			return;
		
		offline_ota_interface_t scb;
		
		p->ota_read_buf.buf_size = 1024;
		p->ota_read_buf.data_buf = (u8_t *)zalloc(p->ota_read_buf.buf_size);
		
		scb.read = (void*)app_ota_data_read;
		scb.write = (void*)app_ota_data_write;
		offline_ota_start(&scb);
		//sys_manager_api->filter_key(YES);
		//sys_set_cpufreq_by_module(SET_CPUFREQ_MODULE_SYS_MANAGER_2, 144/*180*/);
		bt_ma_status_set(PRIVMA_STATUS_OTA_RUNING);
	}

	return;
}

int ota_recv_process(u8_t * data, u16_t size)
{
	//int status;

	app_context_t *p = app_context;

	if (!p)
		return 0;

	u16_t left_size = size;
	int ret = 0;
	u16_t packet_len;
	int offset = 0;
	
	//if (size < 128)
	//	print_hex("ota recv: ", data, size);

	do
	{	
		if (OFFLINE_OTA_OK != ota_analyze_msg_head(data, left_size, &packet_len, (void *)app_ota_interface_init)) 
		{
			log_warning();
			break;
		}
	
		if (bt_manager_hfp_in_calling(NULL))
		{
			app_ota_interface_deinit(OFFLINE_OTA_CALLING);
		}
		else if (p->ota_read_buf.data_buf)
		{
			if (packet_len != loop_buffer_write(&p->ota_read_buf, data, packet_len))
				log_error();

			ret = offline_ota_cmd_parse();
			if ((OFFLINE_OTA_OK != ret) &&
				(OFFLINE_OTA_END != ret))
			{
				log_error();
				app_ota_interface_deinit(ret);
				return -1;
			}

			if (0 != p->ota_read_buf.data_count)
			{
				log_error();
				app_ota_interface_deinit(OFFLINE_OTA_CALC_LEN_ERROR);
				return -1;
			}

		}
		else
		{
			log_error();
		}
		
		data += packet_len;
		
		left_size -= packet_len;
		offset += packet_len;
		
	} while (left_size > 0);

	return offset;
}

int app_recv_process(u8_t * data, u16_t size)
{
	// Receive private protocol data
	log_debug("");
	return 0;
}

void app_exit(void)
{
	OSSchedLock();

    app_context_t *p = app_context;

    if (p)
    {
        log_debug();
		
        app_va_resource_release();
		
		app_ota_interface_deinit(OFFLINE_OTA_OK);

        bt_ma_status_set(PRIVMA_STATUS_NONE);
		if (p->tx_record_buf.data_buf)
			free(p->tx_record_buf.data_buf);
		p->voice_index = 0;

		if (p->rx_buf)
			free(p->rx_buf);

		p->left_len = 0;
		
        sys_free(p);

        app_context = NULL;
    }

	OSSchedUnlock();
}

void app_disconnected(void)
{
    app_context_t *p = app_context;
    if (p)
    {
		app_exit();
    }
}

/** Event data callback, event enum MA_FEEDBACK_EVENT*/
int app_sppble_cb(uint_t event, u8_t* data, int len)
{
	app_context_t *p = app_context;
	int offset = 0;
	int size = 0;
	u8_t head_error = 0;
	u8_t *ptr = data;
	int real_len = len;
	
	if (!p)
		return FALSE;
	if (MA_ROUTE_CONNECTED == event)
	{
		sys_set_cpufreq_by_module_ex(SET_CPUFREQ_MODULE_SYS_MANAGER_2, 0, 20);
		if (MA_SPP_READY == bt_priv_ma_link_mode())
		{
			bt_ma_status_set(PRIVMA_STATUS_LINKED);
		}	

	} else if (MA_DATA_RECEIVE == event) {

		if (bt_manager_hfp_in_calling(NULL))
		{
			log_warning("");
			app_ota_interface_deinit(OFFLINE_OTA_OK);
			ble_hci_control_disconnect();
			return 0;
		}

		// print_hex(CONST("tma_sppble_cb:"), 
		// 	ptr, (sizeof(tma_pkg_head_s) + payload_len + 1));
		if (data)
			print_hex("rec raw:", data, ((len<128)?len:128));

		// sagereal 20230327 by ccmeta Begin
		int ret = app_cmd_check_off_sagereal(data);
		// sagereal 20230327 by ccmeta End

		if ((p->rx_buf) && (p->left_len))
		{
			log_debug("left_len %d len %d.",p->left_len,len);
			memcpy (p->rx_buf + p->left_len, data, len);
			ptr = p->rx_buf;
			real_len = p->left_len + len;
			p->left_len = 0;
		}

		while (real_len > offset)
		{
			//printf("cp0 status: %x\n", read_c0_status());
			//log_error("real_len %d offset %d.",real_len,offset);
			size = ota_recv_process(ptr + offset, real_len - offset);
			if (-1 == size)
			{
				log_error("");
				break;
			}

			if (0 == size)
			{
				head_error ++;
				if (2 == head_error)
				{
					if ((real_len - offset) > 1024)
					{
						log_error("head error, data discard!");
						p->left_len = 0;
						break;
					}
						
					log_warning("packet split.");
					if (!p->rx_buf)
					{
						p->rx_buf = (u8_t *)zalloc(2048+512);
					}
					
					p->left_len = real_len - offset;
					memcpy (p->rx_buf, ptr + offset, p->left_len);
					log_debug("real_len %d offset %d DATA %x.",real_len,offset,p->rx_buf[0]);

					break;
				}
			}
			else
			{
				head_error = 0;
			}	
			offset += size;
			if (real_len <= offset)
				break;
			
			size = app_recv_process(ptr + offset, real_len - offset);
			if (0 == size)
			{
				head_error ++;
				if (2 == head_error)
				{
					if ((real_len - offset) > 1024)
					{
						log_error("head error, data discard!");
						p->left_len = 0;
						break;
					}

					log_warning("packet split.");
					if (!p->rx_buf)
					{
						p->rx_buf = (u8_t *)zalloc(2048+512);
					}
					
					p->left_len = real_len - offset;
					memcpy (p->rx_buf, ptr + offset, p->left_len);
					log_error("real_len %d offset %d DATA %x.",real_len,offset,p->rx_buf[0]);

					break;
				}
			}
			else
			{
				head_error = 0;
			}
			offset += size;
		}
	} else if (MA_ROUTE_DISCONNECT == event) {
	
		sys_set_cpufreq_by_module_ex(SET_CPUFREQ_MODULE_SYS_MANAGER_2, 0, 0);
		app_disconnected();
	} else if (MA_CCCD_UPGRADE == event) {
		
		if ((MA_BLE_READY == bt_priv_ma_link_mode()) &&
			(PRIVMA_STATUS_NONE == bt_ma_status_get()))
		{
			bt_ma_status_set(PRIVMA_STATUS_LINKED);
		}		
	}
	return 0;
}

/** Used for master-slave switch to save the data that needs to be backed up,
    copy the data that needs to be backed up to wbuf, the backup length is a certain size
 */
int app_do_tws_role_switch(u8_t* wbuf, int wlen)
{
	app_context_t *p = app_context;
	int real_len = 0;
	u16_t pos = 0;
	int left_len = 0;

	if (!p)
	{
		return MA_RTN_ERROR;
	}
	
	_PUT_BUF_U8  (wbuf, pos, p->status);
	
	if ((p->left_len) && (p->rx_buf))
	{
		_PUT_BUF_U32(wbuf, pos, p->left_len);
		_PUT_BUF_DATA(wbuf, pos, p->rx_buf, p->left_len);
	}
	else
	{
		_PUT_BUF_U32(wbuf, pos, left_len);
	}
	
	real_len += pos;
	if (real_len > wlen)
	{
		return 0;
	}

	if (!wbuf)
	{
		return real_len;
	}

	if (wbuf)
		app_exit();

	return real_len;
}

void ota_report_status_fail(void)
{
	app_context_t *p = app_context;
	if (!p)
	{
		log_error("");
		return;
	}

	log_debug("");
    u8_t pl[16];
    u32_t pl_len, pack_len;
  	u8_t *send_buf = NULL;
    svc_prot_head_t head;
	u32_t status = OFFLINE_OTA_ROLE_SWITCH;

    send_buf = (u8_t *)zalloc(32);

    pl_len = 0;
    pack_len = tlv_pack_data(&pl[pl_len], 0x7F, sizeof(u32_t), (u8_t *)&status);
    pl_len += pack_len;
	
    head.svc_id = SERVICE_ID_OTA;
    head.cmd = OTA_CMD_D2H_REPORT_STATUS;
    head.param_type = TLV_TYPE_MAIN;
    head.param_len = pl_len;

    memcpy(send_buf, &head, sizeof(svc_prot_head_t));
    memcpy(send_buf+sizeof(svc_prot_head_t), &pl[0], pl_len);
	print_hex("OFFLINE_OTA_ROLE_SWITCH:", send_buf, sizeof(svc_prot_head_t)+pl_len);

	bt_ma_send_data(p->link_hdl, send_buf, sizeof(svc_prot_head_t)+pl_len, MA_BLE_CMD);
						
	free(send_buf);
						
    return;
}


/** Used for master-slave switch to obtain backup data, data is copied from rbuf*/
int app_on_tws_role_switch(u8_t* rbuf, int rlen)
{
	app_context_t *p = app_context;
	log_debug("rlen %d",rlen);
	u16_t pos = 0;

	_GET_BUF_U8  (rbuf, pos, p->status);
	_GET_BUF_U32 (rbuf, pos, p->left_len);

	if (rbuf)
	{	
		if (PRIVMA_STATUS_OTA_RUNING == bt_ma_status_get())
		{
			app_timer_delay_proc_ex
			(
				 APP_ID_BT_ENGINE, 500,
				 (void*)ota_report_status_fail, NULL
			);
		}
	}
	
	if (p->left_len)
	{
		log_warning("role switch packet split.");
		if (!p->rx_buf)
		{
			p->rx_buf = (u8_t *)zalloc(2048+512);
		}
		_GET_BUF_DATA(rbuf, pos, p->rx_buf, p->left_len);
	}

	return pos;
}

/** Get smart voice data, can be PCM, ADPCM, OPUS encoding format*/
int app_va_data_feedback(u8_t* rbuf, int len)
{
	log_debug("len %d.",len);
	app_context_t *p = app_context;
    u8_t*  send_buf = NULL;
	u16_t	 ch_len   = APP_OPUS_FRAME_LEN * APP_OPUS_FRAME_NUM;
    u16_t    send_len = ch_len + 8;
	
	if (PRIVMA_STATUS_RECORDING != bt_ma_status_get())
	{
		log_warning("");
		return 0;
	}

	if (!p->tx_record_buf.data_buf)
	{
    	p->tx_record_buf.buf_size = 320;//4096;
    	p->tx_record_buf.data_buf = zalloc(p->tx_record_buf.buf_size);
	}
	if (len != loop_buffer_write(&p->tx_record_buf, rbuf, len))
	{
		log_error();
	}

    while (p->tx_record_buf.data_count >= ch_len)
    {
		if (MA_RTN_BUFF_OVER == bt_ma_send_data(p->link_hdl, NULL, 0, MA_BLE_CMD))
		{
			log_warning("PLEASE CHECK buffer");
			break;
		}
		
		if (!send_buf) 
		{
			send_buf = zalloc(send_len);
		}
		
        if (loop_buffer_read(&p->tx_record_buf,send_buf+8,ch_len) != ch_len)
           log_error();
		
		*(uint16_t*)(send_buf) = SPP_HEADER_PDLEN_LEN;
		*(uint16_t*) (send_buf + 2) = ch_len + 4;
		*(uint32_t*) (send_buf + 4) = (++ p->voice_index);								
		bt_ma_send_data(p->link_hdl, send_buf, send_len, MA_BLE_CMD);
	}

	if (send_buf)
		free(send_buf);

	return 0;
}

int appdemo_init_cb(u8_t ble_init)
{
 	CFG_Type_APPDEMO_Settings*	cfg = NULL;
	cfg = zalloc(sizeof(*cfg));

    app_config_read
    (
        CFG_ID_SYS_RESERVED_DATA,
        cfg,
        offsetof(CFG_Struct_Sys_Reserved_Data, APPDEMO_Settings),
        sizeof(*cfg)
    );

    if (!cfg->Enable_APPDEMO)
    {
    	free(cfg);
		return NULL;
	}

	if (0xFF == ble_init)
	{
		free(cfg);
		return (int)appdemo_init_cb;
	}
	
	app_context_t *p = app_context;
	ble_ma_characteristic_t ch;
	bt_ma_cb_t mcb;
	bt_manager_configs_t*  bt_configs = bt_manager_get_configs();
		
	OSSchedLock();
	
	if (!app_context)
	{
		app_context = (app_context_t *)zalloc(sizeof(app_context_t));
	}
	p = app_context;

	mcb.sppble_cb = app_sppble_cb;
	mcb.do_tws_role_switch = app_do_tws_role_switch;
	mcb.on_tws_role_switch = app_on_tws_role_switch;
	mcb.va_data_feedback = app_va_data_feedback;
	bt_ma_cb_set(&p->link_hdl, &mcb);

	if (TRUE == ble_init)
	{
		ch.service = 1;
		ch.properties = 0;
		log_debug("SERVICE UUID 2BYTES");
		ch.uuid_len = 2;
		ch.u.uuid_abv = 0xFDC2;
		bt_ma_uuid_ble_set(p->link_hdl, &ch);
		ch.service = 0;
		ch.properties = ble_app_characteristics[0].properties;
		ch.uuid_len = 16;
		memcpy(ch.u.uuid_all, &ble_app_characteristics[0].uuid[0], 16);
		bt_ma_uuid_ble_set(p->link_hdl, &ch);
		ch.service = 0;
		ch.properties = ble_app_characteristics[1].properties;
		ch.uuid_len = 16;
		memcpy(ch.u.uuid_all, &ble_app_characteristics[1].uuid[0], 16);
		bt_ma_uuid_ble_set(p->link_hdl, &ch);
	}
	
	memcpy(p->mac_addr, bt_configs->bt_dev_addr, 6);
	OSSchedUnlock();

    log_debug();
	free(cfg);

	return 0;
}

void app_upload_speech_stop(void)
{
	log_debug("local stop speech.");
}

void app_upload_speech_stop_request(void)
{
	log_debug("");
	app_upload_speech_stop();
	app_va_resource_release();
}

void app_upload_speech_request(void)
{
	app_context_t *p = app_context;

	if (!p)
	{
		log_error("");
		return;
	}

	if (bt_manager_hfp_in_calling(NULL))
	{
		log_error();
		return;
	}

	if (PRIVMA_STATUS_RECORDING == bt_ma_status_get())
	{
		// Interrupt wake up
		app_cmd_send(FALSE);
		p->interupt_opus = 1;
		bt_ma_va_restart_init(p->link_hdl);
	}
	else 
	{
		p->voice_index = 0;
		p->interupt_opus = 0;
		va_start_t va_st;
		va_st.codec = MA_OPUS_CODEC_NO_HEAD;
		va_st.end_time = 15;
		va_st.va_end_func = (void *)app_upload_speech_stop_request;
		
		if (MA_RTN_OK == bt_ma_va_start(p->link_hdl, &va_st, NULL))
		{
			/* send command to app */
			app_cmd_send(FALSE);
		}
	}

	log_debug("11local wakeup \n");
}
void app_talk_start(void)
{
    app_context_t *p = app_context;
		
	//app_talk_end();

    if (p &&
        bt_ma_status_get() >= PRIVMA_STATUS_LINKED)
    {
        log_debug();
		//if (0 == opus_break)
		{
			app_upload_speech_request();
		}
        // bt_ma_va_start(p->xya_hdl, MA_ADPCM_CODEC, NULL);
    }
}

void app_talk_end(void)
{
    app_context_t *p = app_context;

    if (p != NULL &&
        bt_ma_status_get() >= PRIVMA_STATUS_RECORDING)
    {
        log_debug();

        if (bt_ma_status_get() == PRIVMA_STATUS_RECORDING)
        {
			app_upload_speech_stop();  // 
            app_va_resource_release();
        }
    }
}

// sagereal 20230327 by ccmeta Begin
int app_cmd_check_off_sagereal(u8_t *data)
{
		log_debug("ccmeta INTO app_cmd_check_off_sagereal");
		log_debug("ccmeta u8_t *data = %d  ", sizeof(data));

		int i = 0;
		while (i < sizeof(data))
		{
				log_debug("ccmeta %x", data[i]);
				i++;
		}
		log_debug("ccmeta DONE WHILE ");

		u8_t *ptr = data;
		u8_t *ptr_off = "OFF";//{0x4f, 0x46, 0x46, 0x0a};
		if (strncmp(ptr, ptr_off, strlen(ptr_off)) == 0)
		{
				log_debug("ccmeta INTO ptr == ptr_off");
				return sys_manager_power_off();
		} else {
				log_debug("ccmeta INTO ptr != ptr_off");
		}
		return 0;
}

int app_sppble_cb_sagereal(uint_t event, u8_t *data, int len)
{
		app_context_t *p = app_context;
		int offset = 0;
		int size = 0;
		u8_t head_error = 0;
		u8_t *ptr = data;
		int real_len = len;

		if (!p)
				return FALSE;
		if (MA_ROUTE_CONNECTED == event)
		{
				sys_set_cpufreq_by_module_ex(SET_CPUFREQ_MODULE_SYS_MANAGER_2, 0, 20);
				if (MA_SPP_READY == bt_priv_ma_link_mode())
				{
						bt_ma_status_set(PRIVMA_STATUS_LINKED);
				}
		}
		else if (MA_DATA_RECEIVE == event)
		{
				if (bt_manager_hfp_in_calling(NULL))
				{
						log_warning("");
						app_ota_interface_deinit(OFFLINE_OTA_OK);
						ble_hci_control_disconnect();
						return 0;
				}

				// print_hex(CONST("tma_sppble_cb:"),
				// 	ptr, (sizeof(tma_pkg_head_s) + payload_len + 1));
				if (data)
						print_hex("rec raw:", data, ((len < 128) ? len : 128));
				// [4F,46,46] == OFF

				if ((p->rx_buf) && (p->left_len))
				{
						log_debug("left_len %d len %d.", p->left_len, len);
						memcpy(p->rx_buf + p->left_len, data, len);
						ptr = p->rx_buf;
						real_len = p->left_len + len;
						p->left_len = 0;
				}

				while (real_len > offset)
				{
						log_debug("INTO while (real_len > offset)");
						// printf("cp0 status: %x\n", read_c0_status());
						// log_error("real_len %d offset %d.",real_len,offset);
						size = ota_recv_process(ptr + offset, real_len - offset);
						if (-1 == size)
						{
				log_error("");
				break;
						}

						if (0 == size)
						{
				head_error++;
				if (2 == head_error)
				{
					if ((real_len - offset) > 1024)
					{
						log_error("head error, data discard!");
						p->left_len = 0;
						break;
					}

					log_warning("packet split.");
					if (!p->rx_buf)
					{
						p->rx_buf = (u8_t *)zalloc(2048 + 512);
					}

					p->left_len = real_len - offset;
					memcpy(p->rx_buf, ptr + offset, p->left_len);
					log_debug("real_len %d offset %d DATA %x.", real_len, offset, p->rx_buf[0]);

					break;
				}
						}
						else
						{
				head_error = 0;
						}
						offset += size;
						if (real_len <= offset)
				break;

						size = app_recv_process(ptr + offset, real_len - offset);
						if (0 == size)
						{
				head_error++;
				if (2 == head_error)
				{
					if ((real_len - offset) > 1024)
					{
						log_error("head error, data discard!");
						p->left_len = 0;
						break;
					}

					log_warning("packet split.");
					if (!p->rx_buf)
					{
						p->rx_buf = (u8_t *)zalloc(2048 + 512);
					}

					p->left_len = real_len - offset;
					memcpy(p->rx_buf, ptr + offset, p->left_len);
					log_error("real_len %d offset %d DATA %x.", real_len, offset, p->rx_buf[0]);

					break;
				}
						}
						else
						{
				head_error = 0;
						}
						offset += size;
				}
		}
		else if (MA_ROUTE_DISCONNECT == event)
		{

				sys_set_cpufreq_by_module_ex(SET_CPUFREQ_MODULE_SYS_MANAGER_2, 0, 0);
				app_disconnected();
		}
		else if (MA_CCCD_UPGRADE == event)
		{

				if ((MA_BLE_READY == bt_priv_ma_link_mode()) &&
						(PRIVMA_STATUS_NONE == bt_ma_status_get()))
				{
						bt_ma_status_set(PRIVMA_STATUS_LINKED);
				}
		}
		return 0;
}

void app_context_init_sagereal(void)
{
		bt_ma_cb_t mcb;

		if (!app_context)
		{
				app_context = (app_context_t *)zalloc(sizeof(app_context_t));
				mcb.sppble_cb = app_sppble_cb_sagereal;
				mcb.do_tws_role_switch = app_do_tws_role_switch;
				mcb.on_tws_role_switch = app_on_tws_role_switch;
				mcb.va_data_feedback = app_va_data_feedback;
				bt_ma_cb_set(&app_context->link_hdl, &mcb);
		}
}

extern int app_cmd_send_sagereal(void)
{
		log_debug();
		app_context_init_sagereal();
		// bt_manager_context_t *bt_manager = bt_manager_get_context();

		app_context_t *p = app_context;
		u8_t fuck_buf[15] = {49, 49, 48, 49, 49, 54, 48, 48, 48, 48, 48, 48, 48, 48,'\0'};
		u8_t *normal_buf = "11011600000000";
		u8_t *low_power_buf = "13161600000000";
		u8_t *buf = fuck_buf;

		float battary = 2.3f;
		if(battary <= 2.2f){
			// this if condition is for low power mode when battary < 2.4V
				buf = low_power_buf;
		}

		int len = strlen(buf);

		if ((!p) || (!buf) || (!len))
		{
				log_error("0x%x 0x%x 0x%x.", p, buf, len);
				return 0;
		}

		if (MA_RTN_OK == bt_ma_send_data(p->link_hdl, buf, len, MA_BLE_CMD))
		{
				log_debug("app_cmd_send_110 success");
				return -1;
		}
		return 0;
}
// sagereal 20230327 by ccmeta End
