#ifndef FT6336_BSP_H
#define FT6336_BSP_H

#include <driver/i2c_master.h>

class I2cFt6336Dev
{
private:
    i2c_master_dev_handle_t i2c_master_dev_;
    int x_max_;
    int y_max_;
    int reset_init_flag = 0;

    I2cFt6336Dev(i2c_master_bus_handle_t i2c_master_bus,int i2c_dev_addr,int x_max,int y_max);
    ~I2cFt6336Dev();

public:
    void Ft6336_Reset(int reset_pin);
    uint8_t GetTouchPoint(uint16_t *x,uint16_t *y);
    static I2cFt6336Dev *requestInstance(i2c_master_bus_handle_t i2c_master_bus,int i2c_dev_addr,int x_max,int y_max);
    static I2cFt6336Dev *instance_;
};

#endif
