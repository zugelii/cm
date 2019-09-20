#include <iostream>
#include <stdio.h>
#include <thread>
#include <sys/msg.h>
#include <ecal/ecal.h>
#include <ecal/msg/protobuf/publisher.h>
#include "cserialport.h"
#include <sys/time.h>
#include <mutex>
#include <chrono>
#include <vector>
#include "cmobject.h"

using namespace std;
uint8_t poll_msg[] = {0x01, 0x04, 0x03, 0xE8, 0x00, 0x01, 0xB1, 0xBA};  //get relay status msg
//FE 10 00 03 00 02 04 00 04 00 32 40 B9   //wait 5s for close relay
uint8_t oper_msg[] = {0x01, 0x10, 0x00, 0x03, 0x00, 0x02, 0x04, 0x00, 0x04, 0x00, 0x32, 0x73, 0xAE};


#define MAX_TEXT 512
#define MAX_TIME_OUT  5
#define MAX_TRY_COUNT 3
#define SERIAL_BAUDRATE   9600
struct msg_st
{
	long int msg_type;
	uint8_t text[MAX_TEXT];
};


typedef struct moniter_o
{
	string subj;
	uint8_t port;
	struct timeval last_time;
	uint8_t interval;
}MONITERO;

MONITERO m[5];

mutex g_lock;

void GetTime(char *TimeStr)
{
	struct tm tSysTime = { 0 };
	struct timeval tTimeVal = { 0 };
	time_t tCurrentTime = { 0 };
	char szUsec[20] = { 0 };
	
	char szMsec[20] = { 0 };
	if (TimeStr == NULL)
	{
		return;
	}
	tCurrentTime = time(NULL);
	localtime_r(&tCurrentTime, &tSysTime);
	gettimeofday(&tTimeVal, NULL);
	snprintf(szUsec, 3, "%06d", tTimeVal.tv_usec);
	strncpy(szMsec, szUsec, 3);
	snprintf(TimeStr, 128, "[%04d.%02d.%02d %02d:%02d:%02d.%3.3s]", tSysTime.tm_year + 1900, tSysTime.tm_mon + 1, tSysTime.tm_mday, tSysTime.tm_hour, tSysTime.tm_min,
			tSysTime.tm_sec, szMsec);
}

void write_log_file(const char *filename, const char *functionname, uint32_t codeine, const char *content)
{
	FILE *fp = NULL;
	char log_content[1048] = { 0 };
	char timestr[128] = { 0 };
	if (filename == NULL || content == NULL)
	{
		return;
	}
	fp = fopen(filename, "at+");
	if (fp == NULL)
	{
		return;
	}
	GetTime(timestr);
	fputs(timestr, fp);
	memset(log_content, sizeof(log_content), 0);
	snprintf(log_content, sizeof(log_content) - 1, "[%s][%s][%04d]%s\n", filename, functionname, codeine, content);
	fputs(log_content, fp);
	fflush(fp);
	fclose(fp);
	fp = NULL;
}


#define WRITELOGFILE(msg)  write_log_file("./power_moniter.log", __FUNCTION__, __LINE__, msg)


int8_t do_send_msg(CSerialPort* s, uint8_t* buf, uint8_t send_len, uint8_t rev_len)
{
	size_t buf_len = 20;
	size_t read_len = 0;
	uint8_t rev_pool[buf_len] = {0};
	s->SendMessage(buf, send_len);
	memset(rev_pool, 0, buf_len);
	read_len = s->ReadMessage(rev_pool, rev_len);
	if(read_len > 0)
	{
		#ifdef DEBUG_M
		printf("readlen:%d\n", read_len);
		printf("0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n", rev_pool[0], rev_pool[1], rev_pool[2], rev_pool[3], rev_pool[4], rev_pool[5], rev_pool[6]);
		#endif
		uint16_t crc = usMBCRC16(rev_pool, read_len);
		if(crc == 0)
		{
			return 0;
		}
		else
		{
			return -1;
		}
		
	}
}
void process_serial(const string& port_name, const int32_t& msgid)
{
	size_t buf_len = 20;
	size_t read_len = 0;
	uint8_t send_pool[buf_len] = {0};
	uint8_t try_counts = 0;
	struct msg_st data;
	int32_t msgtype = 0;
	cout << "process_serial" << endl;
	
	prctl(PR_SET_NAME, "relay_serial");  

	auto serial = new CSerialPort(port_name.c_str(), SERIAL_BAUDRATE);
	if(serial->GetSerialfd() > 0)
	{
		while(1)
		{
			if(msgrcv(msgid, (void*)&data, BUFSIZ, msgtype, 0) < 0)
			{
				perror("get queue msg error");
			}
			else
			{
				/* code */
				try_counts = 0;
				do
				{
					memcpy(send_pool, &data.text[2], data.text[0]);
					if(!do_send_msg(serial, send_pool, data.text[0], data.text[1])) 
					{
						break;
					}

				}while(try_counts++ < MAX_TRY_COUNT);
				if(try_counts >= MAX_TRY_COUNT)
				{
					if(do_send_msg(serial, poll_msg, 8, 7) < 0)  ///check if relay is online
					{
						#ifdef DEBUG_M
						WRITELOGFILE("can't get relay message\n");
						#endif
					}
				}				
			}
		}
	}
}

uint32_t operate_relay(uint8_t port, uint16_t status, int32_t &msgid, uint16_t time)
{
	/* code */
	struct msg_st data;
	data.msg_type = 1;
	data.text[0] = sizeof(oper_msg);
	data.text[1] = 8;
	uint16_t CRC16;
	if((port == 3) || (port == 8) || (port == 13) || (port = 18))
	{
		oper_msg[3] = port;
	}
	if((time > 10) && (time < 250))
	{
		oper_msg[10] = time;
	}
	CRC16 = usMBCRC16(oper_msg, 11);
	oper_msg[11] = CRC16 & 0xFF;
	oper_msg[12] = (CRC16 >> 8) & 0xFF;
	memcpy(&data.text[2], oper_msg, sizeof(oper_msg) + 2);
	if(msgsnd(msgid, (void*)&data, MAX_TEXT, 0) == -1)
	{
		perror("msgsnd failed");
		WRITELOGFILE("msgsnd failed");				
	}	
	#ifdef DEBUG_M
	operate_relay("opr_relay  send msg\n");
	#endif
}

void onreceive(const char* topic_name_, const struct eCAL::SReceiveCallbackData* data_)
{
	uint8_t i = 0;
	#ifdef DEBUG_M
    printf("We received %s on topic %s\n", (char*)data_->buf, topic_name_);
	#endif
    while((m[i].port != 0) && i < 5)
	{
		if(memcmp(m[i].subj.c_str(), topic_name_, strlen(m[i].subj.c_str())) == 0)
		{
			lock_guard<mutex>locker(g_lock);
			gettimeofday(&m[i].last_time, nullptr);
			break;
		}
		i++;
	}
}

void do_check(struct timeval& time, int32_t& msgid, CMobject& cm)
{
	if(time.tv_sec - cm.get_time().tv_sec > MAX_TIME_OUT )
	{
		operate_relay(cm.port, 0, msgid, cm.interval);
		#ifdef DEBUB_M
		printf("\n relay port:%d\n", cm.port);
		#endif
	}
}


int main(int argc, char**argv)
{
	bool simulation = false;	
	int32_t msgid = -1;	
	struct timeval now_time;

	string ser("/dev/ttyUSB0");
    if (argc >=2)
	{
		if(strlen(argv[1]) > 0)
		{
			ser.clear();
			ser.append(argv[1]);
			cout << "ser:" << ser << endl;
		}
	}
	#ifdef DEBUG_M
    cout << "Hello, Relay!\n";
	cout << "serial port:" << ser << endl;
	#endif
	prctl(PR_SET_NAME, "RelayEM");    

	msgid = msgget((key_t)0x7653, 0666 | IPC_CREAT);
	if(msgid < 0)
	{
		perror("msgget failed");
		WRITELOGFILE("msgget failed");
		exit(1);
	}
	gettimeofday(&now_time, nullptr);

	/*
	m[0].subj.append("foo1");
	m[0].port = 3;
	m[0].last_time = now_time;
	m[0].interval = 50;
	m[1].subj.append("foo2");
	m[1].port = 8;
	m[1].last_time = now_time;
	m[1].interval = 50;
	m[2].subj.append("foo3");
	m[2].port = 13;
	m[2].last_time = now_time;
	m[2].interval = 50;
	m[3].subj.append("foo4");
	m[3].port = 18;
	m[3].last_time = now_time;	
	m[3].interval = 50;
	///m[4].subj.append(nullptr);
	m[4].port = 0;
	m[4].last_time = now_time;	
	m[4].interval = 50;


	eCAL::Initialize(0, nullptr, "power mointor subscriber ");
    eCAL::CSubscriber cam_sub(m[0].subj.c_str(), "std::string");
    cam_sub.AddReceiveCallback(onreceive);

    eCAL::CSubscriber can_sub(m[1].subj.c_str(), "std::string");
    can_sub.AddReceiveCallback(onreceive);

	eCAL::CSubscriber rsu_sub(m[2].subj.c_str(), "std::string");
    rsu_sub.AddReceiveCallback(onreceive);

	eCAL::CSubscriber ecu_sub(m[3].subj.c_str(), "std::string");
    ecu_sub.AddReceiveCallback(onreceive);
*/

	CMobject n1("foo1", 3);
	sleep(3);
	CMobject n2("foo2", 8);
	sleep(3);
	CMobject n3("foo3", 13);
	sleep(3);
	CMobject n4("foo4", 18);

	try
	{
		thread process_ser(process_serial, ser, msgid);
		/*
		while (1)
		{
			gettimeofday(&now_time, nullptr);
			uint8_t i = 0;
			while (m[i].port != 0)
			{
				if(now_time.tv_sec - m[i].last_time.tv_sec > MAX_TIME_OUT )
				{
					operate_relay(m[i].port, 0, msgid, m[i].interval);
					#ifdef DEBUB_M
					printf("\n relay port:%d\n", m[i].port);
					#endif
				}
				i++;
			} 
			std::this_thread::sleep_for(std::chrono::milliseconds(7000));		
		}
		*/

		while (1)
		{
			gettimeofday(&now_time, nullptr);
			do_check(now_time, msgid, n1);
			do_check(now_time, msgid, n2);
			do_check(now_time, msgid, n3);
			do_check(now_time, msgid, n4);
		
			std::this_thread::sleep_for(std::chrono::milliseconds(7000));
		}
		cout << "enter error" << endl;
		process_ser.join();
		n1.rev_ecal.join();
		n2.rev_ecal.join();
		n3.rev_ecal.join();
		n4.rev_ecal.join();
		msgctl(msgid, IPC_RMID, 0);
	}
	catch(const exception& ex)
	{
		cout << "get to exception" << endl;
		cout << ex.what();
	}

}
