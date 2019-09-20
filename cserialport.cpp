#include "cserialport.h"
#include <iostream>
using namespace std;
int32_t speed_arr[] = {B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
                   B38400, B19200, B9600, B4800, B2400, B1200, B300,
                  };

int32_t name_arr[] = {115200, 38400,  19200,  9600,  4800,  2400,  1200,  300,
                  38400,  19200,  9600, 4800, 2400, 1200,  300,
                 };
int32_t CSerialPort::SetSpeed(int32_t fd, int32_t speed)
{
    int32_t   i;
    int32_t   status;
    struct termios   Opt;
    int32_t res = 0;

    tcgetattr(fd, &Opt);
    for ( i = 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
    {
        if  (speed == name_arr[i])
        {
            tcflush(fd, TCIOFLUSH);
            cfsetispeed(&Opt, speed_arr[i]);
            cfsetospeed(&Opt, speed_arr[i]);
            status = tcsetattr(fd, TCSANOW, &Opt);
            if (status != 0)
            {
                perror("tcsetattr fd1");
                res = -1;
            }
            return res;
        }
        tcflush(fd, TCIOFLUSH);
    }
}



int32_t CSerialPort::SetParity(int32_t fd, int32_t databits, int32_t stopbits, int8_t parity)
{
    struct termios options;
    if  ( tcgetattr( fd, &options)  !=  0)
    {
        perror("SetupSerial");
        return (-1);
    }

    options.c_cflag &= ~CSIZE;
    switch (databits)
    {
    case 7:
        options.c_cflag |= CS7;
        break;
    case 8:
        options.c_cflag |= CS8;
        break;
    default:
        fprintf(stderr, "Unsupported data size\n");
        return (-1);
    }

    switch (parity)
    {
    case 'n':
    case 'N':
        options.c_cflag &= ~PARENB;   /* Clear parity enable */
        options.c_iflag &= ~INPCK;     /* Enable parity checking */
        options.c_iflag &= ~(ICRNL | IGNCR);
        options.c_lflag &= ~(ICANON );
        break;
    case 'o':
    case 'O':
        options.c_cflag |= (PARODD | PARENB);
        options.c_iflag |= INPCK;             /* Disnable parity checking */
        break;
    case 'e':
    case 'E':
        options.c_cflag |= PARENB;     /* Enable parity */
        options.c_cflag &= ~PARODD;
        options.c_iflag |= INPCK;       /* Disnable parity checking */
        break;
    case 'S':
    case 's':  /*as no parity*/
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        break;
    default:
        perror("Unsupported parity");
        return (-1);
    }

    switch (stopbits)
    {
    case 1:
        options.c_cflag &= ~CSTOPB;
        break;
    case 2:
        options.c_cflag |= CSTOPB;
        break;
    default:
        perror("Unsupported stop bits");
        return (-1);
    }

    options.c_cc[VTIME] = 10; // 1 seconds
    options.c_cc[VMIN] = 0;

    options.c_lflag &= ~(ECHO |ECHOE | ECHOK | ECHONL | ISIG |IEXTEN);
    options.c_iflag &= ~(IXON | IXOFF);
    options.c_oflag &= ~OPOST;

    tcflush(fd, TCIFLUSH); /* Update the options and do it NOW */
    if (tcsetattr(fd, TCSANOW, &options) != 0)
    {
        perror("Update the options");
        return (-1);
    }
    return (1);
}


int32_t CSerialPort::UartEpoll(int32_t &efd, int32_t &sfd)
{
    struct epoll_event event;
    efd = epoll_create(10);
    if(efd < 0)
    {
        perror("uart create epoll error");
        return -1;
    }
    event.data.fd = sfd;
    event.events = EPOLLIN;
    if(epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event) < 0)
    {
        perror("epoll add event error");
        return -1;
    }
    return 0;

}
CSerialPort::CSerialPort(const char *PortName,
                         int32_t BaudRate,
                         int32_t ByteSize,
                         int32_t StopBits,
                         int8_t Parity)
{
    if(NULL == PortName)
    {
        perror("please appoint com port");
        exit(1);
    }
    if(SetUart(serial_fd, PortName, BaudRate, ByteSize, StopBits, Parity) >= 0)
    {
        UartEpoll(epoll_fd, serial_fd);
    }
}

int32_t CSerialPort::SetUart(int &fd, const char *dev, int32_t BaudRate, int32_t ByteSize, int32_t StopBits, int8_t Parity)
{
    int32_t res = 0;
    fd = open(dev, O_RDWR| O_NDELAY | O_NOCTTY);
    if (fd > 0)
    {
        res = SetSpeed(fd, BaudRate);
        if(res > 0)
        {
            res = SetParity(fd, ByteSize, StopBits, Parity);
            if ( res == -1)
            {
                perror("Set Parity Error\n");
            }  
        }      
    }
    else
    {
        perror("Can't Open Serial Port!\n");
        res = -1;
    }
    return res;
}
int32_t CSerialPort::SendMessage(string &buffer, uint32_t buf_size)
{
     int32_t wr_len;
     int32_t ret;
     ret = flock(serial_fd, LOCK_EX);
     if(ret < 0)
     {
        perror("lock serial port error");
        return -1;
     }
     wr_len = write(serial_fd, buffer.c_str(), buf_size);
     ret = flock(serial_fd, LOCK_UN);
     if(ret < 0)
     {
         perror("unlock serial port error");
     }
     return wr_len;
}

int32_t CSerialPort::SendMessage(uint8_t *buffer, uint32_t buf_size)
{
    int32_t wr_len;
    int32_t ret;
    ret = flock(serial_fd, LOCK_EX);
    if(ret < 0)
    {
        perror("lock serial port error");
        return -1;
    }
    #ifdef DEBUG_M
    printf("send len:%d\n", buf_size);
    
    for(int i =0; i < buf_size; i++)
    {
        printf("0x%X ", buffer[i]);
    }
    
    cout << endl;
    #endif
    wr_len = write(serial_fd, buffer, buf_size);
    ret = flock(serial_fd, LOCK_UN);
    if(ret < 0)
    {
        perror("unlock serial port error");
    }
    return wr_len;
}

int32_t CSerialPort::ReadMessage(string &buffer, uint32_t buf_size)
{
    struct epoll_event wait_event;
    int32_t epoll_ret = -1;
    int32_t read_len = 0, ret = -1;
    epoll_ret = epoll_wait(epoll_fd, &wait_event, EPOLL_WAITE_MAX, EPOLL_WAITE_TIME);
    if(epoll_ret < 0)
    {
        perror("uart epoll wait fault");
    }
    else if(epoll_ret > 0)
    {
        if((wait_event.data.fd == serial_fd) && (EPOLLIN == (wait_event.events & EPOLLIN)))
        {
            do{
                ret = read(serial_fd, &buffer[0], buf_size);
                if(ret > 0)
                {
                    read_len += ret;
                }
            }while(ret > 0);
        }
    }
    else {
        cout << "epoll wait time out\r\n";
    }
    return read_len;
}

int32_t CSerialPort::ReadMessage(uint8_t *buffer, uint32_t buf_size)
{
    struct epoll_event wait_event;
    int32_t epoll_ret = -1;
    int32_t read_len = 0, ret = -1;
    int32_t timeout = 0;
    do
    {
        epoll_ret = epoll_wait(epoll_fd, &wait_event, EPOLL_WAITE_MAX, EPOLL_WAITE_TIME);
        if(epoll_ret < 0)
        {
            perror("uart epoll wait fault");
        }
        else if(epoll_ret > 0)
        {
            if((wait_event.data.fd == serial_fd) && (EPOLLIN == (wait_event.events & EPOLLIN)))
            {
                ret = read(serial_fd,&buffer[read_len], buf_size);
                if(ret > 0)
                {
                    read_len += ret;
                }
            }
        }
        else 
        {            
            if(++timeout > 5);
            {
                cout << "epoll wait time out\r\n";
                return -1;
            }

        }
    } while (read_len < buf_size);    

    return read_len;
}



int32_t CSerialPort::GetSerialfd() const
{
    return serial_fd;
}
int32_t CSerialPort::GetEpollfd() const
{
    return epoll_fd;
}

void CSerialPort::CloseSerial()
{
    if(serial_fd > 0)
    {
        close(serial_fd);
    }
    if(epoll_fd > 0)
    {
        close(epoll_fd);
    }
    cout << "closeSerial" << endl;

}

bool CSerialPort::isConnected()
{
    return (serial_fd > 0);
}

CSerialPort::~CSerialPort()
{
    CloseSerial();
    cout << "destruct " << endl;
}


uint16_t usMBCRC16(uint8_t *src, uint16_t srclen)
{
	uint8_t aucCRCHi[] = {
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
			0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
			0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
			0x00, 0xC1, 0x81, 0x40
	};
	
	uint8_t aucCRCLo[] = {
			0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
			0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
			0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
			0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
			0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
			0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
			0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
			0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
			0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
			0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
			0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
			0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
			0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB,
			0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
			0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
			0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
			0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
			0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
			0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
			0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
			0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
			0x41, 0x81, 0x80, 0x40
	};

	uint8_t           ucCRCHi = 0xFF;
    uint8_t           ucCRCLo = 0xFF;
    uint8_t             iIndex;
	
    while ( srclen-- )
    {
        iIndex = ucCRCLo ^ *(src++);
        ucCRCLo =  ucCRCHi ^ aucCRCHi[iIndex];
        ucCRCHi = aucCRCLo[iIndex];
    }
    return(uint16_t)(ucCRCHi << 8 | ucCRCLo);
}
