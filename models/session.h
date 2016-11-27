#ifndef HAVE_SESSION_H
#define HAVE_SESSION_H
#include "serialize.h"

typedef struct {
	uint_t sessId; // Sess Id
	int_t sockfd; // Sockfd
	char_t host[16]; // Host
	size_t host_length;
	uint_t port; // Port
	uint_t tid; // Tid
	int_t dateline; // Dateline
	char_t createTime[20]; // Create Time
	size_t createTime_length;
	uint_t uid; // Uid
	int_t loginDateline; // Login Dateline
	char_t loginTime[20]; // Login Time
	size_t loginTime_length;
	int_t logoutDateline; // Logout Dateline
	char_t logoutTime[20]; // Logout Time
	size_t logoutTime_length;
} table_session_t;

extern serialize_format_t session_format;

#endif
