#include "esp_event.h"
#include "esp_log.h"
#include "./include/util.h"
#include "../queue/include/queue.h"

static const char *TAG = "Util";
static uint16_t frame_seq = 0x01;

int setstamp64(int64_t sec, int64_t usec)
{
  struct timeval tv;
  struct timezone tz = {
      0, 0};
  tv.tv_sec = sec;
  tv.tv_usec = usec;

  settimeofday(&tv, &tz);

  return 0;
}

int64_t getstamp64()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t intTime = ((int64_t)tv.tv_sec) * 1000LL + ((int64_t)tv.tv_usec) / 1000LL;

  return intTime;
}

int encodeSensorN(Msg_t *msg, char *data, int64_t timestamp)
{
  int i = 0;
  msg->buf[i++] = CMD_NOTIFICATION;
  msg->buf[i++] = TARGET_ULTRA_SONIC;
  msg->buf[i++] = data[0];
  msg->buf[i++] = data[1];
  msg->buf[i++] = data[2];
  msg->buf[i++] = data[3];
  msg->buf[i++] = 0xff & (timestamp >> 56);
  msg->buf[i++] = 0xff & (timestamp >> 48);
  msg->buf[i++] = 0xff & (timestamp >> 40);
  msg->buf[i++] = 0xff & (timestamp >> 32);
  msg->buf[i++] = 0xff & (timestamp >> 24);
  msg->buf[i++] = 0xff & (timestamp >> 16);
  msg->buf[i++] = 0xff & (timestamp >> 8);
  msg->buf[i++] = 0xff & (timestamp >> 0);
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