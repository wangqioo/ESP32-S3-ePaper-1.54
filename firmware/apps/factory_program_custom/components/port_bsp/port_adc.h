#ifndef PORT_ADC_H
#define PORT_ADC_H

#ifdef __cplusplus
extern "C" {
#endif

void BoardAdc_Init();
float Get_VbatVoltage();
uint8_t Get_Batterylevel();

#ifdef __cplusplus
}
#endif

#endif
