/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/
#include "user_interface.h"
#include "driver/uart.h"
#include "driver/key.h"
#include "user.h"

#define KEY_NUM		1
LOCAL struct keys_param keys;
LOCAL struct single_key_param *single_key[KEY_NUM];



LOCAL void ICACHE_FLASH_ATTR
key_ok_short_press(void){
	sc_enable();
}
LOCAL void ICACHE_FLASH_ATTR
key_ok_long_press(void){
	system_restore();
	system_restart();
}

LOCAL void ICACHE_FLASH_ATTR
user_key_init(void){
	single_key[0] = key_init_single(KEY_ONEKEY_NUM, KEY_ONEKEY_MUX, KEY_ONEKEY_FUNC,
			key_ok_long_press, key_ok_short_press);

	keys.key_num = KEY_NUM;
	keys.single_key = single_key;

	key_init(&keys);
}

static
void init_done_cb(void){
    sc_init();
    user_key_init();
    webserver_init();
    user_devicefind_init();
}
/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
	uart_init(BIT_RATE_115200,BIT_RATE_115200);
    os_printf("SDK version:%s\n", system_get_sdk_version());

    wifi_set_opmode(STATION_MODE);
    system_set_os_print(1);
    system_init_done_cb(init_done_cb);
}

