#include "threadpool.h"

 #ifndef DEBUG
 #define DEBUG 0
 #endif
#ifndef PRINT
#define PRINT 0
#endif
const int ADD_THREAD_NUM = 2;
const int DEC_THREAD_NUM = 2;
const int DEFAULT_MIN_NUM = 3;
const int DEFAULT_MAX_NUM = 10;
const int DEFAULT_TASKQ_CAPACITY = 100;

using namespace std;
void* threadPool::manager(void* arg)
{
#if DEBUG
	cout << "manager start..." << endl;
#endif
	threadPool* pool = (threadPool*)arg;
	while (pool->shutdown)
	{
		sleep(3);
		pthread_mutex_lock(&pool->mutexPool);
		int tasknum = pool->taskNum;
		int liveNum = pool->threadLiveNum;
		pthread_mutex_unlock(&pool->mutexPool);

		pthread_mutex_lock(&pool->mutexBusy);
		int busyNum = pool->threadBusyNum;
		pthread_mutex_unlock(&pool->mutexBusy);
		//添加线程
		if (tasknum > busyNum && liveNum < pool->maxNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			int counter = 0;
			for (int i = 0;
				i < pool->maxNum && counter < ADD_THREAD_NUM && pool->threadBusyNum < pool->maxNum;
				i++)
			{
				if (0 == pool->threadIDs[i])
				{

					pthread_t tid;
					tid = pthread_create(&pool->threadIDs[i], nullptr, worker, pool);
					counter++;
#if DEBUG
					cout << "thread " << tid << " added" << endl;
#endif
					pool->threadLiveNum++;
				}
			}
			pthread_mutex_unlock(&pool->mutexPool);
		}
		//销毁线程
		if (busyNum * 2 > pool->threadLiveNum && pool->threadLiveNum > pool->minNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			pool->exitNum = DEC_THREAD_NUM;
			pthread_mutex_unlock(&pool->mutexPool);

			for (int i = 0; i < DEC_THREAD_NUM; i++)
			{
				pthread_cond_signal(&pool->notEmpty);
			}
		}
	}
#if DEBUG
	cout << "manager end..." << endl;
#endif
}
void* threadPool::worker(void* arg)
{
#if DEBUG
	cout << "worker start..." << endl;
#endif
	threadPool* pool = (threadPool*)arg;
	if (nullptr == pool)
	{
		cout << "pool is nullptr" << endl;
		return nullptr;
	}
#if DEBUG
	else
	{
		cout << "pool->maxNum = " << pool->maxNum << endl;
		cout << "pool->minNum = " << pool->minNum << endl;
		cout << "this threadid is: " << pthread_self() << endl;
	}
#endif
	while (1)
	{
		pthread_mutex_lock(&pool->mutexPool);
		//当任务队列为空的时候需要一直判断
		while (0 == pool->taskNum && false == pool->shutdown)
		{
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
			if (pool->exitNum > 0)
			{
				pool->exitNum--;
				if (pool->threadLiveNum > pool->minNum)
				{
#if PRINT
					cout << "thread" << pthread_self() << " exiting becase exitNum > 0" << endl;
#endif
					pthread_mutex_unlock(&pool->mutexPool);
					pool->exitThread();
				}
			}
		}
		if (pool->shutdown)
		{
#if PRINT
			cout << "threadID:" << pthread_self() << "exiting becase shutdown == true" << endl;
#endif
			pthread_mutex_unlock(&pool->mutexPool);
			pool->exitThread();
		}
		Task task;
		task.func = pool->taskFront->func;
		task.arg = pool->taskFront->arg;
		pool->taskFront++;
		if (pool->taskFront == pool->TaskQ->end()) { pool->taskFront = pool->TaskQ->begin(); }
		if (pool->taskNum > 0) { pool->taskNum--; }

		pthread_cond_signal(&pool->notFull);
		pthread_mutex_unlock(&pool->mutexPool);
		//执行
		pthread_mutex_lock(&pool->mutexBusy);
		pool->threadBusyNum++;
		pthread_mutex_unlock(&pool->mutexBusy);

		task.func(task.arg);

		pthread_mutex_lock(&pool->mutexBusy);
		pool->threadBusyNum--;
		pthread_mutex_unlock(&pool->mutexBusy);
		delete task.arg;
		task.arg = nullptr;
	}
#if DEBUG
	cout << "worker end..." << endl;
#endif
	return nullptr;
	
}
int threadPool::threadPoolDestory(threadPool * __Pool)
{
	threadPool * pool = __Pool;
	if (nullptr != pool) { delete pool; };
	return 0;
}
void threadPool::addTask(threadPool* pool, void (*Func)(void* ), void * Arg)
{
	threadPool* ptr = pool;
	if (nullptr == ptr || nullptr == Func)
	{
#if PRINT
		cout << "add task fail, funcptr is nullptr" << endl;
#endif
		return;
	}
	pthread_mutex_lock(&pool->mutexPool);
	while (&pool->taskNum == &pool->taskQCapacity && !pool->shutdown)
	{
		pthread_cond_wait(&pool->notFull, &pool->mutexPool);
	}
	pool->taskRear->func = Func;
	pool->taskRear->arg = Arg;
	pool->taskRear++;
	if (pool->taskRear == pool->TaskQ->end())
	{
		pool->taskRear = pool->TaskQ->begin();
	}
	pool->taskNum++;
	pthread_cond_signal(&pool->notEmpty);
	pthread_mutex_unlock(&pool->mutexPool);
}
threadPool* threadPool::poolCreate( int max, int min, int taskQueueSize)
{
	threadPool* pool = new threadPool(max, min, taskQueueSize);
	return pool;
}
threadPool* threadPool::poolCreate()
{
	threadPool* pool = new threadPool();
	return pool;
}
//构造函数
threadPool::threadPool(int max, int min, int taskQueueSize)
{
	if (0 == ThreadPoolInit(max, min, taskQueueSize))
	{
#if PRINT
		std::cout << "init pool fail..." << std::endl;
#endif
	}
}
//默认构造函数
threadPool::threadPool():threadPool(DEFAULT_MAX_NUM, DEFAULT_MIN_NUM, DEFAULT_TASKQ_CAPACITY){ }
//析构函数
threadPool::~threadPool()
{
	shutdown = true;
	pthread_join(managerID, nullptr);
	//唤醒阻塞的消费者

	for (int i = 0; i < threadLiveNum; i++)
	{
		pthread_cond_signal(&notEmpty);
	}

	vector<Task>::iterator itr = TaskQ->begin();
	for (int i = 0; i < taskQCapacity && itr != TaskQ->end(); i++, itr++)
	{
		if (itr->arg) 
		{ 
			//delete itr->arg; 
			//itr->arg = nullptr;
		}
	}

	if (TaskQ)
	{
		cout << "delete TaskQ" << endl;
		delete TaskQ;
		TaskQ = nullptr;
		cout << "delete TaskQ end" << endl;
	}
	if (threadIDs) { delete (threadIDs); }
	pthread_mutex_destroy(&mutexPool);
	pthread_mutex_destroy(&mutexBusy);
	pthread_cond_destroy(&notEmpty);
	pthread_cond_destroy(&notFull);
}
void threadPool::exitThread()
{
#if DEBUG
	cout << "exitThread start..." << endl;
#endif
	pthread_t tid = pthread_self();
	for (int i = 0; i < this->maxNum; i++)
	{
		if (tid == this->threadIDs[i])
		{
			threadIDs[i] = 0;
			break;
		}
	}
#if DEBUG
	cout << "exitThread end..." << endl;
#endif
	pthread_exit(nullptr);
}

int threadPool::ThreadPoolInit(int max, int min, int taskQueueSize)
{
	do {
		/*初始化taskQ队列， */
#if DEBUG
		cout << "init start..." << endl;
#endif
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
		taskFront =  taskRear = TaskQ->begin();
		taskNum = 0;
		taskQCapacity = taskQueueSize;
		/*初始化taskQ完毕 */
		pthread_create(&managerID, nullptr, manager , this);
		threadIDs = new pthread_t[max];
		memset(threadIDs, 0, sizeof(pthread_t) * max);
		if (nullptr == threadIDs)
		{
			std::cout << "new workerID fail..." << std::endl;
			break;
		}
		for (int i = 0; i < min; i++)
		{
			pthread_create(&threadIDs[i], nullptr, worker, this);
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
#if DEBUG
		cout << "init end..." << endl;
#endif
		return 1;
	} while (0);
	if (TaskQ) delete TaskQ;
	if (threadIDs) delete[] threadIDs;
	return 0;
}