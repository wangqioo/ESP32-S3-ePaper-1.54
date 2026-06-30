#ifndef PORT_SHTC3_H
#define PORT_SHTC3_H

#include <driver/i2c_master.h>
#include "port_i2c.h"

typedef enum{
  NO_ERROR       = 0x00, // no error
  ACK_ERROR      = 0x01, // no acknowledgment error
  CHECKSUM_ERROR = 0x02 // checksum mismatch error
}etError;

typedef enum{
  READ_ID            = 0xEFC8, // command: read ID register
  SOFT_RESET         = 0x805D, // soft reset
  SLEEP              = 0xB098, // sleep
  WAKEUP             = 0x3517, // wakeup
  MEAS_T_RH_POLLING  = 0x7866, // meas. read T first, clock stretching disabled
  MEAS_T_RH_CLOCKSTR = 0x7CA2, // meas. read T first, clock stretching enabled
  MEAS_RH_T_POLLING  = 0x58E0, // meas. read RH first, clock stretching disabled
  MEAS_RH_T_CLOCKSTR = 0x5C24  // meas. read RH first, clock stretching enabled
}etCommands;


#ifdef __cplusplus
extern "C" {
#endif

void Shtc3_Init(I2cMasterBus *i2cmasterbus);
etError Shtc3_Wakeup();
etError Shtc3_Sleep();
etError Shtc3_SoftReset();
void Shtc3_ReadTempHumi(float * t,float * h);

#ifdef __cplusplus
}
#endif

#endif