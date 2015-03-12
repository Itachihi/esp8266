

#include "user_interface.h"
#include "smartconfig.h"
#include "user.h"

static int flag = 0;

static os_timer_t led_timer;
static
void led_timer_cb(void){
	static int status;
	status++;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_ONEKEY_NUM), status & 0x01);
}
static
void led_enable(void){
	os_timer_disarm(&led_timer);
	os_timer_setfn(&led_timer,led_timer_cb, NULL);
	os_timer_arm(&led_timer, 50, 1);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_ONEKEY_NUM),0);
}
static
void led_disable(void){
	os_timer_disarm(&led_timer);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_ONEKEY_NUM),1);
}

static
void wifi_connect(struct station_config *cfg){
	struct station_config conf;
	os_memcpy(&conf, cfg, sizeof(struct station_config));
	wifi_station_set_config(&conf);
	wifi_station_disconnect();
	wifi_station_connect();
}

static
void sc_done_cb(void *dat){
	wifi_connect((struct station_config *)dat);
	sc_disable();
}

static
void sc_softap_mode(void){
	wifi_set_opmode(STATIONAP_MODE);
	struct softap_config config;
	char password[33];
	char macaddr[6];
	wifi_softap_get_config(&config);
	wifi_get_macaddr(SOFTAP_IF, macaddr);

	config.ssid_len = os_sprintf(config.ssid, "%s_" MACSTR3, AP_SSID, MAC2STR3(macaddr));
	os_sprintf(config.password, "%s", PASSWORD);

	config.authmode = AUTH_WPA_WPA2_PSK;

	wifi_softap_set_config(&config);
}

void sc_enable(void){
	if(flag == 0){
		flag = 1;
		smartconfig_start(SC_TYPE_AIRKISS, sc_done_cb);
		led_enable();
	}
}

void sc_disable(void){
	if(flag != 0){
		led_disable();
		smartconfig_stop();
		flag = 0;
	}
}

void sc_init(void){
	struct station_config conf;
	wifi_station_get_config(&conf);
	if(conf.ssid[0] != 0 && conf.password[0] != 0){
		os_printf("ssid: %s\n",conf.ssid);
		os_printf("pwd:  %s\n",conf.password);
		wifi_station_disconnect();
		wifi_station_connect();
	}else{
//		sc_enable();
		sc_softap_mode();
	}
	PIN_FUNC_SELECT(LED_ONEKEY_MUX, LED_ONEKEY_FUNC);
}

