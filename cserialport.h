#ifndef CSERIALPORT_H
#define CSERIALPORT_H

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
#include <string>

using namespace  std;


#define EPOLL_WAITE_TIME 2000
#define EPOLL_WAITE_MAX  1
class CSerialPort
{
public:
    CSerialPort(const char *PortName,
                int32_t BaudRate = 115200,
                int32_t ByteSize = 8,
                int32_t StopBits = 1,
                int8_t Parity = 'N');
    CSerialPort();
    ~CSerialPort();
    int32_t SetSpeed(int32_t, int32_t);
    int32_t SetParity(int32_t, int32_t, int32_t, int8_t);
    int32_t SetUart(int &, const char *, int32_t, int32_t, int32_t,int8_t);
    int32_t UartEpoll(int32_t &efd, int32_t &sfd);
    int32_t GetSerialfd() const;
    int32_t GetEpollfd() const;
    int32_t SendMessage(string &buffer, uint32_t buf_size);
    int32_t ReadMessage(string &buffer, uint32_t buf_size);


    int32_t SendMessage(uint8_t *buffer, uint32_t buf_size);  
    int32_t ReadMessage(uint8_t *buffer, uint32_t buf_size);  
    uint16_t usMBCRC16(uint8_t *src, uint16_t srclen);
    bool isConnected();
    void CloseSerial();
private:
    int32_t serial_fd = -1;
    int32_t epoll_fd = -1;  //epoll fd
    struct epoll_event wait_event;
};

uint16_t usMBCRC16(uint8_t *src, uint16_t srclen);

#endif // CSERIALPORT_H
