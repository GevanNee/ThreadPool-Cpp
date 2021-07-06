#pragma once
#include "pthread.h"
#include <queue>
class threadPool
 {
private:
	//单个任务
    typedef struct Task
    {
        void (*func)(void* arg);
        void* arg;
    }Task;
	//任务组成的队列
	

public:
    int ThreadPooldestory(threadPool* pool);
    void addTask(void (*func), void* arg);
public:
    int LiveThreadNum(threadPool* pool);
    int TaskNum(threadPool* pool);
	static threadPool* createPool(threadPool&pool, int max, int min, int taskQueueSize);
	threadPool(int max, int min, int taskQueueSize);
	threadPool();
private://辅助函数
	~threadPool();
	void exitThread();
	int ThreadPoolInit(int max, int min, int taskQueueSize);
	void* manager(threadPool* arg);
	void* worker(threadPool* arg);
 private://Task队列，用vector和两个迭代器实现不删除元素的循环队列，也可以用循环链表
	std::vector<Task>* TaskQ;
	std::vector<Task>::iterator Front;
	std::vector<Task>::iterator Rear;
 private://工作线程和管理者线程的ID
	pthread_t managerID;
	pthread_t* workerIDs;
 private://线程池的属性变量
	int threadBusyNum;//线程池中的忙线程数量
	int threadLiveNum;//线程池中存活线程的数量
	int maxNum;//线程池中最大线程数量
	int minNum;//线程池中最小线程数量
	int exitNum;//线程池中需要销毁的线程的数量
	bool shutdown;//判断线程是否应该被销毁
 private://线程池中的锁
	pthread_mutex_t mutexPool;//整个池子的锁
	pthread_mutex_t mutexBusy;//变量 threadBusyNum 的专属锁，因为该变量访问频率很高
	pthread_cond_t notEmpty;//该条件变量用来唤醒生产者(addTask函数)
	pthread_cond_t notFull;//该条件变量用来唤醒消费者(worker函数)
	
};