#pragma once
#include "pthread.h"
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
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
    static int threadPoolDestory(threadPool* pool);
    static void addTask(threadPool* pool, void(*Func)(void*), void* arg);
public:
	
	
	static threadPool* poolCreate(int max, int min, int taskQueueSize);
	static threadPool* poolCreate();
private://辅助函数
	threadPool(int max, int min, int taskQueueSize);
	threadPool();
	~threadPool();
	void exitThread();
	int ThreadPoolInit(int max, int min, int taskQueueSize);

	
	static void* manager(void* arg);
	static void* worker(void* arg);
 private://Task队列，用vector和两个迭代器实现不删除元素的循环队列，也可以用循环链表
	std::vector<Task>* TaskQ;
	std::vector<Task>::iterator taskFront;
	std::vector<Task>::iterator taskRear;
	int taskQCapacity;
	int taskNum;
 private://工作线程和管理者线程的ID
	pthread_t managerID;
	pthread_t* threadIDs;
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
	pthread_cond_t notEmpty;//该条件变量用来唤醒消费者(worker函数)，提示TaskQ还没空，可以继续消费
	pthread_cond_t notFull;//该条件变量用来唤醒生产者(addtask函数)，提示TaskQ还没满，可以继续生产
	
};