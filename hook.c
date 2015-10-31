#include "RtCSP.h"
#include "conn.h"
#include "serialize.h"

GHashTable *ht_conn_recvs = NULL;

typedef struct
{
	char *key;
	unsigned int keylen;
} srl_hash_t;

serialize_format_t hformat = SFT_STR(srl_hash_t, key, keylen, "parse receive hook key");

inline void hook_thread_init(worker_thread_t *thread) {
	int i;
	for(i=0;i<rtcsp_length;i++) {
		if(rtcsp_modules[i]->thread_init) {
			rtcsp_modules[i]->thread_init(thread);
		}
	}
}

inline void hook_thread_destory(worker_thread_t *thread) {
	int i;
	for(i=0;i<rtcsp_length;i++) {
		if(rtcsp_modules[i]->thread_destory) {
			rtcsp_modules[i]->thread_destory(thread);
		}
	}
}

inline void hook_start() {
	int i,j;
	conn_recv_t *ptr;
	ht_conn_recvs = g_hash_table_new(g_str_hash, g_str_equal);
	for(i=0;i<rtcsp_length;i++) {
		for(j=0;j<rtcsp_modules[i]->conn_recv_len;j++) {
			ptr = &(rtcsp_modules[i]->conn_recvs[j]);
			if(g_hash_table_lookup(ht_conn_recvs,ptr->key)) {
				fprintf(stderr, "Connection receive hook (%s->%s) is exists.\n", rtcsp_names[i], ptr->key);
			} else {
				g_hash_table_insert(ht_conn_recvs,ptr->key,ptr->call);
			}
		}
		if(rtcsp_modules[i]->start) {
			rtcsp_modules[i]->start();
		}
	}
}

inline void hook_stop() {
	int i;
	for(i=0;i<rtcsp_length;i++) {
		if(rtcsp_modules[i]->stop) {
			rtcsp_modules[i]->stop();
		}
	}

	g_hash_table_destroy(ht_conn_recvs);
}

inline bool hook_conn_accept(conn_t *ptr) {
	int i;
	for(i=0;i<rtcsp_length;i++) {
		if(rtcsp_modules[i]->conn_accept && !rtcsp_modules[i]->conn_accept(ptr)) {
			return false;
		}
	}

	return true;
}

inline void hook_conn_close(conn_t *ptr) {
	int i;
	for(i=0;i<rtcsp_length;i++) {
		if(rtcsp_modules[i]->conn_close) {
			rtcsp_modules[i]->conn_close(ptr);
		}
	}
}

inline void hook_conn_recv(conn_t *ptr,const char *data, int data_len) {
	conn_recv_func_t call;
	unsigned int tmplen;
	srl_hash_t hkey = {NULL, 0};
	
	tmplen = serialize_parse((void*)&hkey, &hformat, data, data_len);
	if(!tmplen) {
		fprintf(stderr, "Parse receive hook key failed(%s).\n", data);
		if(hkey.key) {
			free(hkey.key);
		}
		return;
	}

	call = g_hash_table_lookup(ht_conn_recvs,hkey.key);
	if(!call) {
		fprintf(stderr, "Connection receive hook (%s) not exists.\n", hkey.key);
		free(hkey.key);
		return;
	}

	GString gstr = {NULL,0,0};

	g_string_append_len(&gstr, data, tmplen);

	if(call(ptr, data+tmplen, data_len-tmplen, &gstr)) {
		socket_send(ptr, gstr.str, gstr.len);
	}

	free(hkey.key);
	free(gstr.str);
}

inline void hook_conn_denied(conn_t *ptr) {
	int i;
	for(i=0;i<rtcsp_length;i++) {
		if(rtcsp_modules[i]->conn_denied) {
			rtcsp_modules[i]->conn_denied(ptr);
		}
	}
}
