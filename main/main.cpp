#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include "nvs_flash.h"

#include <cstring>

static const char* TAG = "TriacLights"; 

static EventGroupHandle_t wifiEventGroup;
#define WIFI_EVENT_CONN_BIT BIT0

static void wifiEventHandler(void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData) {
	if (eventBase == WIFI_EVENT) {
		switch(eventID) {
			case WIFI_EVENT_STA_START:
				esp_wifi_connect();
				break;
			case WIFI_EVENT_STA_DISCONNECTED:
				ESP_LOGW(TAG, "WiFi disconnected! Retrying...");
				xEventGroupClearBits(wifiEventGroup, WIFI_EVENT_CONN_BIT);
				esp_wifi_connect();
				break;
			default:
				break;
		}
	} else if (eventBase == IP_EVENT) {
		switch(eventID) {
			ip_event_got_ip_t* ipData;

			case IP_EVENT_STA_GOT_IP:
				ipData = (ip_event_got_ip_t*) eventData;
				ESP_LOGI(TAG, "Successfully connected, got IP: %s", ip4addr_ntoa(&ipData->ip_info.ip));
				xEventGroupSetBits(wifiEventGroup, WIFI_EVENT_CONN_BIT);
				break;
			default:
				break; 
		}
	}
}

extern "C" void app_main() {
	ESP_LOGV(TAG, "Setting up NVS...");

	esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

	ESP_LOGV(TAG, "Setting up events...");

	wifiEventGroup = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, NULL));

	ESP_LOGV(TAG, "Initializing WiFi...");

	tcpip_adapter_init();
	
	wifi_init_config_t wifiInitConfig = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifiInitConfig));

	wifi_config_t wifiConfig = { 0 };
	memcpy(wifiConfig.sta.ssid, CONFIG_WIFI_SSID, sizeof(CONFIG_WIFI_SSID));
	memcpy(wifiConfig.sta.password, CONFIG_WIFI_PASSWORD, sizeof(CONFIG_WIFI_PASSWORD));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "WiFi configured for SSID '%s'", wifiConfig.sta.ssid);

	ESP_LOGI(TAG, "Successfully initialized WiFi!");
}