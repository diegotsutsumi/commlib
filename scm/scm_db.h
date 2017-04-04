#ifndef SCM_DB_H
#define SCM_DB_H

#include <mysql/mysql.h>

#ifdef __cplusplus 
extern "C" {
#endif 

extern MYSQL dbconn;

extern int scmDbConnected;

void scmDbConnect();
#ifdef __cplusplus 
} /* extern "C" */
#endif 

#endif
