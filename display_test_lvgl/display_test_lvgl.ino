#include <lvgl.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

// Pinos para o XIAO ESP32C6
#define TFT_DC    1
#define TFT_CS    2
#define TFT_RST   3

/*Set to your screen resolution and rotation*/
#define TFT_HOR_RES   240
#define TFT_VER_RES   240
#define TFT_ROTATION  LV_DISPLAY_ROTATION_0

#define DEBUG    1

// Instância do display
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

static lv_color_t draw_buf[TFT_HOR_RES * TFT_VER_RES / 10];

#if LV_USE_LOG != 0
void my_print( lv_log_level_t level, const char * buf )
{
    LV_UNUSED(level);
    Serial.println(buf);
    Serial.flush();
}
#endif

void gfx_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.writePixels((uint16_t*)px_map, w * h);
    tft.endWrite();

    lv_display_flush_ready(disp);
}

/*use Arduinos millis() as tick source*/
static uint32_t my_tick(void)
{
    return millis();
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Setup start");

    tft.begin();

    #if DEBUG != 0
        tft.setRotation(0);
        tft.fillScreen(GC9A01A_BLACK);
        tft.setTextColor(GC9A01A_WHITE);
        tft.setTextSize(2);
        tft.setCursor(20, 100);
        tft.println("GC9A01A OK");
        tft.drawCircle(120, 120, 60, GC9A01A_RED);

        delay(1000);
    #endif

    /* initializes lvgl */
    Serial.println("LVGL " + String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch());

    lv_init();
    lv_tick_set_cb(my_tick);

    /* register print function for debugging */
    #if LV_USE_LOG != 0
        lv_log_register_print_cb(my_print);
    #endif

    /* setup lvgl to work with display driver */
    lv_display_t * disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, gfx_disp_flush);
    lv_display_set_rotation(disp, TFT_ROTATION);


    #if DEBUG != 0
        lv_obj_t *label = lv_label_create(lv_screen_active());
        lv_label_set_text(label, "Hello Arduino, I'm LVGL!");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        delay(1000);
    #endif

    Serial.println( "Setup done" );
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay(5); /* let this time pass */
}
