#include <string.h>
#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "tcpip_adapter.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "freertos/queue.h"

#include "include/server.h"
#include "../queue/include/queue.h"

static const char *TAG = "TCP server";

static void socket_task(void *pvParameters)
{
  char rx_buffer[256];
  int sock = *((int *)pvParameters);
  int len = 0;
  QueueHandle_t rx_queue; // from tcp to uart_task
  QueueHandle_t tx_queue;
  rx_queue = create_queue();
  tx_queue = create_queue();

  add_queue(sock, rx_queue, tx_queue);

  ESP_LOGI(TAG, "Connection opened");

  while (1)
  {
    int s;
    fd_set rfds;
    struct timeval tv = {
        .tv_sec = 0,
        .tv_usec = 200000,
    };
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);

    // receive from
    s = select(sock + 1, &rfds, NULL, NULL, &tv);
    // len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    // Error occurred during receiving
    if (s < 0)
    {
      ESP_LOGE(TAG, "recv failed: errno %d", errno);
      break;
    }
    // Connection closed
    else if (s == 0)
    {
      ESP_LOGI(TAG, "recv timeout");
    }
    // Data received
    else
    {
      // Get the sender's ip address as string
      // if (source_addr.sin6_family == PF_INET)
      // {
      //   inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
      // }
      // else if (source_addr.sin6_family == PF_INET6)
      // {
      //   inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
      // }

      // rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
      if (FD_ISSET(sock, &rfds))
      {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0)
        {
          ESP_LOGE(TAG, "recv failed: errno %d", errno);
          break;
        }
        else if (len == 0)
        {
          ESP_LOGE(TAG, "recv failed: 0 len ");
          break;
        }
        else
        {
          ESP_LOGI(TAG, "Received %d bytes from :", len);
          // ESP_LOGI(TAG, "%s", rx_buffer);
          rx_buffer[len] = 0;
          int err = send(sock, rx_buffer, len, 0);
          if (err < 0)
          {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            break;
          }
        }
      }
    }
  }
  // Dont need to close sock, when len == 0?
  // socket is closed?
  remove_queue(sock);

  // if (len != 0)
  // {
  ESP_LOGE(TAG, "Shutting down conn...");
  shutdown(sock, 0);
  close(sock);
  // }

  vTaskDelete(NULL);
}

static void tcp_server_task(void *pvParameters)
{
  char addr_str[128];
  int addr_family;
  int ip_protocol;

  while (1)
  {
    // #ifdef CONFIG_EXAMPLE_IPV4
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0)
    {
      ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
      break;
    }
    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
      ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
      break;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", SERVER_PORT);

    err = listen(listen_sock, 2);
    if (err != 0)
    {
      ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
      break;
    }
    ESP_LOGI(TAG, "Socket listening");

    struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
    uint addr_len = sizeof(source_addr);
    int sock;

    while (1)
    {
      sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
      if (sock < 0)
      {
        ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
        break;
      }
      ESP_LOGI(TAG, "Socket accepted");
      // To start a new task here
      xTaskCreate(socket_task, "conn", 4096, &sock, 6, NULL);
    }

    // To start a new task here

    if (sock != -1)
    {
      ESP_LOGE(TAG, "Shutting down socket and restarting...");
      shutdown(sock, 0);
      close(sock);
    }
  }
  vTaskDelete(NULL);
}

void init_server()
{
  xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
}
