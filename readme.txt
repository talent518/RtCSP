ʵʱ�������л�Э��(Real-time chat serialization protocol)

1. ��������(autotools):./autogen.sh
	./autogen.sh 1 Ĭ�ϵ�glib��libevent��λ����/usr/local�����Զ�ִ�� ./configure --with-libevent=/usr/local --with-glib=/usr/local
2. ���ã�./configure
	--with-glib=/usr/local : glib(>=2.40.0) install path
	--with-libevent=/usr/local : libevent(>=2.0.21) install path

3. ���룺make
4. ���з���ˣ�./rtcspd -s start
5. ����ѹ�����ԣ�./rtcspb
6. ��װ��make install
7. demo����������ģ��ʵ��