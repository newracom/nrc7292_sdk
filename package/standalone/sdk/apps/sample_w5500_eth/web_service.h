#ifndef __WEB_SERVICE_H__
#define __WEB_SERVICE_H__

#include <esp_http_server.h>

httpd_handle_t run_http_server(WIFI_CONFIG* wifi_config);
#endif /* __WEB_SERVICE_H__ */
