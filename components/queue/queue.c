#include <stdio.h>
#include "./include/queue.h"

static QueueManager_t xManager[QUEUE_LENGTH];
static SemaphoreHandle_t xSemaphore = NULL;
StaticSemaphore_t xSemaphoreBuffer;

void init_queue()
{
  xSemaphore = xSemaphoreCreateBinaryStatic(&xSemaphoreBuffer);
}

int add_queue(int sock)
{
  int i = 0;
  if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
  {
    for (i = 0; i < QUEUE_LENGTH; i++)
    {
      if (xManager[i].flag == 0)
      {
        xManager[i].sock_num = sock;
        xManager[i].rx_queue = xQueueCreate(MAX_MSG_NUM, sizeof(Msg_t));
        xManager[i].tx_queue = xQueueCreate(MAX_MSG_NUM, sizeof(Msg_t));
        xManager[i].flag = 1;
      }
    }
    xSemaphoreGive(xSemaphore);
    return 0;
  }
  return -1;
}

int remove_queue(int sock)
{
  if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
  {
    for (int i = 0; i < QUEUE_LENGTH; i++)
    {
      if (xManager[i].flag == 1 && xManager[i].sock_num == sock)
      {
        xManager[i].sock_num = 0;
        vQueueDelete(xManager[i].rx_queue);
        vQueueDelete(xManager[i].tx_queue);
        xManager[i].flag = 0;
      }
    }
    xSemaphoreGive(xSemaphore);
    return 0;
  }
  return -1;
}