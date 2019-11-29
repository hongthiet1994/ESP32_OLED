#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "nvs.h"
#include "esp_log.h"
#include "eeprom_dmm.h"

esp_err_t err;
nvs_handle my_handle;
int32_t restart_counter = 0; // value will default to 0, if not set yet in NVS

int32_t nvs_get_number_dmm(const char *key)
{
    int32_t int32_value = 0;
    size_t *len = malloc(sizeof(size_t));
    //ESP_LOGI(EEPROM_DEBUG, "Opening Non-Volatile Storage (NVS) handle... ");
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(EEPROM_DEBUG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    else
    {
        //ESP_LOGI(EEPROM_DEBUG, "Geting string from NVS ... ");
        // Read
        err = nvs_get_i32(my_handle, key, &int32_value);
        switch (err)
        {
        case ESP_OK:
            ESP_LOGI(EEPROM_DEBUG, "value  = %d ", int32_value);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(EEPROM_DEBUG, "The value is not initialized yet!");
            err = nvs_set_i32(my_handle, key, int32_value);
            switch (err)
            {
            case ESP_OK:
                ESP_LOGI(EEPROM_DEBUG, " WRITE DONE ");
                break;
            default:
                ESP_LOGI(EEPROM_DEBUG, "  WRITE ERROR");
            }
            // Commit written value.
            // After setting any values, nvs_commit() must be called to ensure changes are written
            // to flash storage. Implementations may write to storage at other times,
            // but this is not guaranteed.
            //ESP_LOGI(EEPROM_DEBUG,"Committing updates in NVS ... ");
            err = nvs_commit(my_handle);
            break;
        default:
            ESP_LOGI(EEPROM_DEBUG, "Error (%s) reading!", esp_err_to_name(err));
        }
        // Close
        nvs_close(my_handle);
    }
    return int32_value;
}
void nvs_set_number_dmm(const char *key, int32_t value)
{
    // Open
    //ESP_LOGI(EEPROM_DEBUG, "Opening Non-Volatile Storage (NVS) handle... ");
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(EEPROM_DEBUG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    else
    {
        // Write
        //ESP_LOGI(EEPROM_DEBUG,"Updating restart counter in NVS ... ");
        err = nvs_set_i32(my_handle, key, value);
        switch (err)
        {
            case ESP_OK:
                ESP_LOGI(EEPROM_DEBUG, " WRITE DONE ");
                break;
            default:
                ESP_LOGI(EEPROM_DEBUG, "  WRITE ERROR");
        }
        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        //ESP_LOGI(EEPROM_DEBUG,"Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        switch (err)
        {
        case ESP_OK:
            ESP_LOGI(EEPROM_DEBUG, " COMMIT DONE ");
            break;
        default:
            ESP_LOGI(EEPROM_DEBUG, " COMMIT ERROR");
        }
        // Close
        nvs_close(my_handle);
    }
}

void count_restart()
{
    // Open
    //ESP_LOGI(EEPROM_DEBUG, "Opening Non-Volatile Storage (NVS) handle... ");
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(EEPROM_DEBUG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    else
    {
        //printf("Done\n");
        //ESP_LOGI(EEPROM_DEBUG, "Reading restart counter from NVS ... ");
        // Read

        err = nvs_get_i32(my_handle, FIELD_RESTART_COUNTER, &restart_counter);
        switch (err)
        {
        case ESP_OK:
            ESP_LOGI(EEPROM_DEBUG, "Restart counter = %d", restart_counter);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(EEPROM_DEBUG, "The value is not initialized yet!");
            break;
        default:
            ESP_LOGI(EEPROM_DEBUG, "Error (%s) reading!", esp_err_to_name(err));
        }
        // Write
        //ESP_LOGI(EEPROM_DEBUG,"Updating restart counter in NVS ... ");
        restart_counter++;
        err = nvs_set_i32(my_handle, FIELD_RESTART_COUNTER, restart_counter);
        switch (err)
        {
        case ESP_OK:
            ESP_LOGI(EEPROM_DEBUG, " WRITE DONE ");
            break;
        default:
            ESP_LOGI(EEPROM_DEBUG, "  WRITE ERROR");
        }
        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        //ESP_LOGI(EEPROM_DEBUG,"Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        switch (err)
        {
        case ESP_OK:
            ESP_LOGI(EEPROM_DEBUG, " COMMIT DONE ");
            break;
        default:
            ESP_LOGI(EEPROM_DEBUG, " COMMIT ERROR");
        }
        // Close
        nvs_close(my_handle);
    }
}

esp_err_t nvs_get_str_dmm(const char *key, char *out_value, size_t *length)
{
    size_t *len = malloc(sizeof(size_t));
    //ESP_LOGI(EEPROM_DEBUG, "Opening Non-Volatile Storage (NVS) handle... ");
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(EEPROM_DEBUG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    else
    {
        //ESP_LOGI(EEPROM_DEBUG, "Geting string from NVS ... ");
        // Read
        err = nvs_get_str(my_handle, key, out_value, len);
        switch (err)
        {
        case ESP_OK:
            ESP_LOGI(EEPROM_DEBUG, "String  = %s ", out_value);
            *length = *len - 1; // without \0
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(EEPROM_DEBUG, "The value is not initialized yet!");
            break;
        default:
            ESP_LOGI(EEPROM_DEBUG, "Error (%s) reading!", esp_err_to_name(err));
        }
        // Close
        nvs_close(my_handle);
    }
    return err;
}

esp_err_t nvs_set_str_dmm(const char *key, char *in_value)
{
    // Open
    //ESP_LOGI(EEPROM_DEBUG, "Opening Non-Volatile Storage (NVS) handle... ");
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(EEPROM_DEBUG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    else
    {
        // Write
        //ESP_LOGI(EEPROM_DEBUG, "Setting string from NVS ... ");
        err = nvs_set_str(my_handle, key, in_value);
        switch (err)
        {
        case ESP_OK:
            ESP_LOGI(EEPROM_DEBUG, " WRITE DONE");
            break;
        default:
            ESP_LOGI(EEPROM_DEBUG, " WRITE ERROR (%s) ", esp_err_to_name(err));
        }
        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        //ESP_LOGI(EEPROM_DEBUG,"Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        switch (err)
        {
        case ESP_OK:
            ESP_LOGI(EEPROM_DEBUG, " COMMIT DONE ");
            break;
        default:
            ESP_LOGI(EEPROM_DEBUG, " COMMIT ERROR");
        }
        // Close
        nvs_close(my_handle);
    }
    return err;
}
