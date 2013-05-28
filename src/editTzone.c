#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

extern int tZone;
extern bool changed;

Window setZoneW;
TextLayer setZoneW_zone;
TextLayer setZoneW_label;
TextLayer setZoneW_disclaim;

char gmt[7];

char* itoa2(int valIN, int base){ // 2 in the morning hack
	static char buf2[32] = {0};
	int i = 30;
	int val = abs(valIN);

	for(; val && i ; --i, val /= base)
		buf2[i] = "0123456789abcdef"[val % base];
	if(valIN<0)
		buf2[i] = '-';
	else if(valIN>0)
		buf2[i] = '+';
	if(valIN == 0)
		return &buf2[i+1];
	return &buf2[i];
	
}

void zone_up(ClickRecognizerRef recognizer, Window *window) {
	(void)recognizer;
	(void)window;

	if(tZone<24)
		tZone++;
	strcpy(gmt+3, itoa2(tZone,10));
	text_layer_set_text(&setZoneW_zone, gmt);
	changed = true;
}

void zone_down(ClickRecognizerRef recognizer, Window *window) {
	(void)recognizer;
	(void)window;

	if(tZone>-24)
		tZone--;
	strcpy(gmt+3, itoa2(tZone,10));
	text_layer_set_text(&setZoneW_zone, gmt);
	changed = true;
}

void zone_click_config_provider(ClickConfig **config, Window *window) {
	(void)window;

	config[BUTTON_ID_UP]->click.handler = (ClickHandler) zone_up;
	config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;
		
	config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) zone_down;
	config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;
}

void showEditTimeZone()
{
	window_init(&setZoneW, "Set Time Zone");
	window_set_background_color(&setZoneW, GColorBlack);

	strcpy(gmt, "UTC");
	strcpy(gmt+3, itoa2(tZone,10));

	text_layer_init(&setZoneW_zone, GRect(0,50,144,48));
	text_layer_set_text(&setZoneW_zone, gmt);
	text_layer_set_font(&setZoneW_zone, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_alignment(&setZoneW_zone, GTextAlignmentCenter);
	setZoneW_zone.text_color = GColorWhite;
	setZoneW_zone.background_color = GColorBlack;
	layer_add_child(&setZoneW.layer, &setZoneW_zone.layer);
	
	text_layer_init(&setZoneW_label, GRect(0,5,144,48));
	text_layer_set_text(&setZoneW_label, "Change Time Zone");
	text_layer_set_font(&setZoneW_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(&setZoneW_label, GTextAlignmentCenter);
	setZoneW_label.text_color = GColorWhite;
	setZoneW_label.background_color = GColorBlack;
	layer_add_child(&setZoneW.layer, &setZoneW_label.layer);

	text_layer_init(&setZoneW_disclaim, GRect(0,168-31,144,30));
	text_layer_set_text(&setZoneW_disclaim, "Not Persistant");
	text_layer_set_font(&setZoneW_disclaim, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
	text_layer_set_text_alignment(&setZoneW_disclaim, GTextAlignmentCenter);
	setZoneW_disclaim.text_color = GColorWhite;
	setZoneW_disclaim.background_color = GColorBlack;
	layer_add_child(&setZoneW.layer, &setZoneW_disclaim.layer);

	window_set_click_config_provider(&setZoneW, (ClickConfigProvider) zone_click_config_provider);
	window_stack_push(&setZoneW, true);

	changed = true;
}
