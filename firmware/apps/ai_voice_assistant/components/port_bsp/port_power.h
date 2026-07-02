#ifndef PORT_POWER_H
#define PORT_POWER_H

#ifdef __cplusplus
extern "C" {
#endif


void BoardPower_Init();
void BoardPower_EPD_ON();
void BoardPower_EPD_OFF();
void BoardPower_Audio_ON();
void BoardPower_Audio_OFF();
void BoardPower_VBAT_ON();
void BoardPower_VBAT_OFF();


#ifdef __cplusplus
}
#endif


#endif