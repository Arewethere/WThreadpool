#include"ThreadPool.h"
#include"Task.h"
#include<iostream>
#include<unistd.h>
using namespace std;

class TestTask:public OETask{
    int doWork(void){
        cout<<this->getID()<<":"<<"is doing working"<<endl;
        sleep(100);
    }

};

int main(){
    OEThreadPool::ThreadPoolConfig config;
    config.nMaxThreadsNum=100;
    config.nMinThreadsNum=3;
    config.dbTaskAddThreadRate=0.6;
    config.dbTaskSubThreadRate=0.5;

    shared_ptr<OEThreadPool>thptr(new OEThreadPool);
    thptr->init(config);
    for(int i=0;i<10;i++){
        thptr->addTask(shared_ptr<TestTask>(new TestTask));
    }
    while(1);



}