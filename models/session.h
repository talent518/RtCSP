#ifndef HAVE_SESSION_H
#define HAVE_SESSION_H
#include "serialize.h"

typedef struct {
	uint_t sessId; // Sess Id
	int_t sockfd; // Sockfd
	char_t host[16]; // Host
	size_t host_length;
	int_t port; // Port
	uint_t tid; // Tid
	int_t dateline; // Dateline
	char_t createTime[20]; // Create Time
	size_t createTime_length;
	uint_t uid; // Uid
	char_t strSet[65536]; // Str Set
	size_t strSet_length;
	char_t *longtext; // Longtext
	size_t longtext_length;
} table_session_t;

extern serialize_format_t session_format;

#endif
