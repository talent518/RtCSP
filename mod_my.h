#include "RtCSP.h"
#include <mysql.h>

extern rtcsp_module_t mod_my_module;

extern MYSQL *mod_my_conns;

extern char *mod_my_host;
extern char *mod_my_user;
extern char *mod_my_passwd;
extern char *mod_my_database;
extern unsigned int mod_my_port;
extern char *mod_my_socket;

#define MOD_MY_CONN MOD_MY_CONN_EX(0)
#define MOD_MY_CONN_PTR MOD_MY_CONN_EX(ptr->thread->id+1)
#define MOD_MY_CONN_EX(i) (&(mod_my_conns[i]))

