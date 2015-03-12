#ifndef __WEBSERVER_JSON_H__
#define __WEBSERVER_JSON_H__

#include "json/jsontree.h"

struct jsontree_value* getINFOTree(void);
struct jsontree_value* getWeightInfo(void);
struct jsontree_value* getConStatusTree(void);
struct jsontree_value* getPwmTree(void);
struct jsontree_value* getStatusTree(void);
struct jsontree_value* getUserInfo(void);

#endif
