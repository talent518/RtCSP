#include "RtCSP.h"
#include "conn.h"
#include "serialize.h"

GHashTable *ht_conn_recvs = NULL;

typedef struct
{
	char key[101];
	size_t keylen;
} srl_hash_t;

serialize_format_t hformat = SFT_STR(srl_hash_t, key, keylen, "parse receive hook key");

void hook_thread_init(worker_thread_t *thread) {
	int i;
	for(i=0;i<rtcsp_length;i++) {
		rtcsp_modules[i]->thread_init(thread);
	}
}

void hook_thread_destory(worker_thread_t *thread) {
	int i;
	for(i=0;i<rtcsp_length;i++) {
		rtcsp_modules[i]->thread_destory(thread);
	}
}

void hook_start() {
	int i,j;
	char *key;
	conn_recv_t *ptr;
	ht_conn_recvs = g_hash_table_new(g_str_hash, g_str_equal);
	for(i=0;i<rtcsp_length;i++) {
		for(j=0;j<rtcsp_modules[i]->conn_recv_len;j++) {
			ptr = &(rtcsp_modules[i]->conn_recvs[j]);
			if(g_hash_table_lookup(ht_conn_recvs,ptr->key)) {
				printf("Connection receive hook (%s->%s) is exists.\n", rtcsp_names[i], ptr->key);
			} else {
				dprintf("Connection receive hook hashtable index (%s->%s).\n", rtcsp_names[i], ptr->key);
				g_hash_table_insert(ht_conn_recvs,ptr->key,ptr->call);
			}
		}
	}

	for(i=0;i<rtcsp_length;i++) {
		rtcsp_modules[i]->start();
	}
}

void hook_stop() {
	int i;
	for(i=0;i<rtcsp_length;i++) {
		rtcsp_modules[i]->stop();
	}

	g_hash_table_destroy(ht_conn_recvs);
}

int hook_conn_accept(conn_t *ptr) {
	int i,ret;
	for(i=0;i<rtcsp_length;i++) {
		ret+=rtcsp_modules[i]->conn_accept(ptr);
	}

	return ret;
}

void hook_conn_close(conn_t *ptr) {
	int i;
	for(i=0;i<rtcsp_length;i++) {
		rtcsp_modules[i]->conn_close(ptr);
	}
}

void hook_conn_recv(conn_t *ptr,const char *data, int data_len) {
	volatile char *buf = NULL;
	int buflen;
	conn_recv_func_t call;
	size_t tmplen;
	srl_hash_t hkey = {NULL, 0};
	
	tmplen = serialize_parse((void*)&hkey, &hformat, data, data_len);
	if(!tmplen) {
		printf("Parse receive hook key failed(%s).\n", data);
		return;
	}

	data += tmplen;
	data_len -= tmplen;

	call = g_hash_table_lookup(ht_conn_recvs,hkey.key);
	if(!call) {
		printf("Connection receive hook (%s) not exists.\n", hkey.key);
		free(hkey.key);
		return;
	}

	buflen = call(ptr,data,data_len,&buf);
	if(buflen>0) {
		socket_send(ptr,buf,buflen);
	} else {
		printf("receive hook (%s) not return message.\n", hkey.key);
		free(hkey.key);
		return;
	}

	free(buf);
	free(hkey.key);
}

void hook_conn_denied(conn_t *ptr) {
	int i;
	for(i=0;i<rtcsp_length;i++) {
		rtcsp_modules[i]->conn_denied(ptr);
	}
}
