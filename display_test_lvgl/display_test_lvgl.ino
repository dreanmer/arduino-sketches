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


#include "time.h"  // Para funções de tempo

// Labels para exibir o tempo
lv_obj_t * label_time;
lv_obj_t * label_date;
lv_obj_t * label_seconds;

// Timer para atualizar o relógio
lv_timer_t * clock_timer;

// Função para atualizar o relógio
void update_clock_cb(lv_timer_t * timer) {
    // Pega o tempo atual
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Buffer para strings
    static char time_str[16];
    static char date_str[32];
    static char sec_str[8];
    
    // Formatar hora:minuto
    strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);
    lv_label_set_text(label_time, time_str);
    
    // Formatar data
    strftime(date_str, sizeof(date_str), "%d/%m/%Y", &timeinfo);
    lv_label_set_text(label_date, date_str);
    
    // Segundos separados (para animação mais fluida)
    strftime(sec_str, sizeof(sec_str), "%S", &timeinfo);
    lv_label_set_text(label_seconds, sec_str);
}

void create_clock_ui() {
    // Configurar fundo escuro
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), 0);
    
    // === HORA PRINCIPAL (HH:MM) ===
    label_time = lv_label_create(lv_screen_active());
    lv_label_set_text(label_time, "00:00");
    
    // Estilo da hora principal
    static lv_style_t style_time;
    lv_style_init(&style_time);
    lv_style_set_text_color(&style_time, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_time, &lv_font_montserrat_48);  // Fonte grande
    lv_obj_add_style(label_time, &style_time, 0);
    
    // Centralizar hora
    lv_obj_align(label_time, LV_ALIGN_CENTER, 0, -30);
    
    // === SEGUNDOS ===
    label_seconds = lv_label_create(lv_screen_active());
    lv_label_set_text(label_seconds, "00");
    
    // Estilo dos segundos
    static lv_style_t style_seconds;
    lv_style_init(&style_seconds);
    lv_style_set_text_color(&style_seconds, lv_color_hex(0x00FF00));  // Verde
    lv_style_set_text_font(&style_seconds, &lv_font_montserrat_22);
    lv_obj_add_style(label_seconds, &style_seconds, 0);
    
    // Posicionar segundos ao lado da hora
    lv_obj_align_to(label_seconds, label_time, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    
    // === DATA ===
    label_date = lv_label_create(lv_screen_active());
    lv_label_set_text(label_date, "01/01/2024");
    
    // Estilo da data
    static lv_style_t style_date;
    lv_style_init(&style_date);
    lv_style_set_text_color(&style_date, lv_color_hex(0xAAAAAA));  // Cinza
    lv_style_set_text_font(&style_date, &lv_font_montserrat_18);
    lv_obj_add_style(label_date, &style_date, 0);
    
    // Posicionar data abaixo da hora
    lv_obj_align_to(label_date, label_time, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    
    // === CÍRCULO DECORATIVO ===
    lv_obj_t * circle = lv_obj_create(lv_screen_active());
    lv_obj_set_size(circle, 200, 200);
    lv_obj_set_style_radius(circle, 100, 0);
    lv_obj_set_style_border_width(circle, 2, 0);
    lv_obj_set_style_border_color(circle, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, 0);  // Transparente
    lv_obj_align(circle, LV_ALIGN_CENTER, 0, 0);
    
    // === TIMER PARA ATUALIZAÇÃO ===
    clock_timer = lv_timer_create(update_clock_cb, 1000, NULL);  // Atualiza a cada 1s
    
    // Primeira atualização imediata
    update_clock_cb(clock_timer);
}

// Configuração inicial do tempo (coloque no setup())
void setup_time() {
    // Configurar timezone (ajuste para sua região)
    configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");  // UTC-3 (Brasília)
    
    // Aguardar sincronização NTP (para ESP32 com WiFi)
    Serial.println("Aguardando sincronização de tempo...");
    while (time(nullptr) < 24 * 3600) {
        delay(100);
    }
    Serial.println("Tempo sincronizado!");
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

    create_clock_ui();

    Serial.println( "Setup done" );
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay(5); /* let this time pass */
}
