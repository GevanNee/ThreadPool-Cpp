#include <iostream>
#include "threadpool.h"
using namespace std;
const int TASK_ADD_NUM = 100; //准备添加的任务的数量
pthread_mutex_t mutex;
int a = 0;
void counter(void* arg)
{
	
	int num = *(int*)arg;
	pthread_mutex_lock(&mutex);
	a++;
	cout << "threadID = " << pthread_self() << " a = " << a << endl;
	pthread_mutex_unlock(&mutex);
	usleep(100);
}

int main()
{
	pthread_mutex_init(&mutex, nullptr);
	threadPool* pool = threadPool::poolCreate(10, 2, 100);
	for (int i = 0; i < 200; i++)
	{
		threadPool::addTask(pool, counter, new int(12));
	}
	sleep(2);
	threadPool::threadPoolDestory(pool);
	//delete num;
	sleep(2);
	return 0;
}