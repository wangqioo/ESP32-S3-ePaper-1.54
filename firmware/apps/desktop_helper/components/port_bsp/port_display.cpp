#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "port_display.h"
#include "epaper_config.h"

#define TAG "display"
#define BUFFER_SIZE (EPD_WIDTH * EPD_HEIGHT / 8)

static spi_device_handle_t spi;
static uint8_t *buffer = NULL;

const uint8_t WF_Full_1IN54[159] =
{											
    0x80,0x48,0x40,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x40,0x48,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x80,0x48,0x40,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x40,0x48,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xA,
    0x0,0x0,0x0,0x0,0x0,0x0,0x8,0x1,0x0,0x8,0x1,0x0,0x2,				
    0xA,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x22,0x22,0x22,0x22,0x22,0x22,0x0,
    0x0,0x0,0x22,0x17,0x41,0x0,0x32,0x20
};

unsigned char WF_PARTIAL_1IN54[159] =
{
    0x0,0x40,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x80,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x40,0x40,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0xF,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x1,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x22,0x22,0x22,0x22,0x22,0x22,0x0,0x0,0x0,0x02,
    0x17,0x41,0xB0,0x32,0x28,
};

void set_cs_1(){gpio_set_level(EPD_CS_PIN,1);}
void set_cs_0(){gpio_set_level(EPD_CS_PIN,0);}
void set_dc_1(){gpio_set_level(EPD_DC_PIN,1);}
void set_dc_0(){gpio_set_level(EPD_DC_PIN,0);}
void set_rst_1(){gpio_set_level(EPD_RST_PIN,1);}
void set_rst_0(){gpio_set_level(EPD_RST_PIN,0);}

void read_busy() {
    while(gpio_get_level(EPD_BUSY_PIN) == 1) {
        vTaskDelay(pdMS_TO_TICKS(5));   //LOW: idle, HIGH: busy
    }
}

void PortDisplay_Init() {
    buffer = (uint8_t *)heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "Initialize SPI");
    esp_err_t ret;
  	spi_bus_config_t buscfg = {};
  	buscfg.miso_io_num = -1;
  	buscfg.mosi_io_num = EPD_MOSI_PIN;
  	buscfg.sclk_io_num = EPD_SCK_PIN;
  	buscfg.quadwp_io_num = -1;
  	buscfg.quadhd_io_num = -1;
  	buscfg.max_transfer_sz = EPD_WIDTH * EPD_HEIGHT;

  	spi_device_interface_config_t devcfg = {};
  	devcfg.spics_io_num = -1;
  	devcfg.clock_speed_hz = 40 * 1000 * 1000;  //Clock out at 10 MHz
  	devcfg.mode = 0;                           //SPI mode 0
  	devcfg.queue_size = 7;                     //We want to be able to queue 7 transactions at a time
  	ret = spi_bus_initialize((spi_host_device_t)EPD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO);
  	ESP_ERROR_CHECK(ret);
  	ret = spi_bus_add_device((spi_host_device_t)EPD_SPI_NUM, &devcfg, &spi);
  	ESP_ERROR_CHECK(ret);

    gpio_config_t gpio_conf = {};
	gpio_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_conf.mode = GPIO_MODE_OUTPUT;
	gpio_conf.pin_bit_mask = (0x1ULL<<EPD_RST_PIN) | (0x1ULL<<EPD_DC_PIN) | (0x1ULL<<EPD_CS_PIN);
	gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));

	gpio_conf.mode = GPIO_MODE_INPUT;
	gpio_conf.pin_bit_mask = (0x1ULL<<EPD_BUSY_PIN);
	ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));

    set_rst_1();
}

void SPI_SendByte(uint8_t data) {
    esp_err_t ret;
  	spi_transaction_t t; 
  	memset(&t, 0, sizeof(t));
  	t.length = 8;      
  	t.tx_buffer = &data;
  	ret = spi_device_polling_transmit(spi, &t); //Transmit!
  	assert(ret == ESP_OK);                      //Should have had no issues.
}

void EPD_SendData(uint8_t data) {
    set_dc_1();
  	set_cs_0();
  	SPI_SendByte(data);
  	set_cs_1();
}

void EPD_SendCommand(uint8_t command) {
    set_dc_0();
  	set_cs_0();
  	SPI_SendByte(command);
  	set_cs_1();
}

void writeBytes(uint8_t *buffer,int len) {
    set_dc_1();
  	set_cs_0();
  	esp_err_t ret;
  	spi_transaction_t t; 
  	memset(&t, 0, sizeof(t));
  	t.length = 8 * len;      
  	t.tx_buffer = buffer;
  	ret = spi_device_polling_transmit(spi, &t); //Transmit!
  	assert(ret == ESP_OK);
  	set_cs_1();
}

void writeBytes(const uint8_t *buffer, int len) {
    set_dc_1();
  	set_cs_0();
  	esp_err_t ret;
  	spi_transaction_t t; 
  	memset(&t, 0, sizeof(t));
  	t.length = 8 * len;      
  	t.tx_buffer = buffer;
  	ret = spi_device_polling_transmit(spi, &t); //Transmit!
  	assert(ret == ESP_OK);
  	set_cs_1();
}

void EPD_SetWindows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
{
    EPD_SendCommand(0x44);  // SET_RAM_X_ADDRESS_START_END_POSITION
    EPD_SendData((Xstart>>3) & 0xFF);
    EPD_SendData((Xend>>3) & 0xFF);
	
    EPD_SendCommand(0x45);  // SET_RAM_Y_ADDRESS_START_END_POSITION
    EPD_SendData(Ystart & 0xFF);
    EPD_SendData((Ystart >> 8) & 0xFF);
    EPD_SendData(Yend & 0xFF);
    EPD_SendData((Yend >> 8) & 0xFF);
}

void EPD_SetCursor(uint16_t Xstart, uint16_t Ystart)
{
    EPD_SendCommand(0x4E); // SET_RAM_X_ADDRESS_COUNTER
    EPD_SendData(Xstart & 0xFF);

    EPD_SendCommand(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
    EPD_SendData(Ystart & 0xFF);
    EPD_SendData((Ystart >> 8) & 0xFF);
}

void EPD_SetLut(const uint8_t *lut) {
	EPD_SendCommand(0x32);
    writeBytes(lut,153);
	read_busy();
	
    EPD_SendCommand(0x3f);
    EPD_SendData(lut[153]);
	
    EPD_SendCommand(0x03);
    EPD_SendData(lut[154]);
	
    EPD_SendCommand(0x04);
    EPD_SendData(lut[155]);
	EPD_SendData(lut[156]);
	EPD_SendData(lut[157]);

	EPD_SendCommand(0x2c);
    EPD_SendData(lut[158]);
}

void EPD_TurnOnDisplay() {
    EPD_SendCommand(0x22);
    EPD_SendData(0xc7);
	EPD_SendCommand(0x20);
    read_busy();
}

void EPD_TurnOnDisplayPart() {
    EPD_SendCommand(0x22);
    EPD_SendData(0xcf);
    EPD_SendCommand(0x20);
    read_busy();
}

void EPD_Init() {
    set_rst_1();
  	vTaskDelay(pdMS_TO_TICKS(50));
  	set_rst_0();
  	vTaskDelay(pdMS_TO_TICKS(20));
  	set_rst_1();
  	vTaskDelay(pdMS_TO_TICKS(50));

    read_busy();
    EPD_SendCommand(0x12);  //SWRESET
    read_busy();

    EPD_SendCommand(0x01); //Driver output control
    EPD_SendData(0xC7);
    EPD_SendData(0x00);
    EPD_SendData(0x01);

    EPD_SendCommand(0x11); //data entry mode
    EPD_SendData(0x01);

	EPD_SetWindows(0, EPD_WIDTH-1, EPD_HEIGHT-1, 0);

    EPD_SendCommand(0x3C); //BorderWavefrom
    EPD_SendData(0x01);

    EPD_SendCommand(0x18);
    EPD_SendData(0x80);

    EPD_SendCommand(0x22); //Load Temperature and waveform setting.
    EPD_SendData(0XB1);
    EPD_SendCommand(0x20);

    EPD_SetCursor(0, EPD_HEIGHT-1);
	read_busy();
	
	EPD_SetLut(WF_Full_1IN54);
}

void EPD_Clear() {
    memset(buffer,0xff,BUFFER_SIZE);
}

void EPD_Display() {
    EPD_SendCommand(0x24);
    assert(buffer);
    writeBytes(buffer,BUFFER_SIZE);
    EPD_TurnOnDisplay();
}

void EPD_DisplayPartBaseImage() {
    EPD_SendCommand(0x24);
    assert(buffer);
    writeBytes(buffer,BUFFER_SIZE);
    EPD_SendCommand(0x26);
    writeBytes(buffer,BUFFER_SIZE);
    EPD_TurnOnDisplay();
}

void EPD_Init_Partial() {
    set_rst_1();
  	vTaskDelay(pdMS_TO_TICKS(50));
  	set_rst_0();
  	vTaskDelay(pdMS_TO_TICKS(20));
  	set_rst_1();
  	vTaskDelay(pdMS_TO_TICKS(50));

	read_busy();
	
	EPD_SetLut(WF_PARTIAL_1IN54);

    EPD_SendCommand(0x37); 
    EPD_SendData(0x00);  
    EPD_SendData(0x00);  
    EPD_SendData(0x00);  
    EPD_SendData(0x00); 
    EPD_SendData(0x00);  	
    EPD_SendData(0x40);  
    EPD_SendData(0x00);  
    EPD_SendData(0x00);   
    EPD_SendData(0x00);  
    EPD_SendData(0x00);
	
    EPD_SendCommand(0x3C); //BorderWavefrom
    EPD_SendData(0x80);
	
	EPD_SendCommand(0x22); 
	EPD_SendData(0xc0); 
	EPD_SendCommand(0x20); 
	read_busy();
}

void EPD_DisplayPart() {
    EPD_SendCommand(0x24);
    assert(buffer);
    writeBytes(buffer,5000);
    EPD_TurnOnDisplayPart();
}

void EPD_DrawColorPixel(uint16_t x, uint16_t y,uint8_t color) {
    if (x >= EPD_WIDTH || y >= EPD_HEIGHT)
    {
        ESP_LOGE("EPD", "Out of bounds pixel: (%d,%d)", x, y);
        return; 
    }

    uint16_t index = y * 25 + (x >> 3); //25是200/8
    uint8_t bit = 7 - (x & 0x07);
    if(color == DRIVER_COLOR_WHITE)
    {
        buffer[index] |= (0x01 << bit);
    }
    else
    {
        buffer[index] &= ~(0x01 << bit);
    }
}


void PortLvgl_Start_Init() {
    PortDisplay_Init();
    EPD_Init();
    EPD_Clear();
    EPD_DisplayPartBaseImage();
    EPD_Init_Partial();            //局部刷新初始化
}