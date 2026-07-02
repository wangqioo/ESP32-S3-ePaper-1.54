#include <stdio.h>
#include "port_adc.h"
#include <esp_adc/adc_oneshot.h>

adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t cali_handle;

void BoardAdc_Init() {
    adc_cali_curve_fitting_config_t cali_config = {};
    cali_config.unit_id = ADC_UNIT_1;
    cali_config.atten = ADC_ATTEN_DB_12;
    cali_config.bitwidth = ADC_BITWIDTH_12; //4096
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle));
    
    adc_oneshot_unit_init_cfg_t init_config1 = {};
    init_config1.unit_id = ADC_UNIT_1;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    adc_oneshot_chan_cfg_t config = {};
    config.bitwidth = ADC_BITWIDTH_12;
    config.atten = ADC_ATTEN_DB_12;
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config));

}

float Get_VbatVoltage() {
    int OriginalData;
    int CalibratedData;
    float VbatVoltage;
    adc_oneshot_read(adc1_handle,ADC_CHANNEL_3,&OriginalData);
    adc_cali_raw_to_voltage(cali_handle,OriginalData,&CalibratedData);
    VbatVoltage = 0.001 * CalibratedData * 2;
    return VbatVoltage;
}

uint8_t Get_Batterylevel() {
    const float VOLT_FULL = 4.12f;  
    const float VOLT_EMPTY = 3.0f;  
    
    float vol = Get_VbatVoltage();
    
    if (vol <= VOLT_EMPTY) return 0;
    if (vol >= VOLT_FULL) return 100;

    return (uint8_t)((vol - VOLT_EMPTY) / (VOLT_FULL - VOLT_EMPTY) * 100.0f);
}