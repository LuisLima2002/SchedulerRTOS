#include <Scheduler.h>

semaphoreInfo* testeSi;

typedef struct 
{
  char* name;
  int computation;
} paramTeste;

TickType_t lastTicket;
void task(void *pvParameters){
  //Serial.println(lastTicket);
  paramTeste *params = (paramTeste * ) pvParameters;
  int x=0,j=0;
  // TickType_t timePassed=0;
  TickType_t timeToRun=((params->computation/3)*1000);
  while (timeToRun>0)
  {
    timeToRun-= xTaskGetTickCount()-lastTicket;
    lastTicket = xTaskGetTickCount();
    //Serial.println(timePassed);
    Serial.println(params->name);
    //Serial.printf(" %d ",timeToRun);
  }
  //Serial.printf("\n");
  //Serial.printf(params->name);
  //Serial.println(" Entrando da RC");
  up(testeSi,xTaskGetCurrentTaskHandle());
  //Serial.println(lastTicket);
  timeToRun=((params->computation*2/3)*1000);
  char print[10];
  while (timeToRun>0)
  {
    timeToRun-= xTaskGetTickCount()-lastTicket;
    lastTicket = xTaskGetTickCount();
    strcpy(print,params->name);
    strcat(print,"-RC");  
    Serial.println(print);
  }
  //Serial.printf(params->name);
  //Serial.println(" Saindo da RC");
  //Serial.println(lastTicket);
  down(testeSi);
}

void aperiodic(void *pvParameters){
 TickType_t timeToRun=5700;
  while (timeToRun>0)
  {
    timeToRun-= xTaskGetTickCount()-lastTicket;
    lastTicket = xTaskGetTickCount();
    Serial.println("Server");
  }
}

bool armed=true;

void handleInterruption(){
    if(armed){
    //Serial.println("interrupt");
    xTaskCreateAperiodic(aperiodic,NULL,1);
    armed=false;
    }
}

void turnOnInt(){
  Serial.println("TURN ON ");
  armed=true;
}

void setup(){
  Serial.begin(115200);
  /* Create taskSET */
  taskInfo *tasks= (taskInfo *)malloc(3*sizeof(taskInfo));
  tasks[0].taskCode= task;
  tasks[0].taskName= "Task 1";
  tasks[0].computation= 3;
  tasks[0].period= 12;
  paramTeste* param1= (paramTeste *) malloc(sizeof(paramTeste));
  param1->computation=3;
  param1->name ="T1";
  tasks[0].params= (void * ) param1;
  tasks[0].resourcesId[0] = 0;
  tasks[0].resourcesId[1] = 2;
 
  tasks[1].taskCode= task;  
  tasks[1].taskName= "Task 2";
  tasks[1].computation= 6;
  tasks[1].period= 15;
  paramTeste* param2= (paramTeste *) malloc(sizeof(paramTeste));
  param2->computation=6;
  param2->name ="T2";
  tasks[1].params= (void * ) param2;
  tasks[1].resourcesId[0]= 2;

  tasks[2].taskCode= task;
  tasks[2].taskName= "Task 3";
  tasks[2].computation= 6;
  tasks[2].period= 21;
  paramTeste* param3= (paramTeste *) malloc(sizeof(paramTeste));
  param3->computation=6;
  param3->name ="T3";
  tasks[2].params= (void * ) param3;

  Scheduler(&tasks,3,3,20);
  testeSi = (createSemaphore(tasks,2)); 
  
  attachInterrupt(digitalPinToInterrupt(15),handleInterruption,HIGH);
  attachInterrupt(digitalPinToInterrupt(14),turnOnInt,HIGH);
  startScheduler();
}
void loop(){
  lastTicket = xTaskGetTickCount();
  Serial.println("idle");
  delay(10);
}