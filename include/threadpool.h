struct Task;
struct Threadpool;

//创建线程池
struct Threadpool* threadpoolCreate(int min, int max, int queueCapacity);
//销毁线程池
int threadpoolDestroy(struct Threadpool* pool);

void threadpoolAdd(struct Threadpool* pool, void (*function)(void* arg), void* arg);

int threadpoolbusyNum(struct Threadpool* pool);

int threadpoolliveNum(struct Threadpool* pool);

void* worker(void* arg);
void* manager(void* arg);
void threadExit(struct Threadpool* pool);