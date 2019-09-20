#include "cmobject.h"
#include <string>
#include <iostream>
#include <sys/msg.h>
#include <sys/time.h>
#include <chrono>
using namespace std;

CMobject::CMobject(string sub, uint8_t p, uint8_t in):rev_ecal(bind(&CMobject::do_receive, this))
{
    subject = sub;    
    port = p;
    interval = in;
    gettimeofday(&last_time, nullptr);
    cout << "cm constuct sub:" << subject << endl;
}

void CMobject::do_receive(void)
{
    cout << "enter in receive thread" << endl;
    eCAL::Initialize(0, nullptr, "power mointor subscriber ");
    eCAL::CSubscriber sub(subject, "std::string");
    string msg;
    while(1)
    {
        msg.clear();
        
        sub.Receive(msg, nullptr, -1);    
        cout << "get msg for topic:" << msg << endl;
        gettimeofday(&last_time, nullptr);
    }
    eCAL::Finalize();
  /*  
   while(1)
   {
       sleep(10);
   }*/
}

struct timeval CMobject::get_time()
{
    return last_time;
}

CMobject::~CMobject()
{
    
    cout << "cm destuct" << endl;
}