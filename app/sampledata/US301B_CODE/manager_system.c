/*!
 * \file      manager_system.c
 * \brief     System manage interface
 * \details   
 * \author    
 * \date      
 * \copyright Actions
 */

#include "manager_app.h"
#include "./../app_demo/app_demo.h"

#include <btengine/vm_def.h>

#define SAGEREAL_ON true

#ifdef SUPPORT_A2DP_DELAY_REPORT
#define DELAY_REPORT_BENCHMARK (30)//(290(�ϴε�����ֵ)-200(��������ʱ)-50(Ԥ��ֵ))
extern u16_t BluetoothMusic_delay_ms_get(void);
extern bt_dev_t *RmtFindLinkedDevByBD(uint8* bd,uint8 *cur_index);
extern void APP_DelayReporting_Set(bt_dev_t *p_dev,uint16 delay_value);
#endif

#ifdef FASTPAIR_SUPPORT
typedef struct {
    uint8_t left_battery;
    uint8_t right_battery;
    uint8_t case_battery;
} manager_battery_value_t;

manager_battery_value_t battery_last;

u8_t    gfp_bat_timer = 0;
#endif

int Vram_Map_Data_Pack(UINT32 writable_bytes);

void bt_manager_hfp_bat_report_ex(bt_manager_dev_info_t* dev_info);


/* NVRAM disk initialize
 */
#if 1
void manager_system_vdisk_init(void)
{
    static const u8_t  uncached_items[] =
    {
        VFD_BT_DEV_NAME,
        VFD_BT_DEV_ADDR,
        VFD_HOSC_CAP_ADJUST,

        VFD_TWS_SHARED_ADDR,
        VFD_TWS_SAVED_LIST,
        VFD_AUDIO_CHANNEL,

        VM_BTENGINE,
        VM_BT_DEV_CFG,
        VM_BT_PLIST0,
        VM_BT_PLIST1,
        VM_BT_PLIST2,
        VM_BT_PLIST3,
        VM_BT_PLIST4,
        VM_BT_PLIST5,
        VM_BT_PLIST6,
        VM_BT_PLIST7,
        VM_BT_PLIST8,
        VM_BT_TWS_INFO,
    };
    
    CFG_Struct_VDISK_Driver  cfg_vdisk;
    
    app_config_read
    (
        CFG_ID_VDISK_DRIVER, 
        &cfg_vdisk, 0, sizeof(CFG_Struct_VDISK_Driver)
    );

    log_debug("%d, %d", cfg_vdisk.Sector_Number, cfg_vdisk.Sector_Size);

    Vram_Initial_Fix(cfg_vdisk.Sector_Number, cfg_vdisk.Sector_Size);

    vdisk_set_uncached_items(uncached_items, sizeof(uncached_items));
}
#endif


/* Check key function map
 */
#if 1
void manager_check_key_func_map(void)
{
    sys_manager_status_t*  sys_status = sys_manager_get_status();
    
    void*  key_func;

    /* Power off
     */
    key_func = NULL;

    if (!sys_status->in_front_charge)
    {
        key_func = sys_manager_power_off;
    }

    ui_key_func_map(KEY_VIEW_SYS_MANAGER, KEY_FUNC_POWER_OFF, key_func);
    
    key_func = NULL;
    
    if (!sys_status->in_front_charge &&
        !sys_status->in_ota_update)
    {
        key_func = manager_switch_low_latency_mode;
    }
    
    ui_key_func_map(KEY_VIEW_SYS_MANAGER, KEY_FUNC_SWITCH_LOW_LATENCY_MODE, key_func);
    
    /* Switch TTS voice language
     */
    key_func = NULL;

    if (!sys_status->in_front_charge &&
        !sys_status->in_ota_update)
    {
        key_func = sys_switch_voice_language;
    }

    ui_key_func_map(KEY_VIEW_SYS_MANAGER, KEY_FUNC_SWITCH_VOICE_LANG, key_func);
    
    /* Clear paired list (In front charge)
     */
    key_func = NULL;

    if (sys_status->in_front_charge)
    {
        key_func = bt_manager_clear_paired_list;
    }

    ui_key_func_map
    (
        KEY_VIEW_SYS_MANAGER, 
        KEY_FUNC_CLEAR_PAIRED_LIST_IN_FRONT_CHARGE, key_func
    );

#ifdef SAGEREAL_ON

    /* Send message to APP client , with KEY_FUNC_CUSTOMED_1
     */
    key_func = NULL;
    key_func = app_cmd_send_sagereal;

    ui_key_func_map(
        KEY_VIEW_SYS_MANAGER,
        KEY_FUNC_CUSTOMED_1, key_func
    );

#endif

#ifdef CONFIG_MICREALTIME
    key_func = NULL;
    if (sys_is_transparency_enable() == YES)
    {
        key_func = sys_switch_transparency_mode;
    }

    ui_key_func_map
    (
        KEY_VIEW_SYS_MANAGER, KEY_FUNC_TRANSPARENCY_MODE, key_func
    );

    key_func = NULL;
    if (sys_is_transparency_enable() == YES)
    {
        key_func = sys_switch_transparency_gain;
    }

    ui_key_func_map
    (
        KEY_VIEW_SYS_MANAGER, KEY_FUNC_TRANSPARENCY_GAIN, key_func
    );
#endif

#if 0
    ui_key_func_map(KEY_VIEW_SYS_MANAGER, 
        KEY_FUNC_RESTORE_FACTORY_SETTINGS, (void*)sys_restore_factory_settings
    );
#endif
}
#endif

#ifndef APP_FIX_ENABLE
void manager_system_view_init(void)
{
    ui_view_info_t  view_info;

    memset(&view_info, 0, sizeof(view_info));

    /* Key view
     */
    view_info.flags = UI_FLAG_KEY_VIEW;
    
    ui_view_create(KEY_VIEW_SYS_MANAGER, &view_info);

    /* Key function map
     */
    manager_check_key_func_map();

    /* LED view
     */
    view_info.flags = UI_FLAG_LED_VIEW;
    
    ui_view_create(LED_VIEW_SYS_MANAGER, &view_info);
    
    /* System events notify view
     */
    ui_view_create(LED_VIEW_SYS_NOTIFY, &view_info);
}
#endif


/* System manage initialize
 */
#if 1
void manager_system_init(void)
{
    sys_manager_configs_t*  sys_configs = sys_manager_get_configs();
    
    audio_manager_configs_t*  audio_configs = audio_manager_get_configs();

    /* TTS voice language
     */
    if (vdisk_read(VFD_VOICE_LANGUAGE, 
            &audio_configs->select_voice_language, sizeof(u8_t)) < 0)
    {
        audio_configs->select_voice_language = 
            sys_configs->cfg_sys_settings.Default_Voice_Language;
    }

    /* View initialize
     */
    manager_system_view_init();
    
    app_timer_create(100, (void*)manager_check_dc5v_in_reset, NULL);
}
#endif


#ifndef APP_FIX_ENABLE
void manager_powon_cpufreq_restore(void)
{
    sys_set_cpufreq_by_module(SET_CPUFREQ_MODULE_SYS_MANAGER_1, 0);
}
#endif


#ifndef APP_FIX_ENABLE
void manager_powon_cpufreq_boost(void)
{
    sys_set_cpufreq_by_module(SET_CPUFREQ_MODULE_SYS_MANAGER_1, 144);

    app_timer_delay_proc(3000, (void*)manager_powon_cpufreq_restore, NULL);
}
#endif


/* Set power on registers
 */
#ifndef APP_FIX_ENABLE
void manager_config_power_on_regs(void)
{
    CFG_Struct_Power_On_Registers*  cfg_regs;

    u8_t  cpufreq_base;

    cfg_regs = sys_malloc(sizeof(CFG_Struct_Power_On_Registers));
    
    app_config_read
    (
        CFG_ID_POWER_ON_REGISTERS, 
        cfg_regs, 0, sizeof(CFG_Struct_Power_On_Registers)
    );

    sys_ctrl_config_regs(cfg_regs->Register, CFG_MAX_POWER_ON_REGS, NO);

    sys_free(cfg_regs);

    /* CPU initial frequency
     */
    cpufreq_base = get_cpu_clk_khz() / 1000;

    print_debug("<D> cpufreq_base: %d\n", cpufreq_base);

    sys_set_cpufreq_by_module(SET_CPUFREQ_MODULE_BASE, cpufreq_base);

    /* Boost CPU frequency when power on
     */
    manager_powon_cpufreq_boost();
}
#endif


/* Auto adjust HOSC capacity by temperature
 */
#ifndef APP_FIX_ENABLE
void manager_cap_temp_comp(void)
{
    CFG_Struct_Cap_Temp_Comp  configs;
    
    app_config_read
    (
        CFG_ID_CAP_TEMP_COMP, 
        &configs, 0, sizeof(CFG_Struct_Cap_Temp_Comp)
    );

    cap_temp_comp_init(&configs);
}
#endif


/* Check save settings
 */
#ifndef APP_FIX_ENABLE
void manager_check_save_settings(void)
{
    manager_app_context_t*  manager = manager_app_get_context();

    audio_volume_ctrl_t*  curr_vol_ctrl = &manager->sys_status.vol_ctrl;

    /* Check save volume settings
     */
    if (memcmp(&manager->last_vol_ctrl, 
            curr_vol_ctrl, sizeof(audio_volume_ctrl_t)) != 0)
    {
        manager->last_vol_ctrl = *curr_vol_ctrl;
        
        vdisk_write
        (
            VFD_AUDIO_VOLUME, curr_vol_ctrl, sizeof(audio_volume_ctrl_t)
        );
    }
}
#endif


void manager_bt_ctrl_test_routine(void)
{
    CFG_Type_Auto_Quit_BT_Ctrl_Test  cfg;

    log_debug();
    
    app_config_read
    (
        CFG_ID_SYS_RESERVED_DATA,
        &cfg,
        offsetof(CFG_Struct_Sys_Reserved_Data, Auto_Quit_BT_Ctrl_Test),
        sizeof(CFG_Type_Auto_Quit_BT_Ctrl_Test)
    );
    
    while (1)
    {
        if (cfg.Quit_Timer_Sec != 0 &&
            get_tick_msec() > cfg.Quit_Timer_Sec * 1000)
        {
            if (cfg.Power_Off_After_Quit)
            {
                sys_manager_ctrl_reboot_ex(SYS_REBOOT_POWER_OFF);
            }
            else
            {
                sys_ctrl_reboot();
            }
        }

        app_timer_handler((id_t)manager_check_dc5v_in_reset);
        
        sched_yield();
    }
}

#ifdef FASTPAIR_SUPPORT
void manager_battery_bat_report(manager_battery_value_t *battery)
{
	audio_manager_configs_t*  cfg = audio_manager_get_configs();
	u8_t  dev_channel = (cfg->hw_sel_dev_channel | cfg->sw_sel_dev_channel);
	bt_manager_dev_info_t*	tws_dev = 
		bt_manager_get_dev_info(BT_MANAGER_TWS_PAIRED_DEV);
	u8_t bat[3];
	manager_app_context_t*	manager = manager_app_get_context();

	memset(bat, 0, 3);
	bat[0] = bt_manager_tws_get_bat_level() & 0x7f;
	if (tws_dev)
	{
		bat[1] = tws_dev->tws_remote_bat_level & 0x7f;
	}
	bat[2] = (sys_manager_api->get_status()->charger_box_bat_level&0x7f);

	if (dev_channel == AUDIO_DEVICE_CHANNEL_R)
	{
		_SWAP(&bat[0], &bat[1], u8_t);
	}

	//log_debug("bat %d %d %d", bat[0],bat[1],bat[2]);
	if (dev_channel == AUDIO_DEVICE_CHANNEL_L)
	{
		bat[0] = (manager->sys_status.in_charger_box << 7) | bat[0];
		if (tws_dev)
		{
			bat[1] =(tws_dev->tws_remote_in_charger_box << 7) | bat[1];
		}
	}
	else
	{
		bat[1] = (manager->sys_status.in_charger_box << 7) | bat[1];
		if (tws_dev)
		{
			bat[0] =(tws_dev->tws_remote_in_charger_box << 7) | bat[0];
		}
	}
	bat[2] |= (sys_manager_api->get_status()->charger_box_bat_level & 0x80);
	battery->left_battery = bat[0];
	battery->right_battery = bat[1];
	battery->case_battery = bat[2];
	//if (0 == battery->case_battery)
	//	battery->case_battery = 0x7F;
}

void bt_manager_app_status_notify(void)
{
	manager_battery_value_t cur_battery;

	bt_manager_context_t*  bt_manager = bt_manager_get_context();


	if (bt_manager->bt_common.local_dev_role == TWS_SLAVE)
		return;

	if (bt_manager_get_phone_dev_num(YES) == 0)
		return;

	if (TRUE == manager_overseas_edition())
	{
		bt_gfp_ble_info_upgade();
	}
	manager_battery_bat_report(&cur_battery);
	if (0 == memcmp(&cur_battery, &battery_last, sizeof(manager_battery_value_t)))
		return;

	log_debug("bat %d %d %d", 
		cur_battery.left_battery,
		cur_battery.right_battery,
		cur_battery.case_battery);

	memcpy(&battery_last, &cur_battery, sizeof(manager_battery_value_t));

	if (TRUE == manager_overseas_edition())
	{
		if (gfp_bat_timer)
			sys_manager_delete_timer(&gfp_bat_timer);

		gfp_bat_timer = app_timer_delay_proc_ex
		(
			APP_ID_BT_ENGINE, 1500,
			(void*)battery_value_change_deal, (void *)NULL
		);
	}
}
#endif

#ifdef SUPPORT_A2DP_DELAY_REPORT
void bt_manager_delayreport_notify(void)
{
	bt_manager_context_t*  bt_manager = bt_manager_get_context();
	bt_manager_dev_info_t* dev_info;
	bt_dev_t*  dev = NULL;
	uint8	   index = 0;
	u16_t delay_ms;
	
	manager_app_context_t*  manager = manager_app_get_context();
	
	if (bt_manager->bt_common.local_dev_role == TWS_SLAVE)
		return;

    if (!audio_media_is_playing())
        return;

    if (bt_manager_hfp_in_calling(NULL))
        return;

    /* TWS master put in charger box
     */
    if (manager->sys_status.in_charger_box)
    {
      return;
    }

	if (bt_manager_get_phone_dev_num(YES) == 0)
		return;

    dev_info = bt_manager_get_dev_info(BT_MANAGER_A2DP_ACTIVE_DEV);
    if (!dev_info)
        return;


    dev = RmtFindLinkedDevByBD((u8_t *)dev_info->bd_addr, &index);
    if ((!dev) || (!dev->dev_sub_info))
    {
        //log_error("dev %p, dev->dev_sub_info %p",dev,dev->dev_sub_info);
        return;
    }

    delay_ms = BluetoothMusic_delay_ms_get();
    log_debug("delay %d ms.", delay_ms);
    if (delay_ms > 300)
        delay_ms = 300;

    APP_DelayReporting_Set(dev, DELAY_REPORT_BENCHMARK + delay_ms);
}
#endif

/* Common timer process
 */
#if 1
void manager_common_timer_proc(void)
{
    manager_app_context_t*  manager = manager_app_get_context();

    manager->common_timer_count += 1;
    
    /* Check save settings
     */
    if ((manager->common_timer_count % (1000 / MANAGER_COMMON_TIMER_MS)) == 0)
    {
        manager_check_save_settings();

        bt_manager_save_dev_volume();
    }

#ifdef SUPPORT_A2DP_DELAY_REPORT
    if (0 == manager->common_timer_count % 30) // 3s�ϱ�һ��delayreport
    {
        app_timer_delay_proc_ex
        (
            APP_ID_BT_ENGINE, 0,
            (void*)bt_manager_delayreport_notify, (void *)NULL
        );
    }
#endif

#ifdef FASTPAIR_SUPPORT
	bt_manager_app_status_notify();
#endif
    bt_manager_hfp_bat_report_ex(NULL);

    /* Check auto standby and auto power off
     */
    manager_check_auto_standby_ex();

    /* Check if enter front charge mode
     */
    manager_check_front_charge();

    #if 1
    /* Check role switch when TWS master put in charger box
     */
    if ((manager->common_timer_count % (2000 / MANAGER_COMMON_TIMER_MS)) == 0)
    {
        manager_check_in_charger_box();
    }
    #endif

    #if 1
    /* Check TWS battery balance
     */
    if ((manager->common_timer_count % (5000 / MANAGER_COMMON_TIMER_MS)) == 0)
    {
        bt_manager_tws_check_bat_balance();
    }
    #endif
}
#endif


/* Check system reboot mode
 */
#ifndef APP_FIX_ENABLE
uint_t manager_check_reboot_mode(void)
{
    sys_manager_status_t*  sys_status = sys_manager_get_status();
    
    uint_t  reboot_mode = NONE;
    
    if (vdisk_read(VFD_SYS_REBOOT_MODE, &reboot_mode, sizeof(u32_t)) < 0)
    {
        reboot_mode = NONE;
    }

    if (reboot_mode != NONE)
    {
        uint_t  tmp = NONE;

        log_debug("0x%x", reboot_mode);

        /* Clear the last saved reboot mode
         */
        vdisk_write(VFD_SYS_REBOOT_MODE, &tmp, sizeof(u32_t));
    }

    sys_status->sys_reboot_mode = reboot_mode;

    return reboot_mode;
}
#endif


/* mode : tAAP_SENSOR_CMD_ID_Code
 */
#ifndef APP_FIX_ENABLE
void manager_aap_sensor_ctrl(u8_t mode)
{
    log_debug("%d", mode);
}
#endif


/* mode : tAAP_Noise_Ctrl_ID_Code
 */
#if 1
void manager_aap_noise_ctrl(u8_t mode)
{
    log_debug("%d", mode);

#ifdef CONFIG_MICREALTIME
    if (sys_is_transparency_enable() == YES)
    {
        if (mode == 3)
        {
            sys_tws_switch_transparency_mode(TRUE, YES, NO);
        }
        else if (mode == 2)
        {
            sys_tws_switch_transparency_mode(FALSE, NO, NO);
        }
        else
        {
            sys_tws_switch_transparency_mode(FALSE, YES, NO);
        }
    }
#endif
}
#endif


#if 1
bool_t manager_aap_ctrl_init(void)
{
    // bt_manager_configs_t*  bt_configs = bt_manager_get_configs();
    
    bt_manager_aap_settings_t  settings;
    bool_t need_aap_noise_ctrl = YES;
    
    if (!bt_manager_aap_is_enabled())
    {
        return NO;
    }

    /* config by CFG_Struct_TWS_Pair.Match_Mode */
#if 0
    /* Force TWS match by device ID when AAP is enabled
     */
    bt_configs->cfg_bt_engine.tws_filtering_rules = TWS_MATCH_ID;
#endif

    if (vdisk_read(VFD_AAP_SETTINGS, &settings, sizeof(settings)) < 0)
    {
        memset(&settings, 0, sizeof(settings));
    }

    manager_aap_sensor_ctrl(settings.sensor_ctrl.mode);

#ifdef CONFIG_MICREALTIME
    if (sys_is_transparency_enable() == YES)
    {
        need_aap_noise_ctrl = NO;
    }
#endif

    if (need_aap_noise_ctrl == YES)
    {
        manager_aap_noise_ctrl(settings.noise_ctrl.mode);
    }

    return YES;
}
#endif


void manager_switch_low_latency_mode(void)
{
    bool_t  low_latency_mode = audio_get_low_latency_mode();
    uint_t  notify_event = SYS_EVENT_NORMAL_LATENCY_MODE;
    uint16_t fade_param[2];
    u8_t  need_fadein_flag = 0;

    bt_manager_context_t*  bt_manager = bt_manager_get_context();

    // [forSearchBtCallNoLLM] bt call has no low latency mode(LLM)
    if (audio_bt_call_is_playing())
    {
        log_debug("call abort");
        return ;
    }

    low_latency_mode ^= 1;
    if (low_latency_mode)
    {
        notify_event = SYS_EVENT_LOW_LATENCY_MODE;
    }

    // 1. Fadeout music
    if ((sys_manager_check_notify_tone(notify_event) == YES) \
     && audio_bt_music_is_playing())
    {
        need_fadein_flag = 1;
        fade_param[0] = 2;      // fadeout
        fade_param[1] = 0;      // use cfg fadeout time
        if (bt_manager->bt_common.local_dev_role != TWS_NONE)
        {
            bt_manager_tws_send_event_ex(TWS_EVENT_BT_MUSIC_FADE, (void*)fade_param, sizeof(fade_param), YES);
        }
        audio_bt_music_set_fade(fade_param[0], fade_param[1]);

        // wait for fadeout finish
        while (audio_bt_music_get_fade_status() == fade_param[0])
        {
            manager_wait_proc();
            sched_yield();
        }
    }

#if 0
    // 2. Play tone or tts
    if (bt_manager->bt_common.local_dev_role != TWS_NONE)
    {
        bt_manager_tws_send_notify(notify_event, YES);
    }
    sys_manager_event_notify(notify_event, LED_VIEW_SYS_NOTIFY, YES);

    // 3. Swith latency mode
    if (bt_manager->bt_common.local_dev_role != TWS_NONE)
    {
        bt_manager_tws_send_event
        (
            TWS_EVENT_SYNC_LOW_LATENCY_MODE, low_latency_mode, YES
        );
    }

    log_debug("%d", low_latency_mode);
    audio_set_low_latency_mode(low_latency_mode);
#else
    // 2. tws sync latency mode and do notify at the same time
    if (bt_manager->bt_common.local_dev_role != TWS_NONE)
    {
        fade_param[0] = notify_event;
        fade_param[1] = (u16_t)low_latency_mode;
        bt_manager_tws_send_event_ex(TWS_EVENT_SYNC_LCYMODE_NOTITY, fade_param, sizeof(fade_param), YES);
    }
    sys_manager_event_notify(notify_event, LED_VIEW_SYS_NOTIFY, YES);
    log_debug("%d", low_latency_mode);
    audio_set_low_latency_mode(low_latency_mode);
#endif

#if 0
    // BluetoothMusic will do fadein as soon as swith latency mode finished, otherwise do it below
    // 4. Fadein music
    if (need_fadein_flag > 0)
    {
        fade_param[0] = 1;      // fadein
        fade_param[1] = 350;    // 350ms
        if (bt_manager->bt_common.local_dev_role != TWS_NONE)
        {
            bt_manager_tws_send_event_ex(TWS_EVENT_BT_MUSIC_FADE, (void*)fade_param, sizeof(fade_param), YES);
        }
        audio_bt_music_set_fade(fade_param[0], fade_param[1]);
    }
#endif
}


/*!
 * \brief Get system manager status
 */
#ifndef APP_FIX_ENABLE
extern sys_manager_status_t* sys_manager_get_status(void)
{
    manager_app_context_t*  manager = manager_app_get_context();
    
    return &manager->sys_status;
}
#endif


/*!
 * \brief Enable or Disable tone and voice play
 */
#if 1
extern void sys_manager_enable_tone_voice(bool_t enable)
{
    sys_manager_status_t*  sys_status = sys_manager_get_status();

    OSSchedLock();

    if (enable)
    {
        if (sys_status->disable_audio_tone > 0)
        {
            sys_status->disable_audio_tone -= 1;
        }

        if (sys_status->disable_audio_voice > 0)
        {
            sys_status->disable_audio_voice -= 1;
        }
    }
    else
    {
        sys_status->disable_audio_tone  += 1;
        sys_status->disable_audio_voice += 1;
    }

    print_debug("<D> disable_tone_voice: %d, %d\n", 
        sys_status->disable_audio_tone, 
        sys_status->disable_audio_voice);

    OSSchedUnlock();
}
#endif


/*!
 * \brief Send message
 * \n  app_id == APP_TYPE_FOREGROUND Send message to foreground application
 * \n  app_id == APP_TYPE_BLUETOOTH  Send message to foreground BT application
 * \return
 *     Success: Return value from message process
 * \n  Fail: -1
 */
#ifndef APP_FIX_ENABLE
extern int sys_manager_send_message
(
    id_t app_id, id_t msg_id, void* msg_data, int msg_size)
{
    int  ret_val;
    
    if (app_id == APP_TYPE_FOREGROUND ||
        app_id == APP_TYPE_BLUETOOTH)
    {
        id_t  fg_app = sys_manager_get_foreground_app();
    
        if (!(fg_app & app_id))
        {
            return -1;
        }

        app_id = fg_app;
    }

    ret_val = app_message_send_sync(app_id, msg_id, msg_data, msg_size);

    return ret_val;
}
#endif


/*!
 * \brief System reboot for specific mode
 * \n  reboot_mode : sys_manager_reboot_mode_t
 */
#ifndef APP_FIX_ENABLE
extern void sys_manager_ctrl_reboot(uint_t reboot_mode)
{
    /* Wait all events notify finished?
     */
    sys_manager_event_notify(SYS_EVENT_NONE, LED_VIEW_SYS_NOTIFY, YES);
    
    vdisk_write(VFD_SYS_REBOOT_MODE, &reboot_mode, sizeof(u32_t));

    sys_ctrl_reboot();
}
#endif


void sys_manager_ctrl_reboot_ex(uint_t reboot_mode)
{
    vdisk_write(VFD_SYS_REBOOT_MODE, &reboot_mode, sizeof(u32_t));

    sys_ctrl_reboot();
}


/*!
 * \brief Switch voice language between 1/2
 */
#ifndef APP_FIX_ENABLE
extern void sys_switch_voice_language(void)
{
    audio_manager_configs_t*  audio_configs = audio_manager_get_configs();

    bt_manager_context_t*  bt_manager = bt_manager_get_context();

    audio_configs->select_voice_language += 1;

    if (audio_configs->select_voice_language > VOICE_LANGUAGE_2)
    {
        audio_configs->select_voice_language = VOICE_LANGUAGE_1;
    }
    
    log_debug("%d", audio_configs->select_voice_language);

    vdisk_write
    (
        VFD_VOICE_LANGUAGE, &audio_configs->select_voice_language, sizeof(u8_t)
    );

    /* TWS sync switch TTS voice language?
     */
    if (bt_manager->bt_common.local_dev_role != TWS_NONE)
    {
        bt_manager_tws_send_event
        (
            TWS_EVENT_SET_VOICE_LANG, audio_configs->select_voice_language, NO
        );

        if (audio_configs->select_voice_language == VOICE_LANGUAGE_1)
        {
            bt_manager_tws_send_notify(SYS_EVENT_SEL_VOICE_LANG_1, YES);
        }
        else
        {
            bt_manager_tws_send_notify(SYS_EVENT_SEL_VOICE_LANG_2, YES);
        }
    }

    if (audio_configs->select_voice_language == VOICE_LANGUAGE_1)
    {
        sys_manager_event_notify
        (
            SYS_EVENT_SEL_VOICE_LANG_1, LED_VIEW_SYS_NOTIFY, NO
        );
    }
    else
    {
        sys_manager_event_notify
        (
            SYS_EVENT_SEL_VOICE_LANG_2, LED_VIEW_SYS_NOTIFY, NO
        );
    }
}
#endif


extern void sys_restore_factory_settings(void)
{
    log_warning();

    auto_reconnect_stop(YES, YES);
    
    bt_manager_tws_end_pair_search();
    bt_manager_end_pair_mode();
    bt_manager_end_wait_connect();
    
    bt_manager_set_visual_mode(NO, NO);
    
    bt_manager_disconnect_all_phones();
    bt_manager_disconnect_tws_dev();
    
    bt_manager_engine_ctrl(NULL, MSG_BTENGINE_CLR_PAIRED_LIST_SYNC, 0);

    bt_manager_engine_ctrl(NULL, MSG_BTENGINE_CLR_TWS_PAIRED_LIST_SYNC, 0);
    bt_manager_tws_clear_shared_addr();

    vdisk_clear(VFD_SAVED_DEV_VOLUME, 
        sizeof(bt_manager_saved_dev_volume_t) * CFG_MAX_BT_SUPPORT_DEVICES
    );

    vdisk_clear(VFD_BTMUSIC_MULTI_DAE, sizeof(u8_t));

    sys_manager_ctrl_reboot_ex(SYS_REBOOT_RESTORE_FACTORY_SETTINGS);
}


