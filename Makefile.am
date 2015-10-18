bin_PROGRAMS = rtcspd bench

rtcspd_SOURCES = RtCSP.c server.c loop_event.c hook.c socket.c conn.c serialize.c queue.c api.c getopt.c hook_internal.c  modules/demo/demo.c
rtcspd_CFLAGS = -w
rtcspd_LDFLAGS = 

bench_SOURCES = bench.c bench_internal.c  modules/demo/demo_bench.c
bench_CFLAGS = -w -O3 -DBENCH
bench_LDFLAGS = 
