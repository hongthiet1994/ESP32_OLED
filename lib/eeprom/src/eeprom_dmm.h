#include <stdio.h>
#include <string.h>
#include <stdint.h>



#define EEPROM_DEBUG                 "EEPROM_DEBUG"
#define FIELD_RESTART_COUNTER        "RESTART_COUNTER"

#define FIELD_SSID_STA               "FIELD_SSID_STA"
#define FIELD_PASSWORD_STA           "FIELD_PASS_STA"

#define FIELD_SSID_AP                "FIELD_SSID_AP"
#define FIELD_PASSWORD_AP            "FIELD_PASS_AP"


#define FIELD_PASSWORD_SYSTEM        "F_PASS_SYSTEM"

#define FIELD_NUM_REV                "FIELD_REV"
#define FIELD_NUM_SEND               "FIELD_SEND"  



int32_t nvs_get_number_dmm(const char* key);
void nvs_set_number_dmm(const char *key, int32_t value);
void count_restart();
esp_err_t nvs_get_str_dmm(const char* key, char* out_value, size_t* length);
esp_err_t nvs_set_str_dmm(const char* key, char* in_value);
void test_nvs();