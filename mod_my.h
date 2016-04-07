#include "RtCSP.h"
#include <mysql.h>

extern rtcsp_module_t mod_my_module;

extern MYSQL **mod_my_conns;

extern char *mod_my_host;
extern char *mod_my_user;
extern char *mod_my_passwd;
extern char *mod_my_database;
extern unsigned int mod_my_port;
extern char *mod_my_socket;

#define MOD_MY_CONN (mod_my_conns[0])
#define MOD_MY_CONN_PTR (mod_my_conns[ptr->thread->id+1])
#define MOD_MY_CONN_EX(i) (mod_my_conns[i])

