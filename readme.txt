实时聊天序列化协议(Real-time chat serialization protocol)

1. 生成配置(autotools):./autogen.sh
	./autogen.sh 1 默认的glib和libevent库位置是/usr/local并会自动执行 ./configure --with-libevent=/usr/local --with-glib=/usr/local
2. 配置：./configure
	--with-glib=/usr/local : glib(>=2.40.0) install path
	--with-libevent=/usr/local : libevent(>=2.0.21) install path

3. 编译：make
4. 运行服务端：./rtcspd -s start
5. 运行压力测试：./rtcspb
6. 安装：make install
7. demo：最完整的模块实例