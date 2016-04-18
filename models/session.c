#include "session.h"

static serialize_format_t session_formats[] = {
	SFT_UINT(table_session_t, sessId, "table_session_t of sessId"), // Sess Id
	SFT_INT(table_session_t, sockfd, "table_session_t of sockfd"), // Sockfd
	SFT_STRLEN(table_session_t, host, host_length, 15, "table_session_t of host"), // Host
	SFT_INT(table_session_t, port, "table_session_t of port"), // Port
	SFT_UINT(table_session_t, tid, "table_session_t of tid"), // Tid
	SFT_INT(table_session_t, dateline, "table_session_t of dateline"), // Dateline
	SFT_STRLEN(table_session_t, createTime, createTime_length, 19, "table_session_t of createTime"), // Create Time
	SFT_UINT(table_session_t, uid, "table_session_t of uid"), // Uid
	SFT_STRLEN(table_session_t, strSet, strSet_length, 65535, "table_session_t of strSet"), // Str Set
	SFT_STR(table_session_t, longtext, longtext_length, "table_session_t of longtext"), // Longtext
	SFT_END
};

static serialize_object_t session_object = {session_formats, NULL};

serialize_format_t session_format = SFT_OBJECT(null_t, null, &session_object, "object_t of objects_format");

