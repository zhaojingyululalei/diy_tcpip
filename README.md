# diy_tcpip

uint8_t rbuf[256] ;
    package_read(pkg,rbuf,pkg->total);
    for (int i = 0; i < pkg->total; ++i)
    {

       dbg_info("%x ", buf[i]);
        
    }
   dbg_info("\r\n");

   760	4398.126458763	192.168.169.10	192.168.169.20	ICMP	98	Echo (ping) reply    id=0x0018, seq=1/256, ttl=64 (request in 759)