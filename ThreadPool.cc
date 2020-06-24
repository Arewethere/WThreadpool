#include "ThreadPool.h"
#include <iostream>
#include <thread>

#include "Task.h"
#include "TaskQueue.h"

//OEThreadPool SystemThreadPool;

OEThreadPool::OEThreadPool(void)
:taskQueue_(new OETaskQueue<OETask>()), atcCurTotalThrNum_(0), atcWorking_(true) {
}

OEThreadPool::~OEThreadPool(void) {
    // @note: 曾经因析构自动调用 release 触发了错误
    release();
}

// 初始化资源
int OEThreadPool::init(const ThreadPoolConfig& threadPoolConfig) {
    // 错误的设置
    if (threadPoolConfig.dbTaskAddThreadRate < threadPoolConfig.dbTaskSubThreadRate)
        return 87;


    threadPoolConfig_.nMaxThreadsNum = threadPoolConfig.nMaxThreadsNum;
    threadPoolConfig_.nMinThreadsNum = threadPoolConfig.nMinThreadsNum;
    threadPoolConfig_.dbTaskAddThreadRate = threadPoolConfig.dbTaskAddThreadRate;
    threadPoolConfig_.dbTaskSubThreadRate = threadPoolConfig.dbTaskSubThreadRate;


    int ret = 0;
    // 创建线程池
    if (threadPoolConfig_.nMinThreadsNum > 0)
        ret = addProThreads(threadPoolConfig_.nMinThreadsNum);


    return ret;
}

// 添加任务
int OEThreadPool::addTask(std::shared_ptr<OETask> taskptr, bool priority) {
    const double& rate = getThreadTaskRate();
    int ret = 0;
    if (priority) { //如果添加的任务有优先级，那么插入到任务队列的头部

        if (rate > 1000)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        taskQueue_->put_front(taskptr);

    }
    else {

        // 检测任务数量
        if (rate > 100) {
            taskptr->onCanceled();
            return 298;
        }


        // 将任务推入队列
        taskQueue_->put_back(taskptr);

    }


    // 检查是否要扩展线程
    if (atcCurTotalThrNum_ < threadPoolConfig_.nMaxThreadsNum
        && rate > threadPoolConfig_.dbTaskAddThreadRate)
        ret = addProThreads(1);


    return ret;
}

// 删除任务（从就绪队列删除，如果就绪队列没有，则看执行队列有没有，有的话置下取消状态位）
int OEThreadPool::deleteTask(size_t nID) {
    return taskQueue_->deleteTask(nID);
}

// 删除所有任务
int OEThreadPool::deleteAllTasks(void) {
    return taskQueue_->deleteAllTasks();
}

//查询任务是否正在处理
std::shared_ptr<OETask> OEThreadPool::isTaskProcessed(size_t nId) {
    return taskQueue_->isTaskProcessed(nId);
}


// 释放资源（释放线程池、释放任务队列）
bool OEThreadPool::release(void) {
    // 1、停止线程池。2、清楚就绪队列。3、等待执行队列为0
    releaseThreadPool();
    taskQueue_->release();

    int i = 0;
    while (atcCurTotalThrNum_ != 0) {

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // 异常等待
        if (i++ == 10)
            exit(23);

    }

    atcCurTotalThrNum_ = 0;
    return true;
}

// 获取当前线程任务比
double OEThreadPool::getThreadTaskRate(void) {
    // @note :无所谓线程安全
    //std::unique_lock<std::mutex> lock(mutex_)

    if (atcCurTotalThrNum_ != 0)
        return taskQueue_->size() * 1.0 / atcCurTotalThrNum_;

    return 0;
}
// 当前线程是否需要结束
bool OEThreadPool::shouldEnd(void) {

    bool bFlag = false;
    double dbThreadTaskRate = getThreadTaskRate();

    // 检查线程与任务比率
    if (!atcWorking_ || atcCurTotalThrNum_ > threadPoolConfig_.nMinThreadsNum
        && dbThreadTaskRate < threadPoolConfig_.dbTaskSubThreadRate)
        bFlag = true;

    return bFlag;
}
// 添加指定数量的处理线程
int OEThreadPool::addProThreads(int nThreadsNum) {

    try {

        for (; nThreadsNum > 0; --nThreadsNum)
            std::thread(&OEThreadPool::taskProcThread, this).detach();

    }
    catch (...) {
        return 155;
    }

    return 0;
}
// 释放线程池
bool OEThreadPool::releaseThreadPool(void) {
    threadPoolConfig_.nMinThreadsNum = 0;
    threadPoolConfig_.dbTaskSubThreadRate = 0;
    atcWorking_ = false;
    return true;
}

// 任务处理线程函数
void OEThreadPool::taskProcThread(void) {
    int nTaskProcRet = 0;
    // 线程增加
    atcCurTotalThrNum_.fetch_add(1);
    std::chrono::milliseconds mills_sleep(500);


    std::shared_ptr<OETask> pTask;
    while (atcWorking_) {
        // 获取任务
        pTask = taskQueue_->get();
        if (pTask == nullptr) {

            if (shouldEnd())
                break;

            // 进入睡眠池
            taskQueue_->wait(mills_sleep);
            continue;

        }


        // 检测任务取消状态
        if (pTask->isCancelRequired())
            pTask->onCanceled();
        else
            // 处理任务
            pTask->onCompleted(pTask->doWork());


        // 从运行任务队列中移除任务
        taskQueue_->onTaskFinished(pTask->getID());


        // 判断线程是否需要结束
        if (shouldEnd())
            break;

    }

    // 线程个数减一
    atcCurTotalThrNum_.fetch_sub(1);
}