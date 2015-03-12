
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"	//该行必须放在合适位置,否则编译出错,暂时不清楚是什么原因

#include "espconn.h"
#include "webserver_process.h"
#include "webserver_json.h"
#include "user_json.h"
#include "user_config.h"

/******************************************************************************
 * FunctionName : data_send
 * Description  : processing the data as http format and send to the client or server
 * Parameters   : arg -- argument to set for client or server
 *                responseOK -- true or false
 *                psend -- The send data
 * Returns      :
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
data_send(void *arg, bool responseOK, char *psend)
{
    uint16 length = 0;
    char *pbuf = NULL;
    char httphead[256];
    struct espconn *ptrespconn = arg;
    os_memset(httphead, 0, 256);

    if (responseOK) {
        os_sprintf(httphead,
                   "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nServer: lwIP/1.4.0\r\n",
                   psend ? os_strlen(psend) : 0);

        if (psend) {
            os_sprintf(httphead + os_strlen(httphead),
                       "Content-type: application/json\r\nExpires: Fri, 10 Apr 2008 14:00:00 GMT\r\nPragma: no-cache\r\n\r\n");
            length = os_strlen(httphead) + os_strlen(psend);
            pbuf = (char *)os_zalloc(length + 1);
            os_memcpy(pbuf, httphead, os_strlen(httphead));
            os_memcpy(pbuf + os_strlen(httphead), psend, os_strlen(psend));
        } else {
            os_sprintf(httphead + os_strlen(httphead), "\n");
            length = os_strlen(httphead);
        }
    } else {
        os_sprintf(httphead, "HTTP/1.0 400 BadRequest\r\n\
Content-Length: 0\r\nServer: lwIP/1.4.0\r\n\n");
        length = os_strlen(httphead);
    }

    if (psend) {
#ifdef SERVER_SSL_ENABLE
        espconn_secure_sent(ptrespconn, pbuf, length);
#else
        espconn_sent(ptrespconn, pbuf, length);
#endif
    } else {
#ifdef SERVER_SSL_ENABLE
        espconn_secure_sent(ptrespconn, httphead, length);
#else
        espconn_sent(ptrespconn, httphead, length);
#endif
    }

    if (pbuf) {
        os_free(pbuf);
        pbuf = NULL;
    }
}

static
const struct ProcSelect_t* getSelect(const struct ProcSelect_t* list, int size, const char* const name){
	int i;
	for(i = 0; i < size; i++){
		if(os_strcmp(list[i].name, name) == 0){
			return &list[i];
		}
	}
	return NULL;
}

static
const struct ProcCommand_t* getCommand(const struct ProcCommand_t* list, int size, const char* const name){
	int i;
	for(i = 0; i < size; i++){
		if(os_strcmp(list[i].name, name) == 0){
			return &list[i];
		}
	}
	return NULL;
}

static
void jsontree_send(struct espconn* ctx,struct jsontree_value *tree, const char* const path){
	char *pbuf = (char *)os_zalloc(jsonSize);
	json_ws_send(tree, path, pbuf);
	data_send(ctx, true, pbuf);
	os_free(pbuf);
}

static
void client_func_info(struct espconn* ctx, const char* const parm){
	jsontree_send(ctx, getINFOTree(), "info");
}

static
void client_func_status(struct espconn* ctx, const char* const parm){
	jsontree_send(ctx, getConStatusTree(), "status");
}

static
void client_func_scan(struct espconn* ctx, const char* const parm){

}

static
void config_func_wifi(struct espconn* ctx, const char* const parm){
	struct softap_config *ap_conf = (struct softap_config *)os_zalloc(sizeof(struct softap_config));;
	struct station_config *sta_conf = (struct station_config *)os_zalloc(sizeof(struct station_config));;
//	json_send(ptrespconn, WIFI);
	//	jsontree_send(ctx,wifi_info_tree, "wifi");
	os_free(sta_conf);
	os_free(ap_conf);
}

static
void config_func_switch(struct espconn* ctx, const char* const parm){
#if PLUG_DEVICE
	jsontree_send(ctx, getStatusTree(), "switch");
#endif
}

static
void config_func_light(struct espconn* ctx, const char* const parm){
#if LIGHT_DEVICE
	jsontree_send(ctx, getPwmTree(), "light");
#endif
}

static
void config_func_reboot(struct espconn* ctx, const char* const parm){
//	json_send(ctx, REBOOT);
}

static
void weight_func_info(struct espconn* ctx, const char* const parm){
	jsontree_send(ctx, getWeightInfo(), "info");
}

static void upgrade_func_getuser(struct espconn* ctx, const char* const parm){
	jsontree_send(ctx, getUserInfo(), "user_info");
}

static
const struct ProcCommand_t weight_cmd_info = {
		"info",		weight_func_info
};
static
const struct ProcCommand_t client_cmds[] = {
		{"info", 	client_func_info},
		{"status",	client_func_status},
		{"scan",	client_func_scan},
};
static
const struct ProcCommand_t config_cmds[] = {
//		{"wifi",	config_func_wifi},
		{"switch",	config_func_switch},
		{"light",	config_func_light},
//		{"reboot",	config_func_reboot},
};
static
const struct ProcCommand_t upgrade_cmd_getuser = {
		"getuser",	upgrade_func_getuser
};
static
const struct ProcSelect_t selects[] = {
		{"weight", 		&weight_cmd_info, 			1},
		{"client", 		client_cmds, 				ARRAY_SIZE(client_cmds)},
		{"config", 		config_cmds, 				ARRAY_SIZE(config_cmds)},
		{"upgrade", 	&upgrade_cmd_getuser, 		1},
};

/**
 * select:		pURL_Frame->pSelect
 * command:		pURL_Frame->pFilename
 * return:		void (*func)(struct espconn* ctx,const char* const str)
 */
void* getCommandHandler(const char* const select, const char* const command){
	const struct ProcSelect_t *psel = getSelect(selects, ARRAY_SIZE(selects), select);
	if(psel != NULL){
		const struct ProcCommand_t* pcmd = getCommand(psel->commands, psel->count, command);
		if(pcmd != NULL){
			return pcmd->func;
		}
	}
	return NULL;
}
