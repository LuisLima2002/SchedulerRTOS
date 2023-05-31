
#ifndef SCHEDULER_H
#define SCHEDULER_H
#define PRIVILEGED_FUNCTION
#define PRIVILEGED_DATA
 
#include <Arduino.h>
#define maxResources 5


typedef struct 
{
  TaskFunction_t taskCode;
  uint32_t computation;
  uint32_t period;
  char* taskName;
  void* params;
  int priority;
  uint32_t resourcesId[maxResources];
} taskInfo;

typedef struct 
{
  taskInfo* tasks;
  uint32_t n;
} taskList;

typedef struct 
{
  int cellingP;
  TaskHandle_t holder;
  TaskHandle_t* queue=NULL;
  int n=0;
  int originalP;
} semaphoreInfo;

void xTaskCreateAperiodic(pdTASK_CODE pvTaskCode, void *pvParameters,unsigned long computation);

void Periodic(void *pvParameters) PRIVILEGED_FUNCTION;

void ServerActivation(void *pvParameters) PRIVILEGED_FUNCTION;

void Server(void *pvParameters) PRIVILEGED_FUNCTION;

void TimerServer(TimerHandle_t xTimer) PRIVILEGED_FUNCTION;

void Scheduler(taskInfo **tasks,uint8_t size,int ServerC=2,int ServerP=20);

void down(semaphoreInfo *si);

void up(semaphoreInfo *si,TaskHandle_t xTask);

semaphoreInfo* createSemaphore(taskInfo* tasks,uint32_t id);

int compairPriority(const void *cmp1, const void *cmp2) PRIVILEGED_FUNCTION;

void startScheduler();

void pauseScheduler();

 
 
#endif