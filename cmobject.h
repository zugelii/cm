#ifndef COBJECT_H
#define COBJECT_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <termios.h>
#include <errno.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <string>
#include <ecal/ecal.h>
#include <ecal/msg/protobuf/publisher.h>
#include <thread>
using namespace std;


class CMobject
{
public:
    CMobject(string subj="foo", uint8_t p=3, uint8_t in = 50);
    ~CMobject();
    void do_receive(void);
    struct timeval get_time();
    uint8_t port;
    uint8_t interval;
    thread rev_ecal;

private:
    struct timeval last_time;
    string subject;
    
};




#endif