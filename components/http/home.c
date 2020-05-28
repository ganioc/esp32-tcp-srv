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

static const char *TAG = "HTTP home";

/* An HTTP GET handler */
static esp_err_t home_get_handler(httpd_req_t *req)
{
  char *buf;
  size_t buf_len;

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
      char param[32];
      /* Get value of expected key from query string */
      if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK)
      {
        ESP_LOGI(TAG, "Found URL query parameter => query1=%s", param);
      }
      if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK)
      {
        ESP_LOGI(TAG, "Found URL query parameter => query3=%s", param);
      }
      if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK)
      {
        ESP_LOGI(TAG, "Found URL query parameter => query2=%s", param);
      }
    }
    free(buf);
  }

  /* Set some custom headers */
  httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
  httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

  /* Send response with custom headers and body set as the
     * string passed in user context*/
  const char *resp_str = (const char *)req->user_ctx;
  httpd_resp_send(req, resp_str, strlen(resp_str));

  /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
  if (httpd_req_get_hdr_value_len(req, "Host") == 0)
  {
    ESP_LOGI(TAG, "Request headers lost");
  }
  return ESP_OK;
}

static const char strHome[] = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"/><title>ESP32主页</title><style>body{text-align:center;font-size:medium;} .pic {display: block;      padding-top: 10%%;    }    footer {      font-size: small;      color: #2ca089;      text-align: center;    }    a {      color: aquamarine;    }</style></head><body><section><div class=\"pic\"><svg xmlns:dc=\" http://purl.org/dc/elements/1.1/\" xmlns:cc=\"http://creativecommons.org/ns#\"        xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns:svg=\"http://www.w3.org/2000/svg\"        xmlns=\"http://www.w3.org/2000/svg\" id=\"svg8\" version=\"1.1\" viewBox=\"0 0 38.510086 52.722115\"        height=\"52.722115mm\" width=\"38.510086mm\"><defs id=\"defs2\" /><metadata id=\"metadata5\"><rdf:RDF><cc:Work rdf:about=\"\"><dc:format>image/svg+xml</dc:format><dc:type rdf:resource=\"http://purl.org/dc/dcmitype/StillImage\" /><dc:title></dc:title></cc:Work></rdf:RDF></metadata><g transform=\"translate(-76.601284,-139.59744)\" id=\"layer1\"><path            d=\"m 94.456245,158.88751 c -0.705779,-4.04169 4.056681,-3.45475 5.523094,-0.96447 2.116561,3.59438 -0.518888,7.94693 -4.071329,9.27809 -5.065012,1.89794 -10.493363,-1.51436 -12.012251,-6.46949 -1.910233,-6.23183 2.260419,-12.68882 8.375222,-14.35817 7.253278,-1.98017 14.653789,2.88311 16.460219,10.02128 2.07007,8.17999 -3.4304,16.45364 -11.499093,18.3889 -9.037444,2.16761 -18.127617,-3.92574 -20.185519,-12.85598 -2.267785,-9.84102 4.38259,-19.70141 14.120295,-21.87689 10.600957,-2.36833 21.192937,4.80942 23.481647,15.31066 2.46815,11.32453 -5.21198,22.61523 -16.440062,25.0134 -1.796943,0.3838 -3.64181,0.5389 -5.477642,0.46268\"            id=\"path815\"            style=\"fill:#5fd3bc;fill-opacity:1;fill-rule:evenodd;stroke:#000000;stroke-width:0;stroke-miterlimit:4;stroke-dasharray:none\" /><text id=\"text819\" y=\"192.75418\" x=\"82.814575\"            style=\"font-style:normal;font-weight:normal;font-size:10.58333302px;line-height:1.25;font-family:sans-serif;letter-spacing:0px;word-spacing:0px;fill:#2ca089;fill-opacity:1;stroke:none;stroke-width:0.26458332\"            xml:space=\"preserve\"><tspan              style=\"font-style:normal;font-variant:normal;font-weight:normal;font-stretch:normal;font-size:9.87777805px;line-height:4.25;font-family:'Songti TC';-inkscape-font-specification:'Songti TC';fill:#2ca089;stroke-width:0.26458332\"              y=\"192.75418\" x=\"82.814575\" id=\"tspan817\">ESP32</tspan></text><flowRoot transform=\"scale(0.26458333)\"            style=\"font-style:normal;font-weight:normal;font-size:40px;line-height:1.25;font-family:sans-serif;letter-spacing:0px;word-spacing:0px;fill:#000000;fill-opacity:1;stroke:none\"            id=\"flowRoot821\" xml:space=\"preserve\"><flowRegion id=\"flowRegion823\"><rect y=\"710.51971\" x=\"497\" height=\"44\" width=\"154\" id=\"rect825\" /></flowRegion><flowPara id=\"flowPara827\">ESP32</flowPara><flowPara id=\"flowPara829\" /></flowRoot></g></svg></div></section><section><div><p><a href=\"/info\">读取</a></p><p><a href=\"/setting\">设置</a></p></div></section></body><footer><p>2020 Ruff Team</p></footer></html>";

httpd_uri_t home = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = home_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = (void *)strHome};
