#include "esphome/core/util.h"
#include "esphome/core/defines.h"
#include "esphome/core/application.h"
#include "esphome/core/version.h"
#include "esphome/core/log.h"

#ifdef USE_WIFI
#include "esphome/components/wifi/wifi_component.h"
#endif

#ifdef USE_API
#include "esphome/components/api/api_server.h"
#endif

#ifdef USE_ETHERNET
#include "esphome/components/ethernet/ethernet_component.h"
#endif

#ifdef USE_MQTT
#include "esphome/components/mqtt/mqtt_client.h"
#endif

#ifdef USE_MDNS
#ifdef ARDUINO_ARCH_ESP32
#include <ESPmDNS.h>
#endif
#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266mDNS.h>
#endif
#endif

namespace esphome {

bool network_is_connected() {
#ifdef USE_ETHERNET
  if (ethernet::global_eth_component != nullptr && ethernet::global_eth_component->is_connected())
    return true;
#endif

#ifdef USE_WIFI
  if (wifi::global_wifi_component != nullptr)
    return wifi::global_wifi_component->is_connected();
#endif

  return false;
}

bool api_is_connected() {
#ifdef USE_API
  if (api::global_api_server != nullptr) {
    return api::global_api_server->is_connected();
  }
#endif
  return false;
}

bool mqtt_is_connected() {
#ifdef USE_MQTT
  if (mqtt::global_mqtt_client != nullptr) {
    return mqtt::global_mqtt_client->is_connected();
  }
#endif
  return false;
}

bool remote_is_connected() { return api_is_connected() || mqtt_is_connected(); }

#if defined(ARDUINO_ARCH_ESP8266) && defined(USE_MDNS)
static bool mdns_setup;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
#endif

#ifndef WEBSERVER_PORT
static const uint8_t WEBSERVER_PORT = 80;
#endif

#ifdef USE_MDNS
#ifdef ARDUINO_ARCH_ESP8266
void network_setup_mdns(const IPAddress &address, int interface) {
  // Latest arduino framework breaks mDNS for AP interface
  // see https://github.com/esp8266/Arduino/issues/6114
  if (interface == 1)
    return;
  MDNS.begin(App.get_name().c_str(), address);
  mdns_setup = true;
#endif
#ifdef ARDUINO_ARCH_ESP32
  void network_setup_mdns() {
    MDNS.begin(App.get_name().c_str());
#endif
#ifdef USE_API
    if (api::global_api_server != nullptr) {
      MDNS.addService("esphomelib", "tcp", api::global_api_server->get_port());
      // DNS-SD (!=mDNS !) requires at least one TXT record for service discovery - let's add version
      MDNS.addServiceTxt("esphomelib", "tcp", "version", ESPHOME_VERSION);
      MDNS.addServiceTxt("esphomelib", "tcp", "address", network_get_address().c_str());
      MDNS.addServiceTxt("esphomelib", "tcp", "mac", get_mac_address().c_str());
#ifdef ARDUINO_ARCH_ESP8266
      MDNS.addServiceTxt("esphomelib", "tcp", "platform", "ESP8266");
#endif
#ifdef ARDUINO_ARCH_ESP32
      MDNS.addServiceTxt("esphomelib", "tcp", "platform", "ESP32");
#endif
      MDNS.addServiceTxt("esphomelib", "tcp", "board", ESPHOME_BOARD);
#ifdef ESPHOME_PROJECT_NAME
      MDNS.addServiceTxt("esphomelib", "tcp", "project_name", ESPHOME_PROJECT_NAME);
      MDNS.addServiceTxt("esphomelib", "tcp", "project_version", ESPHOME_PROJECT_VERSION);
#endif
    } else {
#endif
      // Publish "http" service if not using native API nor the webserver component
      // This is just to have *some* mDNS service so that .local resolution works
      MDNS.addService("http", "tcp", WEBSERVER_PORT);
      MDNS.addServiceTxt("http", "tcp", "version", ESPHOME_VERSION);
#ifdef USE_API
    }
#endif
#ifdef USE_PROMETHEUS
    MDNS.addService("prometheus-http", "tcp", WEBSERVER_PORT);
#endif
  }
#endif

  void network_tick_mdns() {
#if defined(ARDUINO_ARCH_ESP8266) && defined(USE_MDNS)
    if (mdns_setup)
      MDNS.update();
#endif
  }

  std::string network_get_address() {
#ifdef USE_ETHERNET
    if (ethernet::global_eth_component != nullptr)
      return ethernet::global_eth_component->get_use_address();
#endif
#ifdef USE_WIFI
    if (wifi::global_wifi_component != nullptr)
      return wifi::global_wifi_component->get_use_address();
#endif
    return "";
  }

}  // namespace esphome
