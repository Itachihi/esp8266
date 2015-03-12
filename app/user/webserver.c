/*
 * webserver.c
 *
 *  Created on: 2015-2-28
 *      Author: Administrator
 */

#include "user_interface.h"
#include "user_webserver.h"
#include "webserver_process.h"
#include "espconn.h"
#include "mem.h"

LOCAL os_timer_t client_timer;

LOCAL ICACHE_FLASH_ATTR
void checkip_timer_cb(uint8 flag){
	struct ip_info ipconfig;
	uint8_t status;

	os_timer_disarm(&client_timer);
	wifi_get_ip_info(STATION_IF, &ipconfig);
	status = wifi_station_get_connect_status();
	if (status == STATION_GOT_IP && ipconfig.ip.addr != 0) {

	}else if((status == STATION_WRONG_PASSWORD) ||
			(status == STATION_NO_AP_FOUND) ||
			(status == STATION_CONNECT_FAIL)){
		os_printf("station connect error\n");
	}else{
		os_timer_setfn(&client_timer, (os_timer_func_t *)checkip_timer_cb, NULL);
		os_timer_arm(&client_timer, 100, 0);
	}
}
LOCAL ICACHE_FLASH_ATTR
void parse_url(char *precv, URL_Frame *purl_frame)
{
    char *str = NULL;
    uint8 length = 0;
    char *pbuffer = NULL;
    char *pbufer = NULL;

    if (purl_frame == NULL || precv == NULL) {
        return;
    }

    pbuffer = (char *)os_strstr(precv, "Host:");

    if (pbuffer != NULL) {
        length = pbuffer - precv;
        pbufer = (char *)os_zalloc(length + 1);
        pbuffer = pbufer;
        os_memcpy(pbuffer, precv, length);
        os_memset(purl_frame->pSelect, 0, URLSize);
        os_memset(purl_frame->pCommand, 0, URLSize);
        os_memset(purl_frame->pFilename, 0, URLSize);

        if (os_strncmp(pbuffer, "GET ", 4) == 0) {
            purl_frame->Type = GET;
            pbuffer += 4;
        } else if (os_strncmp(pbuffer, "POST ", 5) == 0) {
            purl_frame->Type = POST;
            pbuffer += 5;
        }

        pbuffer ++;
        str = (char *)os_strstr(pbuffer, "?");

        if (str != NULL) {
            length = str - pbuffer;
            os_memcpy(purl_frame->pSelect, pbuffer, length);
            str ++;
            pbuffer = (char *)os_strstr(str, "=");

            if (pbuffer != NULL) {
                length = pbuffer - str;
                os_memcpy(purl_frame->pCommand, str, length);
                pbuffer ++;
                str = (char *)os_strstr(pbuffer, "&");

                if (str != NULL) {
                    length = str - pbuffer;
                    os_memcpy(purl_frame->pFilename, pbuffer, length);
                } else {
                    str = (char *)os_strstr(pbuffer, " HTTP");

                    if (str != NULL) {
                        length = str - pbuffer;
                        os_memcpy(purl_frame->pFilename, pbuffer, length);
                    }
                }
            }
        }

        os_free(pbufer);
    } else {
        return;
    }
}
LOCAL ICACHE_FLASH_ATTR
char* save_data(char *precv, uint16 length)
{
	LOCAL char *precvbuffer;
	LOCAL uint32 dat_sumlength = 0;
	LOCAL uint32 totallength = 0;

    bool flag = false;
    char length_buf[10] = {0};
    char *ptemp = NULL;
    char *pdata = NULL;
    uint16 headlength = 0;

    ptemp = (char *)os_strstr(precv, "\r\n\r\n");

    if (ptemp != NULL) {
        length -= ptemp - precv;
        length -= 4;
        totallength += length;
        headlength = ptemp - precv + 4;
        pdata = (char *)os_strstr(precv, "Content-Length: ");

        if (pdata != NULL) {
            pdata += 16;
            precvbuffer = (char *)os_strstr(pdata, "\r\n");

            if (precvbuffer != NULL) {
                os_memcpy(length_buf, pdata, precvbuffer - pdata);
                dat_sumlength = atoi(length_buf);
            }
        } else {
        	if (totallength != 0x00){
        		totallength = 0;
        		dat_sumlength = 0;
        		return NULL;
        	}
        }
        if ((dat_sumlength + headlength) >= 1024) {
        	precvbuffer = (char *)os_zalloc(headlength + 1);
            os_memcpy(precvbuffer, precv, headlength + 1);
        } else {
        	precvbuffer = (char *)os_zalloc(dat_sumlength + headlength + 1);
        	os_memcpy(precvbuffer, precv, os_strlen(precv));
        }
    } else {
        if (precvbuffer != NULL) {
            totallength += length;
            os_memcpy(precvbuffer + os_strlen(precvbuffer), precv, length);
        } else {
            totallength = 0;
            dat_sumlength = 0;
            return NULL;
        }
    }

    if (totallength == dat_sumlength) {
        totallength = 0;
        dat_sumlength = 0;
        return precvbuffer;
    } else {
        return NULL;
    }
}

LOCAL ICACHE_FLASH_ATTR
void webserver_recv(void *arg, char *pusrdata, unsigned short length){
	LOCAL uint8 upgrade_lock = 0;
	if(upgrade_lock) return;

    struct espconn *ptrespconn = arg;
    char *precvbuffer = NULL;

    URL_Frame *pURL_Frame = (URL_Frame *)os_zalloc(sizeof(URL_Frame));
    do{
    	void (*handler)(struct espconn* ctx,const char* const parm) = NULL;

    	precvbuffer = save_data(pusrdata, length);
    	if(precvbuffer == NULL) break;

    	parse_url(precvbuffer, pURL_Frame);
    	if(pURL_Frame->Type == GET){
        	handler = getCommandHandler(pURL_Frame->pSelect, pURL_Frame->pFilename);
        	if(handler != NULL){
        		char *s = (char *)os_strstr(pusrdata, "&");
        		handler(ptrespconn, s);
        	}

    	}else if(pURL_Frame->Type == POST){

    	}

    }while(0);
    os_free(pURL_Frame);
}

LOCAL ICACHE_FLASH_ATTR
void webserver_recon(void *arg, sint8 err)
{
    struct espconn *pesp_conn = arg;
    uint8_t *p = pesp_conn->proto.tcp->remote_ip;
    os_printf("webserver's %d.%d.%d.%d:%d err %d reconnect\n", p[0],p[1],p[2],p[3],pesp_conn->proto.tcp->remote_port, err);
}
LOCAL ICACHE_FLASH_ATTR
void webserver_discon(void *arg)
{
    struct espconn *pesp_conn = arg;
    uint8_t *p = pesp_conn->proto.tcp->remote_ip;
    os_printf("webserver's %d.%d.%d.%d:%d disconnect\n",  p[0],p[1],p[2],p[3],pesp_conn->proto.tcp->remote_port);
}

LOCAL ICACHE_FLASH_ATTR
void webserver_listen(void *arg)
{
    struct espconn *pesp_conn = arg;

    espconn_regist_recvcb(pesp_conn, webserver_recv);
    espconn_regist_reconcb(pesp_conn, webserver_recon);
    espconn_regist_disconcb(pesp_conn, webserver_discon);
}

LOCAL ICACHE_FLASH_ATTR
void webserver_start(int port){
    LOCAL struct espconn esp_conn;
    LOCAL esp_tcp esptcp;

    esp_conn.type = ESPCONN_TCP;
    esp_conn.state = ESPCONN_NONE;
    esp_conn.proto.tcp = &esptcp;
    esp_conn.proto.tcp->local_port = port;
    espconn_regist_connectcb(&esp_conn, webserver_listen);
    espconn_accept(&esp_conn);
}

ICACHE_FLASH_ATTR
void webserver_init(void){
	if(wifi_get_opmode() == SOFTAP_MODE){
		os_printf("wifi mode is error\n");
		return;
	}

	os_timer_disarm(&client_timer);
	os_timer_setfn(&client_timer, (os_timer_func_t *)checkip_timer_cb, 1);
	os_timer_arm(&client_timer, 100, 0);

	webserver_start(SERVER_PORT);
}
