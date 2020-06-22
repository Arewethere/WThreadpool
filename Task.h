/*
任务的基础类，定义了基础的任务接口
*/
#ifndef __OETASK_H__
#define __OETASK_H__

#include <time.h>
#include <atomic>

// 任务基类
class OETask
{

protected:

    // 任务的唯一标识
    size_t                      id_;
    // 任务创建时间 （非Unix时间戳）
    clock_t                     createTime_;

private:
    //负责任务ID
    static std::atomic<size_t>  nRequestID_;
    // 任务取消状态
    std::atomic<bool>           isCancelRequired_;

public:

    OETask(void) :id_(nRequestID_++), isCancelRequired_(false),
        createTime_(clock()){}

    virtual ~OETask(void) {};

public:

    // 任务类虚接口，继承这个类的必须要实现这个接口
    virtual int 
	doWork(void) = 0;

    // 任务已取消回调
    virtual int 
	onCanceled(void){ return 1; }

    // 任务已完成
    virtual int 
	onCompleted(int){ return 1; }

    // 任务是否超时
    virtual bool 
	isTimeout(const clock_t& now) {
        return ((now - createTime_) > 5000);
    }

    // 获取任务ID

    size_t 
	getID(void){ return id_; }

    // 获取任务取消状态
    bool 
	isCancelRequired(void){ return isCancelRequired_; }

    // 设置任务取消状态
    void 
	setCancelRequired(void){ isCancelRequired_ = true; }


};

__declspec(selectany) std::atomic<size_t> OETask::nRequestID_ = 100000;

#endif // __OETASK_H__

