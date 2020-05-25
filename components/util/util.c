#include "esp_event.h"
#include "esp_log.h"
#include "./include/util.h"
#include "../queue/include/queue.h"
#include <string.h>

static const char *TAG = "Util";
static uint16_t frame_seq = 0x01;

static int LOGEN = 1;

int64_t stamp64FromBuffer(char *buf, int len)
{
  char ch[2];
  uint8_t ch8;
  int i, j;
  int64_t temp, tm = 0;
  for (i = 0; i < len; i++)
  {
    ch[0] = buf[i];
    ch[1] = 0;
    ch8 = atoi((char *)&ch[0]);
    temp = 1ll;
    for (j = 1; j < len - i; j++)
    {
      temp *= 10;
    }
    tm += ch8 * temp;
  }
  return tm;
}
int stamp64ToBuffer(int64_t tmin, char *buf)
{
  int index = 0;
  int i, j;
  uint64_t temp, tm;
  char ch[2];
  uint8_t ch8;

  tm = tmin;

  for (i = 16; i >= 0; i--)
  {
    temp = 1;
    for (j = 0; j < i; j++)
    {
      temp *= 10;
    }
    ch8 = tm / temp;
    tm = tm - ch8 * temp;
    itoa(ch8, &ch[0], 10);
    // printf("%d ch8:%d ch[0]:%c\n", i, ch8, ch[0]);

    if (index == 0 && ch8 != 0)
    {
      buf[index++] = ch[0];
    }
    else if (index > 0)
    {
      buf[index++] = ch[0];
    }
  }

  buf[index] = 0;
  return 0;
}

int setstamp64(int64_t sec, int64_t usec)
{
  struct timeval tv;
  // struct timezone tz = {
  //     0, 0};
  tv.tv_sec = sec;
  tv.tv_usec = usec;

  settimeofday(&tv, NULL);

  return 0;
}

int64_t getstamp64()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t intTime = ((int64_t)tv.tv_sec) * 1000LL + ((int64_t)tv.tv_usec) / 1000LL;
  // int64_t temp = 0LL;

  if (LOGEN == 1)
  {
    printf("getstamp64()\n");
    printf("%lld\n", intTime);
    printf("\n");
  }

  return intTime;
}
int encodeUTReset(Msg_t *msg, uint8_t err, int64_t timestamp)
{
  int i = 0;
  char buffer[24];

  msg->buf[i++] = CMD_RESPONSE;
  msg->buf[i++] = TARGET_ULTRA_SONIC;
  msg->buf[i++] = err;
  msg->buf[i++] = UT_RESET;

  stamp64ToBuffer(timestamp, buffer);

  for (int j = 0; j < strlen(buffer); j++)
  {
    msg->buf[i++] = buffer[j];
  }

  msg->len = i;

  return 0;
}

int encodeSensorN(Msg_t *msg, char *data, int64_t timestamp)
{
  int i = 0;
  char buffer[24];

  msg->buf[i++] = CMD_NOTIFICATION;
  msg->buf[i++] = TARGET_ULTRA_SONIC;
  msg->buf[i++] = data[0];
  msg->buf[i++] = data[1];
  msg->buf[i++] = data[2];
  msg->buf[i++] = data[3];

  // timestamp
  stamp64ToBuffer(timestamp, buffer);

  for (int j = 0; j < strlen(buffer); j++)
  {
    msg->buf[i++] = buffer[j];
  }

  msg->len = i;

  return 0;
}

/**
 * Update CRC16 for input byte
 * 
 * From: https://github.com/meegoo-tsui/stm32/blob/master/demo/iap_uart/src/ymodem.c
 * 
 */
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
  uint32_t crc = crcIn;
  uint32_t in = byte | 0x100;
  do
  {
    crc <<= 1;
    in <<= 1;
    if (in & 0x100)
      ++crc;
    if (crc & 0x10000)
      crc ^= 0x1021;
  } while (!(in & 0x10000));
  return crc & 0xffffu;
}
/**
  * @brief  Cal CRC16 for YModem Packet
  * @param  data
  * @param  length
   * @retval None
  */
static uint16_t calCRC16(const uint8_t *data, uint32_t size)
{
  uint32_t crc = 0;
  const uint8_t *dataEnd = data + size;
  while (data < dataEnd)
    crc = UpdateCRC16(crc, *data++);

  crc = UpdateCRC16(crc, 0);
  crc = UpdateCRC16(crc, 0);
  return crc & 0xffffu;
}

int check_crc(char *buf)
{
  // check len
  int len = (buf[3] << 8) | buf[4];

  if (len < 2 || len > 24)
  {
    ESP_LOGI(TAG, "Rx invalid frame, len=%d", len);
    return -1;
  }

  return 0;
}
/**
 * 
 * return frame length
 */
int create_frame(char *buf, Msg_t msg)
{
  int index = 0;
  uint16_t seq = frame_seq++;
  uint16_t len = msg.len + 2;
  buf[index++] = 0x01;

  buf[index++] = 0xff & (seq >> 8);
  buf[index++] = 0xff & seq;
  buf[index++] = (len >> 8) & 0xff;
  buf[index++] = len & 0xff;
  for (int i = 0; i < (len - 2); i++)
  {
    buf[index++] = msg.buf[i];
  }
  buf[index++] = 0xff;
  buf[index++] = 0xff;
  return index;
}