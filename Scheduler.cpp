#include "Scheduler.h"

int cellingSystem = -1 PRIVILEGED_DATA;
//int *cellingSystem=(int *) malloc(10*sizeof(int)) PRIVILEGED_DATA;
uint8_t sizeT=0 PRIVILEGED_DATA;
TaskHandle_t serverHandler PRIVILEGED_DATA;
TaskHandle_t *tasksHandler PRIVILEGED_DATA;
TimerHandle_t xTimerServer PRIVILEGED_DATA;
taskList *pxReadyAperiodicTasksLists PRIVILEGED_DATA;
bool serverOn=false PRIVILEGED_DATA;
bool serverPaused=false PRIVILEGED_DATA;

int compairPriority(const void *cmp1, const void *cmp2){
  taskInfo a = *((taskInfo *)cmp1);
  taskInfo b = *((taskInfo *)cmp2);

  return a.period - b.period;
}

/*
data 
*/

void up(semaphoreInfo *si,TaskHandle_t xTask){
  int pTask = uxTaskPriorityGet(xTask);
  vTaskPrioritySet(xTask,configMAX_PRIORITIES-1);

  if(si->holder!=NULL ||pTask<cellingSystem /*(cellingQueue>0 && pTask<cellingSystem[0])*/){
    //Serial.println("Bloqueando tarefa");
    if(si->n==0){
      si->n++;
      si->queue = (TaskHandle_t *) malloc(sizeof(TaskHandle_t));
      Serial.println("Criando queue");
    }else{
      Serial.println("Adicionando queue");
      si->n++;
      si->queue = (TaskHandle_t *) realloc(si->queue,si->n*sizeof(TaskHandle_t));
    }
    for(int i = si->n-1;i>0;i--){
      si->queue[i]=si->queue[i-1];
    }
    si->queue[0] = xTask;
    vTaskPrioritySet(si->holder,pTask+1);
    vTaskSuspend(xTask);
    //Serial.println("Mudou a prioridade");
    //Serial.println("Saiu do suspens");
  }else{
    //Serial.println("Aceitando tarefa");
    si->originalP=pTask;
    si->holder =xTask;
    /*if(cellingQueue==0){
      Serial.println("Criando lista");
      cellingQueue++;
      //cellingSystem = (int *) malloc(2*sizeof(int));
    }else{
      Serial.println("Aumentado lista");
      cellingQueue++;
      //cellingSystem = (int * ) realloc(cellingSystem,cellingQueue*sizeof(int));
      for(int i = cellingQueue-1; i > 0; i--) {
        si->queue[i] = si->queue[i-1];
      }
    }
    Serial.println("Adicionando na lista");*/
    cellingSystem = si->cellingP;
  }
  vTaskPrioritySet(xTask,pTask);
}

void down(semaphoreInfo *si){
  vTaskPrioritySet( si->holder,configMAX_PRIORITIES-1);
  if(si->n>0){
      Serial.println("Queue tem item");
    TaskHandle_t aux= si->holder;
    int auxP= si->originalP;
    si->holder = si->queue[0];
    si->originalP= uxTaskPriorityGet(si->holder);
    if(si->n>1){
      Serial.println("Queue mais de 1 item");
      for(int i = 0; i < si->n; i++) {
        si->queue[i] = si->queue[i+1];
      }
      si->n--;
    si->queue = (TaskHandle_t *) realloc(si->queue, si->n*sizeof(TaskHandle_t));
    }else{
      Serial.println("Queue apenas 1 item");
      si->n--;
      delete si->queue;
    }
    //Serial.println("Liberando tarefa E resume queue");
    vTaskPrioritySet(aux,auxP);
    vTaskResume(si->holder);
  }else{
      Serial.println("Queue nao tem item");
    TaskHandle_t aux= si->holder;
    int auxP= si->originalP;
    si->holder = NULL;
   /* if(cellingQueue==1){
      cellingQueue--;
      delete cellingSystem;
      cellingSystem = NULL;
    }else{
      for(int i = 0; i < cellingQueue; i++) {
        si->queue[i] = si->queue[i+1];
      }
      cellingQueue--;
      cellingSystem = (int * ) realloc(cellingSystem,cellingQueue*sizeof(int));
    }*/
    cellingSystem = -1;
    //Serial.println("Liberando tarefa");
    vTaskPrioritySet(aux,auxP);
  }
}

void Scheduler(taskInfo **tasks,uint8_t size,int ServerC,int ServerP){
  pxReadyAperiodicTasksLists= (taskList * ) malloc(sizeof(taskList));
  pxReadyAperiodicTasksLists->tasks=NULL;
  pxReadyAperiodicTasksLists->n=0;
  (*tasks) = (taskInfo*) realloc((*tasks),(size+1)*sizeof(taskInfo));
  size++;
  (*tasks)[size-1].taskCode = ServerActivation;
  (*tasks)[size-1].taskName = "Server";
  (*tasks)[size-1].computation = ServerC;
  (*tasks)[size-1].period = ServerP;
  (*tasks)[size-1].params = (void *) pxReadyAperiodicTasksLists;
  xTimerServer =xTimerCreate("ServerTimer",(*tasks)[size-1].computation*1000,pdFALSE,( void * ) 0,TimerServer);
  // sort
  qsort((*tasks),size,sizeof((*tasks)[0]),compairPriority);
  //schedule
  int prio = uxTaskPriorityGet(xTaskGetCurrentTaskHandle());
  vTaskPrioritySet(xTaskGetCurrentTaskHandle(),configMAX_PRIORITIES-1);
  tasksHandler = (TaskHandle_t *) malloc((size)*(sizeof(TaskHandle_t)));
  for (int i = 0; i < size; i++)
  {
    (*tasks)[i].priority = configMAX_PRIORITIES-2-i*2;
    if((*tasks)[i].taskName=="Server"){
      xTaskCreatePinnedToCore((*tasks)[i].taskCode, (*tasks)[i].taskName, 10000,(void *) &(*tasks)[i], configMAX_PRIORITIES-2-i*2, &tasksHandler[i],1);
      xTaskCreatePinnedToCore(Server, (*tasks)[i].taskName, 10000,(void *) &(*tasks)[i], configMAX_PRIORITIES-2-i*2, &serverHandler,1);
      vTaskSuspend(serverHandler);
    }else{
      xTaskCreatePinnedToCore(Periodic, (*tasks)[i].taskName, 10000,(void *) &(*tasks)[i], configMAX_PRIORITIES-2-i*2, &tasksHandler[i],1);
    }
    vTaskSuspend(tasksHandler[i]);
  }
  sizeT = size;
  vTaskPrioritySet(xTaskGetCurrentTaskHandle(),prio);
};

void TimerServer(TimerHandle_t xTimer){
  Serial.println("\nSuspend task");
  vTaskSuspend(serverHandler);
  serverOn=false;
}

void Server(void *pvParameters){
  for(;;){
    while (pxReadyAperiodicTasksLists->n>0)
    {
      pxReadyAperiodicTasksLists->tasks[0].taskCode(pxReadyAperiodicTasksLists->tasks[0].params);
      pxReadyAperiodicTasksLists->n--;
      if(pxReadyAperiodicTasksLists->n==0){
        pxReadyAperiodicTasksLists->tasks=NULL;
      }else{
        for(int i = 0; i < pxReadyAperiodicTasksLists->n; i++) {
          pxReadyAperiodicTasksLists->tasks[i] = pxReadyAperiodicTasksLists->tasks[i+1];
        }
        pxReadyAperiodicTasksLists->tasks = (taskInfo *) realloc(pxReadyAperiodicTasksLists->tasks, pxReadyAperiodicTasksLists->n*sizeof(taskInfo));
      }
    }
    xTimerStop(xTimerServer,0);
    serverOn=false;
   Serial.println("Servers task Done");
    vTaskSuspend(NULL);
  }
}

void ServerActivation(void *pvParameters){
  TickType_t xLastWakeTime;
  taskInfo* task = (taskInfo *) pvParameters;
  const TickType_t xFrequency = task->period*1000;
  xLastWakeTime = xTaskGetTickCount();
  for(;;){
    //Serial.print("Check for aperiodic...");
    //Serial.printf(" Number of tasks: %d\n",pxReadyAperiodicTasksLists->n);
    if(pxReadyAperiodicTasksLists->n>0){
    xTimerChangePeriod(xTimerServer,task->computation*1000,0);
    xTimerStart(xTimerServer,0);
    serverOn=true;
    vTaskResume(serverHandler);
    }
    vTaskDelayUntil( &xLastWakeTime, xFrequency );
  }
}

void Periodic(void *pvParameters){
  TickType_t xLastWakeTime;
  taskInfo * task = (taskInfo *) pvParameters;
  const TickType_t xFrequency = task->period*1000;
  xLastWakeTime = xTaskGetTickCount();
  for( ;; )
    {
      bool resumeServer = false;
      int timeLeft=0;
       //Serial.printf(task->taskName);
       //Serial.println(" running...");
      if(serverOn && !serverPaused){
       timeLeft=xTimerGetExpiryTime(xTimerServer)-xTaskGetTickCount();
        xTimerStop(xTimerServer,0);
        resumeServer=true;
        serverPaused=true;
        //Serial.println("Stop Timer");
      }
       task->taskCode(task->params);
      if(resumeServer){
        //Serial.println("Start Timer");
        xTimerChangePeriod(xTimerServer,timeLeft,0);
        serverPaused=false;
      }
       //Serial.printf("\n... finish");
       //Serial.println(task->taskName);
      vTaskDelayUntil( &xLastWakeTime, xFrequency );
    }
}

void xTaskCreateAperiodic(pdTASK_CODE pvTaskCode, void *pvParameters,unsigned long computation) {
  pxReadyAperiodicTasksLists->n++;
  pxReadyAperiodicTasksLists->tasks = (taskInfo*) realloc(pxReadyAperiodicTasksLists->tasks,(pxReadyAperiodicTasksLists->n)*sizeof(taskInfo));
	pxReadyAperiodicTasksLists->tasks[pxReadyAperiodicTasksLists->n-1].computation = computation;
	pxReadyAperiodicTasksLists->tasks[pxReadyAperiodicTasksLists->n-1].params = pvParameters;
	pxReadyAperiodicTasksLists->tasks[pxReadyAperiodicTasksLists->n-1].taskCode = pvTaskCode;
	pxReadyAperiodicTasksLists->tasks[pxReadyAperiodicTasksLists->n-1].period = 0;
}

semaphoreInfo* createSemaphore(taskInfo* tasks,uint32_t id){
  int p=0;
  for (int i=0; i<sizeT; i++) {
    if(p!=0) break;
    // Serial.printf("Testando  tarefa ");
    // Serial.printf(tasks[i].taskName);
    // Serial.println("");
    for (int j = 0; j < maxResources; j++)
    {
      // Serial.println(tasks[i].resourcesId[j]);
      if(tasks[i].resourcesId[j]==id){
        p = tasks[i].priority;
        break;
      }
    }
  }
  semaphoreInfo *sI = (semaphoreInfo *) malloc(sizeof(semaphoreInfo)) ;
  sI->cellingP=p;
  sI->holder=NULL;
  sI->queue=NULL;
  sI->n=0;
  sI->originalP=0;
  // Serial.printf("\nCelling semaphore %d\n",p);
  return sI;
}

void startScheduler(){
  int prio = uxTaskPriorityGet(xTaskGetCurrentTaskHandle());
  vTaskPrioritySet(xTaskGetCurrentTaskHandle(),configMAX_PRIORITIES-1);
  // Serial.printf("\nResume task \n");
  for(int i=0;i<sizeT;i++){
    // Serial.println(i);
    vTaskResume(tasksHandler[i]);
  }
  //Serial.println("start");

  vTaskPrioritySet(xTaskGetCurrentTaskHandle(),prio);
}


void pauseScheduler(){
  int prio = uxTaskPriorityGet(xTaskGetCurrentTaskHandle());
  vTaskPrioritySet(xTaskGetCurrentTaskHandle(),configMAX_PRIORITIES-1);
  for(int i=0;i<sizeT;i++){
    vTaskSuspend(tasksHandler[i]);
  }
  vTaskPrioritySet(xTaskGetCurrentTaskHandle(),prio);
}