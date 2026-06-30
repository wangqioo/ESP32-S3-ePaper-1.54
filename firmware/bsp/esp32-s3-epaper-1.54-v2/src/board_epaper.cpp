#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <new>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_epaper.h"
#include "board_pins.h"
#include "board_power.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_heap_caps.h"

static const char *TAG = "board_epaper";

typedef enum {
    DRIVER_COLOR_WHITE  = 0xff,
    DRIVER_COLOR_BLACK  = 0x00,
    FONT_BACKGROUND = DRIVER_COLOR_WHITE,
} COLOR_IMAGE;

typedef struct {
    uint8_t cs;
    uint8_t dc;
    uint8_t rst;
    uint8_t busy;
    uint8_t mosi;
    uint8_t scl;
    int spi_host;
    int buffer_len;
} custom_lcd_spi_t;

class epaper_driver_display {
private:
    const custom_lcd_spi_t lcd_spi_data;
    const int Width;
    const int Height;
    spi_device_handle_t spi = NULL;
    uint8_t *buffer = NULL;
    esp_err_t init_status = ESP_OK;
    bool spi_bus_initialized = false;

    esp_err_t spi_gpio_init();
    esp_err_t spi_port_init();
    void read_busy();

    void set_cs_1(){gpio_set_level((gpio_num_t)lcd_spi_data.cs,1);}
    void set_cs_0(){gpio_set_level((gpio_num_t)lcd_spi_data.cs,0);}
    void set_dc_1(){gpio_set_level((gpio_num_t)lcd_spi_data.dc,1);}
    void set_dc_0(){gpio_set_level((gpio_num_t)lcd_spi_data.dc,0);}
    void set_rst_1(){gpio_set_level((gpio_num_t)lcd_spi_data.rst,1);}
    void set_rst_0(){gpio_set_level((gpio_num_t)lcd_spi_data.rst,0);}

    void SPI_SendByte(uint8_t data);
    void EPD_SendData(uint8_t data);
    void EPD_SendCommand(uint8_t command);
    void writeBytes(uint8_t *buffer,int len);
    void writeBytes(const uint8_t *buffer, int len);
    void EPD_SetWindows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend);
    void EPD_SetCursor(uint16_t Xstart, uint16_t Ystart);
    void EPD_SetLut(const uint8_t *lut);
    void EPD_TurnOnDisplay();
    void EPD_TurnOnDisplayPart();

public:
    epaper_driver_display(int width, int height,custom_lcd_spi_t _lcd_spi_data);
    ~epaper_driver_display();

    esp_err_t get_init_status() const { return init_status; }

    void EPD_Init();    /* 墨水屏初始化 */
    void EPD_Clear();   /* 清空屏幕 */
    void EPD_Display(); /* 刷buffer到墨水屏 */

    /*局部刷新*/
    void EPD_DisplayPartBaseImage();
    void EPD_Init_Partial();
    void EPD_DisplayPart();
    void EPD_DrawColorPixel(uint16_t x, uint16_t y,uint8_t color);
};

const uint8_t WF_Full_1IN54[159] =
{
    0x80,	0x48,	0x40,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0x40,	0x48,	0x80,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0x80,	0x48,	0x40,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0x40,	0x48,	0x80,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0xA,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0x8,	0x1,	0x0,	0x8,	0x1,	0x0,	0x2,
    0xA,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
    0x22,	0x22,	0x22,	0x22,	0x22,	0x22,	0x0,	0x0,	0x0,
    0x22,	0x17,	0x41,	0x0,	0x32,	0x20
};

unsigned char WF_PARTIAL_1IN54_0[159] =
{
    0x0,0x40,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x80,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x40,0x40,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0xF,0x0,0x0,0x0,0x0,0x0,0x0,
    0x1,0x1,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x22,0x22,0x22,0x22,0x22,0x22,0x0,0x0,0x0,
    0x02,0x17,0x41,0xB0,0x32,0x28,
};

epaper_driver_display::epaper_driver_display(int width, int height,custom_lcd_spi_t _lcd_spi_data) :
    lcd_spi_data(_lcd_spi_data),
    Width(width),
    Height(height) {

    ESP_LOGI(TAG, "Initialize SPI");
    init_status = spi_port_init();
    if (init_status != ESP_OK) {
        return;
    }

    init_status = spi_gpio_init();
    if (init_status != ESP_OK) {
        return;
    }

    buffer = (uint8_t *)heap_caps_malloc(lcd_spi_data.buffer_len, MALLOC_CAP_SPIRAM);
    if (buffer == NULL) {
        init_status = ESP_ERR_NO_MEM;
    }
}

epaper_driver_display::~epaper_driver_display() {
    if (buffer != NULL) {
        heap_caps_free(buffer);
    }
    if (spi != NULL) {
        spi_bus_remove_device(spi);
    }
    if (spi_bus_initialized) {
        spi_bus_free((spi_host_device_t)lcd_spi_data.spi_host);
    }
}

esp_err_t epaper_driver_display::spi_gpio_init() {
    int rst = lcd_spi_data.rst;
    int cs = lcd_spi_data.cs;
    int dc = lcd_spi_data.dc;
    int busy = lcd_spi_data.busy;

    gpio_config_t gpio_conf = {};
	gpio_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_conf.mode = GPIO_MODE_OUTPUT;
	gpio_conf.pin_bit_mask = (0x1ULL<<rst) | (0x1ULL<<dc) | (0x1ULL<<cs);
	gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	ESP_RETURN_ON_ERROR(gpio_config(&gpio_conf), TAG, "configure epaper output pins failed");

	gpio_conf.mode = GPIO_MODE_INPUT;
	gpio_conf.pin_bit_mask = (0x1ULL<<busy);
	ESP_RETURN_ON_ERROR(gpio_config(&gpio_conf), TAG, "configure epaper busy pin failed");

    set_rst_1();
    return ESP_OK;
}

esp_err_t epaper_driver_display::spi_port_init() {
    int mosi = lcd_spi_data.mosi;
    int scl = lcd_spi_data.scl;
    int spi_host = lcd_spi_data.spi_host;
    esp_err_t ret;
    spi_bus_config_t buscfg = {};
    buscfg.miso_io_num = -1;
    buscfg.mosi_io_num = mosi;
    buscfg.sclk_io_num = scl;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = Width * Height;

    spi_device_interface_config_t devcfg = {};
    devcfg.spics_io_num = -1;
    devcfg.clock_speed_hz = 40 * 1000 * 1000;  //Clock out at 10 MHz
    devcfg.mode = 0;                           //SPI mode 0
    devcfg.queue_size = 7;                     //We want to be able to queue 7 transactions at a time

    ret = spi_bus_initialize((spi_host_device_t)spi_host, &buscfg, SPI_DMA_CH_AUTO);
    ESP_RETURN_ON_ERROR(ret, TAG, "initialize epaper spi bus failed");
    spi_bus_initialized = true;
    ret = spi_bus_add_device((spi_host_device_t)spi_host, &devcfg, &spi);
    ESP_RETURN_ON_ERROR(ret, TAG, "add epaper spi device failed");
    return ESP_OK;
}

void epaper_driver_display::read_busy() {
    int busy = lcd_spi_data.busy;
    while(gpio_get_level((gpio_num_t)busy) == 1)
	{
        vTaskDelay(pdMS_TO_TICKS(5));   //LOW: idle, HIGH: busy
    }
}

void epaper_driver_display::SPI_SendByte(uint8_t data) {
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &data;
    ret = spi_device_polling_transmit(spi, &t); //Transmit!
    assert(ret == ESP_OK);                      //Should have had no issues.
}

void epaper_driver_display::EPD_SendData(uint8_t data) {
    set_dc_1();
    set_cs_0();
    SPI_SendByte(data);
    set_cs_1();
}

void epaper_driver_display::EPD_SendCommand(uint8_t command) {
    set_dc_0();
    set_cs_0();
    SPI_SendByte(command);
    set_cs_1();
}

void epaper_driver_display::writeBytes(uint8_t *buffer,int len) {
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

void epaper_driver_display::writeBytes(const uint8_t *buffer, int len) {
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

void epaper_driver_display::EPD_SetWindows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
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

void epaper_driver_display::EPD_SetCursor(uint16_t Xstart, uint16_t Ystart)
{
    EPD_SendCommand(0x4E); // SET_RAM_X_ADDRESS_COUNTER
    EPD_SendData(Xstart & 0xFF);

    EPD_SendCommand(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
    EPD_SendData(Ystart & 0xFF);
    EPD_SendData((Ystart >> 8) & 0xFF);
}

void epaper_driver_display::EPD_SetLut(const uint8_t *lut) {
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

void epaper_driver_display::EPD_TurnOnDisplay() {
    EPD_SendCommand(0x22);
    EPD_SendData(0xc7);
	EPD_SendCommand(0x20);
    read_busy();
}

void epaper_driver_display::EPD_TurnOnDisplayPart() {
    EPD_SendCommand(0x22);
    EPD_SendData(0xcf);
    EPD_SendCommand(0x20);
    read_busy();
}

void epaper_driver_display::EPD_Init() {
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

	EPD_SetWindows(0, Width-1, Height-1, 0);

    EPD_SendCommand(0x3C); //BorderWavefrom
    EPD_SendData(0x01);

    EPD_SendCommand(0x18);
    EPD_SendData(0x80);

    EPD_SendCommand(0x22); //Load Temperature and waveform setting.
    EPD_SendData(0XB1);
    EPD_SendCommand(0x20);

    EPD_SetCursor(0, Height-1);
	read_busy();

	EPD_SetLut(WF_Full_1IN54);
}

void epaper_driver_display::EPD_Clear() {
    int buffer_len = lcd_spi_data.buffer_len;
    memset(buffer,0xff,buffer_len);
}

void epaper_driver_display::EPD_Display() {
    int buffer_len = lcd_spi_data.buffer_len;
    EPD_SendCommand(0x24);
    assert(buffer);
    writeBytes(buffer,buffer_len);
    EPD_TurnOnDisplay();
}

void epaper_driver_display::EPD_DisplayPartBaseImage() {
    int buffer_len = lcd_spi_data.buffer_len;
    EPD_SendCommand(0x24);
    assert(buffer);
    writeBytes(buffer,buffer_len);
    EPD_SendCommand(0x26);
    writeBytes(buffer,buffer_len);
    EPD_TurnOnDisplay();
}

void epaper_driver_display::EPD_Init_Partial() {
    set_rst_1();
    vTaskDelay(pdMS_TO_TICKS(50));
    set_rst_0();
    vTaskDelay(pdMS_TO_TICKS(20));
    set_rst_1();
    vTaskDelay(pdMS_TO_TICKS(50));

	read_busy();

	EPD_SetLut(WF_PARTIAL_1IN54_0);

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

void epaper_driver_display::EPD_DisplayPart() {
    EPD_SendCommand(0x24);
    assert(buffer);
    writeBytes(buffer,5000);
    EPD_TurnOnDisplayPart();
}

void epaper_driver_display::EPD_DrawColorPixel(uint16_t x, uint16_t y,uint8_t color) {
    if (x >= Width || y >= Height)
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

static epaper_driver_display *s_driver = NULL;

static custom_lcd_spi_t board_epaper_config()
{
    custom_lcd_spi_t config = {};
    config.cs = BOARD_EPD_CS_PIN;
    config.dc = BOARD_EPD_DC_PIN;
    config.rst = BOARD_EPD_RST_PIN;
    config.busy = BOARD_EPD_BUSY_PIN;
    config.mosi = BOARD_EPD_MOSI_PIN;
    config.scl = BOARD_EPD_SCK_PIN;
    config.spi_host = BOARD_EPD_SPI_HOST;
    config.buffer_len = (BOARD_EPD_WIDTH * BOARD_EPD_HEIGHT) / 8;
    return config;
}

esp_err_t board_epaper_init(void)
{
    if (s_driver != NULL) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(board_power_epaper_on(), TAG, "epaper power on failed");

    epaper_driver_display *driver = new (std::nothrow) epaper_driver_display(
        BOARD_EPD_WIDTH,
        BOARD_EPD_HEIGHT,
        board_epaper_config());
    if (driver == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = driver->get_init_status();
    if (ret != ESP_OK) {
        delete driver;
        return ret;
    }

    driver->EPD_Init();
    driver->EPD_Clear();
    driver->EPD_DisplayPartBaseImage();
    driver->EPD_Init_Partial();
    s_driver = driver;
    return ESP_OK;
}

void board_epaper_clear(void)
{
    if (s_driver != NULL) {
        s_driver->EPD_Clear();
    }
}

void board_epaper_draw_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    if (s_driver != NULL) {
        s_driver->EPD_DrawColorPixel(x, y, color);
    }
}

void board_epaper_flush_full(void)
{
    if (s_driver != NULL) {
        s_driver->EPD_Display();
    }
}

void board_epaper_flush_partial(void)
{
    if (s_driver != NULL) {
        s_driver->EPD_DisplayPart();
    }
}

void *board_epaper_get_driver(void)
{
    return s_driver;
}
