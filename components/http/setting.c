#include <stdio.h>

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include "nvs_flash.h"
#include <sys/param.h>
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_eth.h"
#include "protocol_examples_common.h"

#include <esp_http_server.h>

#include "./include/http.h"

static const char *TAG = "HTTP setting";

esp_err_t
setting_get_handler(httpd_req_t *req)
{
  char *buf;
  size_t buf_len;
  char param[32];
  char param1[32];
  char param2[32];

  /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
  buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
  if (buf_len > 1)
  {
    buf = malloc(buf_len);
    /* Copy null terminated value string into buffer */
    if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK)
    {
      ESP_LOGI(TAG, "Found header => Host: %s", buf);
    }
    free(buf);
  }

  buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
  if (buf_len > 1)
  {
    buf = malloc(buf_len);
    if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK)
    {
      ESP_LOGI(TAG, "Found header => Test-Header-2: %s", buf);
    }
    free(buf);
  }

  buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
  if (buf_len > 1)
  {
    buf = malloc(buf_len);
    if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK)
    {
      ESP_LOGI(TAG, "Found header => Test-Header-1: %s", buf);
    }
    free(buf);
  }

  /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1)
  {
    buf = malloc(buf_len);
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
    {
      ESP_LOGI(TAG, "Found URL query => %s", buf);

      /* Get value of expected key from query string */
      if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK)
      {
        ESP_LOGI(TAG, "Found URL query parameter => query1=%s", param);
      }
      if (httpd_query_key_value(buf, "query2", param1, sizeof(param1)) == ESP_OK)
      {
        ESP_LOGI(TAG, "Found URL query parameter => query2=%s", param1);
      }
      if (httpd_query_key_value(buf, "query3", param2, sizeof(param2)) == ESP_OK)
      {
        ESP_LOGI(TAG, "Found URL query parameter => query3=%s", param2);
      }
    }
    free(buf);
  }

  /* Set some custom headers */
  httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
  httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

  /* Send response with custom headers and body set as the
     * string passed in user context*/
  // const char *resp_str = (const char *)req->user_ctx;
  char *resp_str = malloc(1024);
  if (strcmp(param, "cmd") == 0)
  {
    ESP_LOGI(TAG, "Receive cmd");
    sprintf(resp_str, "<!DOCTYPE html><html><head><meta charset=\"UTF-8\" /><title>ESP32配置</title><script type=\"text/javascript\">    (function (window, document, undefined) {      window.onload = init;      function init() {      }    })(window, document, undefined);</script><style>    body {      text-align: center;      font-size: medium;      color: #2ca089;    }    footer {      font-size: small;      color: #2ca089;      text-align: center;    }    a {      color: aquamarine;    }    .form-item {      margin-bottom: 10px;    }</style></head><body><h3>配置结果</h3><section><p>%s</p><p>SSID:</p><p>PASS:</p></section><section><p><a href=\"/\">返回</a></p></section></body></html>", "OK");
    httpd_resp_send(req, resp_str, strlen(resp_str));
  }
  else
  {
    sprintf(resp_str, "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"/><title>ESP32配置</title><script type=\"text/javascript\">    (function (window, document, undefined) {      window.onload = init;    function init() { } })(window, document, undefined);</script><style>    body {      text-align: center;  font-size: medium;    color: #2ca089;  }    footer { font-size: small;  color: #2ca089;      text-align: center;    }  a {      color: aquamarine;  }  .form-item {   margin-bottom: 10px;  }</style></head><body><h3>配置界面</h3><section><form action=\"/setting\" method=\"get\"><div class=\"form-item\"><label>CMD</label><input type=\"text\" name=\"query1\" id=\"query1\" value=\"cmd\" /></div><div class=\"form-item\"><label>SSID</label><input type=\"text\" name=\"query2\" id=\"query2\" /></div><div class=\"form-item\"><label>PASS</label><input type=\"text\" name=\"query3\" id=\"query3\" /></div><input type=\"submit\" value=\"提交\" style=\" width : 100px;font-size: x-large\"></form></section><section><p><a href=\"/\">返回</a></p></section></body></html>");
    httpd_resp_send(req, resp_str, strlen(resp_str));
  }
  free(resp_str);
  /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
  if (httpd_req_get_hdr_value_len(req, "Host") == 0)
  {
    ESP_LOGI(TAG, "Request headers lost");
  }
  return ESP_OK;
}

httpd_uri_t setting = {
    .uri = "/setting",
    .method = HTTP_GET,
    .handler = setting_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = "Page Setting"};
