#include <stdio.h>
#include "./include/queue.h"

QueueManager_t xManager[QUEUE_LENGTH];
SemaphoreHandle_t xSemaphore = NULL;
StaticSemaphore_t xSemaphoreBuffer;

static char *TAG = "queue.c";

void init_queue()
{
  xSemaphore = xSemaphoreCreateBinaryStatic(&xSemaphoreBuffer);
  xSemaphoreGive(xSemaphore);
}
QueueHandle_t create_queue()
{
  return xQueueCreate(MAX_MSG_NUM, sizeof(Msg_t));
}
int add_queue(int sock)
{
  ESP_LOGI(TAG, "add_queue");
  int i = 0;
  if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
  {
    ESP_LOGI(TAG, "xSemaphoreTake pdTRUE");
    for (i = 0; i < QUEUE_LENGTH; i++)
    {
      if (xManager[i].flag == 0)
      {
        xManager[i].sock_num = sock;
        xManager[i].rx_queue = create_queue();
        // xQueueCreate(MAX_MSG_NUM, sizeof(Msg_t));
        xManager[i].tx_queue = create_queue();
        xManager[i].flag = 1;
        printf("sock=%d\n", sock);
        break;
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
        printf("remove_queue %d\n", sock);
        break;
      }
    }
    xSemaphoreGive(xSemaphore);
    return 0;
  }
  return -1;
}

QueueHandle_t get_rx_queue(int sock)
{
  for (int i = 0; i < QUEUE_LENGTH; i++)
  {
    if (xManager[i].flag == 1 && xManager[i].sock_num == sock)
    {
      return xManager[i].rx_queue;
    }
  }
  return NULL;
}
QueueHandle_t get_tx_queue(int sock)
{
  for (int i = 0; i < QUEUE_LENGTH; i++)
  {
    if (xManager[i].flag == 1 && xManager[i].sock_num == sock)
    {
      return xManager[i].tx_queue;
    }
  }
  return NULL;
}
/**
 * It must ensure : rx, tx_queue exists
 * When uart_task is checking the packet;
 */