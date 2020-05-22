
#include <time.h>
#include <sys/time.h>
#include "./include/encode.h"

int64_t getstamp()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t intTime = ((int64_t)tv.tv_sec) * 1000LL + ((int64_t)tv.tv_usec) / 1000LL;

  return intTime;
}

int encodeSensorNotification(Msg_t *msg, char *data, int64_t timestamp)
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