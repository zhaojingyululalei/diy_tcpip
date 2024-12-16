# diy_tcpip

uint8_t rbuf[256] ;
    package_read(pkg,rbuf,pkg->total);
    for (int i = 0; i < pkg->total; ++i)
    {

        printf("%x ", buf[i]);
        
    }
    printf("\r\n");