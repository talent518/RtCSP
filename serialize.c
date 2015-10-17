#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>

#include "serialize.h"

typedef struct
{
	char *key;
	size_t keylen;
} srl_hash_t;

typedef struct
{
	GString *gstr;
	serialize_format_t *format;
} srl_foreach_t;

void serialize_object_init(serialize_object_t *format) {
	assert(format);
	assert(format->formats);
	assert(format->ht == NULL);

	format->ht = g_hash_table_new(g_str_hash, g_str_equal);

	int i = 0;
	while(format->formats[i].key) {
		g_hash_table_replace(format->ht, format->formats[i].key, &format->formats[i]);

		i++;
	}
}

void serialize_object_destory(serialize_object_t *format) {
	assert(format);
	assert(format->formats);
	assert(format->ht);

	g_hash_table_destroy(format->ht);

	format->ht = NULL;
}

serialize_format_t *find_serialize_format(serialize_object_t *format,const char *key, size_t keylen) {
	return g_hash_table_lookup(format->ht, key);
}

size_t _serialize_parse_force(const char *bbuffer, char *buffer, size_t buflen) {
	char *bpos = buffer,*ptr = NULL;
	size_t n = 0, tmplen;
	char be[] = "(){}[]";

	if(buflen < 2) {
		return 0;
	}

	switch(*buffer) {
		case SRL_TYPE_BOOLEAN:
		case SRL_TYPE_BOOLEAN2:
		case SRL_TYPE_CHAR:
		case SRL_TYPE_CHAR2:
		case SRL_TYPE_INTEGER:
		case SRL_TYPE_UNSIGNED_INTEGER:
		case SRL_TYPE_LONG:
		case SRL_TYPE_UNSIGNED_LONG:
		case SRL_TYPE_FLOAT:
		case SRL_TYPE_FLOAT2:
		case SRL_TYPE_DOUBLE:
		case SRL_TYPE_LONG_DOUBLE:
			ptr = memchr(buffer, ';', buflen);
			if(!ptr) {
				dprintf("error(force): In %u position after not found character for \";\".\n", (size_t)(buffer-bbuffer));
				return 0;
			}

			buflen -= (size_t)(ptr - buffer + 1);
			buffer = ptr + 1;
			break;
		case SRL_TYPE_STRING:
		case SRL_TYPE_STRING_LENGTH: {
			size_t len = strtoul(buffer+2, &ptr, 10);

			if(len+1 > buflen) {
				dprintf("error(force): In %u position string length too much.\n", (size_t)(buffer-bbuffer+1));
				return 0;
			}

			len = (ptr - buffer + len + 1);

			buflen -= len;
			buffer += len;

			if( buflen < 1 || *buffer != ';') {
				dprintf("error(force): In %u position not is character for \";\".\n", (size_t)(buffer-bbuffer));

				return 0;
			}

			buffer++;
			buflen--;
			break;
		}
		case SRL_TYPE_ARRAY:
		case SRL_TYPE_ARRLEN: {
			n += 2;
		}
		case SRL_TYPE_OBJECT:
		case SRL_TYPE_OBJECT2: {
			n += 2;
		}
		case SRL_TYPE_HASHTABLE:
		case SRL_TYPE_HASHTABLE2:
			if(!(ptr = memchr(buffer, be[n], buflen))) {
				dprintf("error(force): In %u position after not found character for \"%c\".\n", (size_t)(buffer-bbuffer), be[n]);

				return 0;
			}
			
			buflen -= (size_t)(ptr + 1 - buffer);
			buffer = ptr + 1;

			while(buflen>0 && *buffer != be[n+1]) {
				tmplen = _serialize_parse_force(bbuffer, buffer, buflen);
				if(!tmplen) {
					dprintf("error(force): in %d postion invalid type character.\n", (size_t)(buffer-bbuffer));
					return 0;
				}
				buffer += tmplen;
				buflen -= tmplen;

				if(be[n] == '[') {
					continue;
				}
				if(!buflen || *buffer == be[n+1]) {
					break;
				}
				tmplen = _serialize_parse_force(bbuffer, buffer, buflen);
				if(!tmplen) {
					dprintf("error(force): in %d postion invalid type character.\n", (size_t)(buffer-bbuffer));
					return 0;
				}
				buffer += tmplen;
				buflen -= tmplen;
			}

			if( buflen < 1 || *buffer != be[n+1]) {
				dprintf("error(force): In %u position not is character for \"%c\".\n", (size_t)(buffer-bbuffer), be[n+1]);

				return 0;
			}

			buffer++;
			buflen--;
			break;
		default:
			return 0;
	}

	return buffer - bpos;
}

size_t _serialize_parse(void *obj, serialize_format_t *format, const char *bbuffer, char *buffer, size_t buflen, size_t *forcelen) {
	char *bpos = buffer, *pos=buffer, chr=';';
	size_t tmplen, vallen=0;
	void *arg = obj+format->offset;

	while(buflen>0 && *buffer != format->type && (tmplen = _serialize_parse_force(bbuffer, buffer, buflen))) {
		buffer += tmplen;
		buflen -= tmplen;
		*forcelen += tmplen;
	}

	if(!buflen || *buffer != format->type) {
		return 0;
	}

	buffer++;

	if(*buffer != ':') {
		dprintf("error: In %u position not is character for \":\".\n", (size_t)(buffer-bbuffer));
		return 0;
	}

	buffer++;

	pos = NULL;

	switch(format->type) {
		case SRL_TYPE_BOOLEAN:
		case SRL_TYPE_BOOLEAN2: {
			*((bool*)arg)=(*buffer != '0');
			pos=buffer+1;
			break;
		}
		case SRL_TYPE_CHAR:
		case SRL_TYPE_CHAR2: {
			*((char*)arg)=*buffer;
			pos=buffer+1;
			break;
		}
		case SRL_TYPE_INTEGER: {
			*((int*)arg)=strtol(buffer,&pos,10);
			break;
		}
		case SRL_TYPE_UNSIGNED_INTEGER: {
			*((unsigned int*)arg)=strtoul(buffer,&pos,10);
			break;
		}
		case SRL_TYPE_LONG: {
			*((long int*)arg)=strtol(buffer,&pos,10);
			break;
		}
		case SRL_TYPE_UNSIGNED_LONG: {
			*((unsigned long int*)arg)=strtoul(buffer,&pos,10);
			break;
		}
		case SRL_TYPE_FLOAT:
		case SRL_TYPE_FLOAT2: {
			*((float*)arg)=strtof(buffer,&pos);
			break;
		}
		case SRL_TYPE_DOUBLE: {
			*((double*)arg)=strtod(buffer,&pos);
			break;
		}
		case SRL_TYPE_LONG_DOUBLE: {
			*((double*)arg)=strtold(buffer,&pos);
			break;
		}
		case SRL_TYPE_STRING:
		case SRL_TYPE_STRING_LENGTH: {
			vallen=strtoul(buffer,&pos,10);
			chr = ':';
			break;
		}
		case SRL_TYPE_OBJECT:
		case SRL_TYPE_OBJECT2: {
			chr = '{';
			pos = buffer;
			break;
		}
		case SRL_TYPE_ARRAY:
		case SRL_TYPE_ARRLEN: {
			vallen=strtoul(buffer,&pos,10);
			chr = '[';
			break;
		}
		case SRL_TYPE_HASHTABLE:
		case SRL_TYPE_HASHTABLE2: {
			chr = '(';
			pos = buffer;
			break;
		}
		default: {
			pos = buffer;
		}
	}

	pos = strchr(pos, chr);
	if(!pos) {
		dprintf("error: In %u position after character not is \"%c\".\n", (size_t)(buffer-bbuffer), chr);
		return 0;
	}

	buffer=pos+1;

	if(chr == ':' || chr == '[') {
		if(!vallen) {
			return buffer - bpos + 1;
		}

		if(buflen-(buffer-bpos)<vallen) {
			dprintf("error: In %u position before parse data format length is %d, the after character length than less.\n", (size_t)(buffer-bbuffer), vallen);
			return 0;
		}

		tmplen = vallen;
		if(format->type == SRL_TYPE_STRING_LENGTH || format->type == SRL_TYPE_ARRLEN) {
			if(vallen > format->vallen) {
				vallen = format->vallen;
			}
			pos = (char*)arg;
		}

		*((size_t*)(obj+format->offsetlen))=vallen;

		switch(format->type) {
			case SRL_TYPE_STRING: {
				if(*((void **)arg)) {
					dprintf("error: Dynamic pointer properties cannot be duplicated(%s).", format->help);
					return 0;
				}
				*((char **)arg) = (char*)malloc(vallen+1);
				pos = *((char**)arg);
				memset(pos, 0, vallen+1);
			}
			case SRL_TYPE_STRING_LENGTH: {
				memcpy(pos, buffer, vallen);
				*(pos+vallen) = '\0';
				buffer+=tmplen;
				if(*buffer != ';') {
					dprintf("error: In %u position after character not is \";\".\n", (size_t)(buffer-bbuffer));
					return 0;
				}
				buffer++;
				break;
			}
			case SRL_TYPE_ARRAY: {
				if(*((void **)arg)) {
					dprintf("error: Dynamic pointer properties cannot be duplicated(%s).", format->help);
					return 0;
				}
				*((void **)arg) = malloc(format->memsize*vallen);
				pos = *((void**)arg);
				memset(pos, 0, format->memsize*vallen);
			}
			case SRL_TYPE_ARRLEN: {
				size_t i;
				for(i=0; i<vallen; i++) {
					tmplen = _serialize_parse(pos + i*format->memsize, format->format, bbuffer, buffer, buflen, forcelen);

					if(!tmplen) {
						dprintf("error: %s\n", format->help);
						return 0;
					}

					buffer += tmplen;
					buflen -= tmplen;
				}
				if(*buffer != ']') {
					dprintf("error: In %u position after character not is \"]\".\n", (size_t)(buffer-bbuffer));
					return 0;
				}
				buffer++;
				break;
			}
		}
	}
	if(chr == '{') {
		vallen = _serialize_parse_object(arg, format->object, bbuffer, buffer, buflen, forcelen);
		
		if(!vallen) {
			dprintf("error: %s\n", format->help);
			return 0;
		}

		buffer += vallen;
		buflen -= vallen;
		
		if(*buffer != '}') {
			dprintf("error: In %d position after character not is \"}\".\n", (int)(buffer-bbuffer));
			return 0;
		}

		buffer++;
	}
	if(chr == '(') {
		GHashTable *ht = (format->isint ? g_hash_table_new_full(g_int_hash, g_int_equal, (GDestroyNotify)free, format->func) : g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)free, format->func));
		int i;

		if(*((void **)arg)) {
			dprintf("error: Dynamic pointer properties cannot be duplicated(%s).", format->help);
			return 0;
		}
		*((void **)arg) = ht;

		unsigned int ikey;
		void *hval;
		serialize_format_t iformat = SFT_UINT(null_t, null, "parse hash table of integer key");

		srl_hash_t hkey = {NULL, 0};
		serialize_format_t hformat = SFT_STR(srl_hash_t, key, keylen, "parse hash table of string key");

		while(buflen>0 && *buffer != ')') {
			if(format->isint) {
				tmplen = _serialize_parse((void*)&ikey, &iformat, bbuffer, buffer, buflen, forcelen);

				if(!tmplen) {
					dprintf("error: %s\n", iformat.help);
					return 0;
				}
			} else {
				tmplen = _serialize_parse((void*)&hkey, &hformat, bbuffer, buffer, buflen, forcelen);

				if(!tmplen) {
					if(hkey.key) {
						free(hkey.key);
					}
					dprintf("error: %s\n", hformat.help);
					return 0;
				}
			}

			buffer += tmplen;
			buflen -= tmplen;

			hval = malloc(format->memsize);

			if(format->isint) {
				hkey.key = (char*)malloc(sizeof(unsigned int));
				memcpy(hkey.key, &ikey, sizeof(unsigned int));
			}

			g_hash_table_insert(ht, hkey.key, hval);

			tmplen = _serialize_parse(hval, format->format, bbuffer, buffer, buflen, forcelen);

			if(!tmplen) {
				dprintf("error: %s\n", format->help);
				return 0;
			}

			buffer += tmplen;
			buflen -= tmplen;
		}

		if(*buffer != ')') {
			dprintf("error: In %d position after character not is \"]\".\n", (int)(buffer-bbuffer));
			return 0;
		}
		buffer++;
	}

	return buffer-bpos;
}

int _serialize_parse_object(void *obj, serialize_object_t *format, const char *bbuffer, char *buffer, int buflen, size_t *forcelen) {
	serialize_format_t *fmt;
	char key[101], *pos, *bpos=buffer, *epos;
	size_t tmplen;

	while(buflen>2 && *buffer == 's' && *(buffer+1) == ':') {
		epos = buffer;

		tmplen = strtoul(buffer+2, &pos, 10);
		if(!tmplen) {
			dprintf("error: In %d position after character not is \":\".\n", (size_t)(buffer-bbuffer));
			return 0;
		}

		if(tmplen >= sizeof(key)) {
			dprintf("error: In %d position after string length not be more than %d.\n", (size_t)(buffer-bbuffer), sizeof(key)-1);
			return 0;
		}

		buffer = pos + 1 + tmplen;

		if(buflen - (buffer-epos) <= 0 || *buffer != ';') {
			dprintf("error: In %d position character not is \";\".\n", (size_t)(buffer-bbuffer));
			return 0;
		}

		buffer++;

		buflen-=(buffer-epos);

		memcpy(key, pos+1, tmplen);

		key[tmplen] = '\0';

		fmt = find_serialize_format(format, key, tmplen);
		if(!fmt) {
			tmplen = _serialize_parse_force(bbuffer, buffer, buflen);
			if(tmplen) {
				buffer += tmplen;
				buflen -= tmplen;
				*forcelen += buffer - epos;
				continue;
			}

			return 0;
		}

		tmplen = _serialize_parse(obj, fmt, bbuffer, buffer, buflen, forcelen);

		if(!tmplen) {
			dprintf("error: %s\n", fmt->help);
			return 0;
		}
		
		buffer += tmplen;
		buflen -= tmplen;
	}

	return buffer - bpos;
}

static void _serialize_foreach_ht(unsigned int *key, void *value, srl_foreach_t *fe) {
	if(fe->format->isint) {
		g_string_append_printf(fe->gstr, "I:%u;", *key);
	} else {
		g_string_append_printf(fe->gstr, "s:%s;", (char*)key);
	}

	_serialize_string(fe->gstr, value, fe->format->format);
}

void _serialize_string(GString *gstr, void *obj, serialize_format_t *format) {
	void *arg = obj+format->offset;
	char *ptr=(char*)arg;
	size_t *arglen = (size_t*)(obj+format->offsetlen);
	char chr = ';';

	g_string_append_printf(gstr, "%c:", format->type);
	switch(format->type) {
		case SRL_TYPE_BOOLEAN:
		case SRL_TYPE_BOOLEAN2: {
			g_string_append_printf(gstr, "%d", *(bool*)arg?1:0);
			break;
		}

		case SRL_TYPE_CHAR:
		case SRL_TYPE_CHAR2: {
			g_string_append_printf(gstr, "%c", *(char*)arg);
			break;
		}

		case SRL_TYPE_INTEGER: {
			g_string_append_printf(gstr, "%d", *(int*)arg);
			break;
		}
		case SRL_TYPE_UNSIGNED_INTEGER: {
			g_string_append_printf(gstr, "%u", *(unsigned int*)arg);
			break;
		}

		case SRL_TYPE_LONG: {
			g_string_append_printf(gstr, "%ld", *(long int*)arg);
			break;
		}

		case SRL_TYPE_UNSIGNED_LONG: {
			g_string_append_printf(gstr, "%lld", *(unsigned long int*)arg);
			break;
		}

		case SRL_TYPE_FLOAT:
		case SRL_TYPE_FLOAT2: {
			g_string_append_printf(gstr, "%f", *(float*)arg);
			break;
		}

		case SRL_TYPE_DOUBLE: {
			g_string_append_printf(gstr, "%lf", *(double*)arg);
			break;
		}

		case SRL_TYPE_LONG_DOUBLE: {
			g_string_append_printf(gstr, "%llf", *(long double*)arg);
			break;
		}

		case SRL_TYPE_STRING: {
			ptr = *((char**)arg);
		}
		case SRL_TYPE_STRING_LENGTH: {
			g_string_append_printf(gstr, "%u:", *arglen);
			g_string_append_len(gstr, ptr, (size_t)*arglen);
			break;
		}

		case SRL_TYPE_OBJECT: {
			g_string_append_c(gstr, '{');
			_serialize_string_object(gstr, arg, format->object);
			chr = '}';
			break;
		}

		case SRL_TYPE_ARRAY: {
			ptr = *((void**)arg);
		}
		case SRL_TYPE_ARRLEN: {
			g_string_append_printf(gstr, "%u[", *arglen);
			size_t i;
			for(i=0; i<*arglen; i++) {
				_serialize_string(gstr, ptr + i*format->memsize, format->format);
			}
			chr = ']';
			break;
		}

		case SRL_TYPE_HASHTABLE:
		case SRL_TYPE_HASHTABLE2: {
			g_string_append_c(gstr, '(');
			srl_foreach_t fe = {gstr, format};
			g_hash_table_foreach(*(GHashTable**)arg, _serialize_foreach_ht, &fe);
			chr = ')';
		}
	}
	g_string_append_c(gstr, chr);
}

void _serialize_string_object(GString *gstr, void *obj, serialize_object_t *format) {
	size_t i = 0;
	while(format->formats[i].key) {
		g_string_append_printf(gstr, "s:%d:%s;", format->formats[i].keylen, format->formats[i].key);

		_serialize_string(gstr, obj, &format->formats[i]);

		i++;
	}
}

size_t serialize_parse(void *obj, serialize_format_t *format, const char *buffer, size_t buflen) {
	assert(obj);
	assert(format);
	assert(buffer);
	assert(buflen > 0);

	size_t forcelen = 0;

	size_t parselen = _serialize_parse(obj, format, buffer, buffer, buflen, &forcelen);

	if(forcelen>0) {
		dprintf("forcelen: %d\n", forcelen);
	}

	return parselen;
}

size_t serialize_parse_object(void *obj, serialize_object_t *format, const char *buffer, size_t buflen) {
	assert(obj);
	assert(format);
	assert(format->ht);
	assert(buffer);
	assert(buflen > 0);

	size_t forcelen = 0;

	size_t parselen = _serialize_parse_object(obj, format, buffer, buffer, buflen, &forcelen);

	if(forcelen>0) {
		dprintf("forcelen: %d\n", forcelen);
	}

	return parselen;
}

size_t serialize_string(void *obj, serialize_format_t *format, char **buffer) {
	assert(obj);
	assert(format);
	assert(buffer);

	GString *gstr;
	size_t buflen;

	gstr = g_string_sized_new(1024);

	_serialize_string(gstr, obj, format);

	buflen = gstr->len;
	*buffer = g_string_free(gstr, FALSE);

	return buflen;
}

size_t serialize_string_object(void *obj, serialize_object_t *format, char **buffer) {
	assert(obj);
	assert(format);
	assert(format->ht);
	assert(buffer);

	GString *gstr;
	size_t buflen;

	gstr = g_string_sized_new(1024);

	_serialize_string_object(gstr, obj, format);

	buflen = gstr->len;
	*buffer = g_string_free(gstr, FALSE);

	return buflen;
}
