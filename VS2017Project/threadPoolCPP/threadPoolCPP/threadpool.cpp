#include "threadpool.h"
#include <iostream>
#include <string.h>
 #ifndef DEBUG
 #define DEBUG 1
 #endif
using namespace std;
void* threadPool::manager(threadPool* arg)
{

}
void* threadPool::worker(threadPool* arg)
{
	
	while (1)
	{
		pthread_mutex_lock(&mutexPool);
		//当任务队列为空的时候需要一直判断
		while (Front == Rear && false == shutdown)
		{
			pthread_cond_wait(&notEmpty, &mutexPool);
			if (exitNum > 0)
			{
				exitNum--;
				if (threadLiveNum > minNum)
				{
					cout << "thread" << pthread_self() << "exiting becase exitNum > 0" << endl;
					exitThread();
				}
			}
		}
		if (shutdown)
		{
			cout << "threadID:" << pthread_self() << "exiting becase shutdown == true" << endl;
			exitThread();

		}


		pthread_mutex_lock(&mutexPool);
	}
	
	
}
static threadPool* createPool( int max, int min, int taskQueueSize)
{
	threadPool* pool = new threadPool(max, min, taskQueueSize);
	return pool;
}

//构造函数,
threadPool::threadPool(int max, int min, int taskQueueSize)
{
	if (0 == ThreadPoolInit(max, min, taskQueueSize))
	{
		std::cout << "init pool fail..." << std::endl;
	}
}


/*辅助函数*/
int threadPool::ThreadPoolInit(int max, int min, int taskQueueSize)
{
	do {
		/*初始化taskQ队列， */
		TaskQ = new std::vector<Task>;
		if (nullptr == TaskQ)
		{
			std::cout << "new TaskQ fail..." << std::endl;
			break;
		}
		for (int i = 0; i < taskQueueSize; i++)
		{
			Task temp = Task{ nullptr, nullptr };
			TaskQ->push_back(temp);
		}
		Front =  Rear = TaskQ->begin();
		/*初始化taskQ完毕 */

		pthread_create(&managerID, nullptr, this->manager, this);
		workerIDs = new pthread_t[max];
		if (nullptr == workerIDs)
		{
			std::cout << "new workerID fail..." << std::endl;
			break;
		}
		for (int i = 0; i < max; i++)
		{
			pthread_create(&workerIDs[i], nullptr, this->worker, this);
		}
		threadBusyNum = 0;//线程池中忙线程的数量，这个值一直在变需要高频访问，所以单独用个锁
		threadLiveNum = min;
		maxNum = max;
		minNum = min;
		exitNum = 0;
		if (pthread_mutex_init(&mutexPool, nullptr) != 0 ||
			pthread_mutex_init(&mutexBusy, nullptr) != 0 ||
			pthread_cond_init(&notEmpty, nullptr) != 0 ||
			pthread_cond_init(&notFull, nullptr) != 0)
		{
			std::cout << "mutex or condition init fail..." << std::endl;
			break;
		}
		shutdown = false;
		return 1;
	} while (0);
	if (TaskQ) delete TaskQ;
	if (workerIDs) delete[] workerIDs;
	return 0;
}