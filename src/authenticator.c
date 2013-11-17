#include <pebble.h>
#include "configuration.h"
#include "sha1.h"

// Truncate n decimal digits to 2^n for 6 digits
#define DIGITS_TRUNCATE 1000000
#define SHA1_SIZE 20

#define TZ_STORAGE_KEY 42

static Window *main_window;
static TextLayer *label;
static TextLayer *token;
static TextLayer *ticker;

static Window *tz_window;
static TextLayer *tz_zone;
static TextLayer *tz_label;

static char gmt[7];

static int curToken = 0;
static int tZone = 0;
static bool changed = true;

// return seconds since epoch compensating for Pebble's lack of location
// independent GMT

int curSeconds=0;

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

void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {

	static char tokenText[] = "RYRYRY"; // Needs to be static because it's used by the system later.

	sha1nfo s;
	uint8_t ofs;
	uint32_t otp;
	int i;
	char sha1_time[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	
	int curSeconds = tick_time->tm_sec;

	if(curSeconds == 0 || curSeconds == 30 || changed)
	{
		changed = false;

		// TOTP uses seconds since epoch in the upper half of an 8 byte payload
		// TOTP is HOTP with a time based payload
		// HOTP is HMAC with a truncation function to get a short decimal key
		uint32_t unix_time = time(NULL) + ((0-tZone)*3600);
		unix_time /= 30;

		sha1_time[4] = (unix_time >> 24) & 0xFF;
		sha1_time[5] = (unix_time >> 16) & 0xFF;
		sha1_time[6] = (unix_time >> 8) & 0xFF;
		sha1_time[7] = unix_time & 0xFF;

		// First get the HMAC hash of the time payload with the shared key
		sha1_initHmac(&s, otpkeys[curToken], otpsizes[curToken]);
		sha1_write(&s, sha1_time, 8);
		sha1_resultHmac(&s);
		
		// Then do the HOTP truncation.  HOTP pulls its result from a 31-bit byte
		// aligned window in the HMAC result, then lops off digits to the left
		// over 6 digits.
		ofs=s.state.b[SHA1_SIZE-1] & 0xf;
		otp = 0;
		otp = ((s.state.b[ofs] & 0x7f) << 24) |
			((s.state.b[ofs + 1] & 0xff) << 16) |
			((s.state.b[ofs + 2] & 0xff) << 8) |
			(s.state.b[ofs + 3] & 0xff);
		otp %= DIGITS_TRUNCATE;
		
		// Convert result into a string.  Sure wish we had working snprintf...
		for(i = 0; i < 6; i++) {
			tokenText[5-i] = 0x30 + (otp % 10);
			otp /= 10;
		}
		tokenText[6]=0;

		char *labelText = otplabels[curToken];

		text_layer_set_text(label, labelText);
		text_layer_set_text(token, tokenText);
	}

	if ((curSeconds>=0) && (curSeconds<30)) {
		text_layer_set_text(ticker, itoa((30-curSeconds),10));
	} else {
		text_layer_set_text(ticker, itoa((60-curSeconds),10));
	}
}

void tz_up_click_handler(ClickRecognizerRef recognizer, void *context) {
	if(tZone<24)
		tZone++;
	strcpy(gmt+3, itoa2(tZone,10));
	text_layer_set_text(tz_zone, gmt);
	changed = true;
}

void tz_down_click_handler(ClickRecognizerRef recognizer, void *context) {
	if(tZone>-24)
		tZone--;
	strcpy(gmt+3, itoa2(tZone,10));
	text_layer_set_text(tz_zone, gmt);
	changed = true;
}

void tz_click_config_provider(Window *window) {
	window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, tz_up_click_handler);
	window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, tz_down_click_handler);
}

void tz_window_load(Window *window) {
	strcpy(gmt, "UTC");
	strcpy(gmt+3, itoa2(tZone,10));
	window_set_click_config_provider(tz_window, (ClickConfigProvider) tz_click_config_provider);

	Layer *tz_layer = window_get_root_layer(tz_window);
	tz_zone = text_layer_create(GRect(0,50,144,48));
	text_layer_set_text(tz_zone, gmt);
	text_layer_set_font(tz_zone, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_alignment(tz_zone, GTextAlignmentCenter);
	text_layer_set_text_color(tz_zone, GColorWhite);
	text_layer_set_background_color(tz_zone ,GColorBlack);
	layer_add_child(tz_layer, text_layer_get_layer(tz_zone));
	
	tz_label = text_layer_create(GRect(0,5,144,48));
	text_layer_set_text(tz_label, "Change Time Zone");
	text_layer_set_font(tz_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(tz_label, GTextAlignmentCenter);
	text_layer_set_text_color(tz_label, GColorWhite);
	text_layer_set_background_color(tz_label ,GColorBlack);
	layer_add_child(tz_layer, text_layer_get_layer(tz_label));
}

void tz_window_unload(Window *window) {
	Layer *tz_layer = window_get_root_layer(window);
	layer_remove_child_layers(tz_layer);
	text_layer_destroy(tz_label);
	text_layer_destroy(tz_zone);
	persist_write_int(TZ_STORAGE_KEY, tZone + 24); //store tz
}

void main_up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (curToken==0) {
		curToken=NUM_SECRETS-1;
	} else {
		curToken--;
	};
	changed = true;
	time_t t = time(NULL);
	handle_second_tick(gmtime(&t), SECOND_UNIT);
}

void main_down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	if ((curToken+1)==NUM_SECRETS) {
		curToken=0;
	} else {
		curToken++;
	};
	changed = true;
	time_t t = time(NULL);
	handle_second_tick(gmtime(&t), SECOND_UNIT);
}

void main_select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	window_stack_push(tz_window, true);
}

void main_click_config_provider(Window *main_window) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, main_up_single_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, main_down_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, main_select_single_click_handler);	
}

void main_window_load(Window *main_window) {
	window_set_background_color(main_window, GColorBlack);
	Layer *main_layer = window_get_root_layer(main_window);

	// Init the label text layer
	label = text_layer_create(GRect(5, 27, 140, 31));
	text_layer_set_font(label, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_color(label, GColorWhite);
	text_layer_set_background_color(label, GColorClear);
	layer_add_child(main_layer, text_layer_get_layer(label));

	// Init the token text layer
	token = text_layer_create(GRect(10, 60, 140, 59));
	text_layer_set_font(token, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
	text_layer_set_text_color(token, GColorWhite);
	text_layer_set_background_color(token, GColorClear);
	layer_add_child(main_layer, text_layer_get_layer(token));

	// Init the second ticker text layer
	ticker = text_layer_create(GRect(60, 120, 140, 29));
	text_layer_set_font(ticker, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text_color(ticker, GColorWhite);
	text_layer_set_background_color(ticker, GColorClear);
	layer_add_child(main_layer, text_layer_get_layer(ticker));

}

void main_window_unload(Window *main_window) {
	tick_timer_service_unsubscribe();
	
	Layer *main_layer = window_get_root_layer(main_window);
	layer_remove_child_layers(main_layer);

	text_layer_destroy(label);		
	text_layer_destroy(token);
	text_layer_destroy(ticker);
}

void init(void) {
	tZone = persist_exists(TZ_STORAGE_KEY) ? (persist_read_int(TZ_STORAGE_KEY) - 24): 0;
	changed = true;

	tz_window = window_create();
	window_set_window_handlers(tz_window, (WindowHandlers) {
		.load = tz_window_load,
		.unload = tz_window_unload,
	});
	window_set_background_color(tz_window, GColorBlack);
	
	main_window = window_create();
	window_set_window_handlers(main_window, (WindowHandlers) {
    	.load = main_window_load,
    	.unload = main_window_unload,
  	});
	tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
	window_set_click_config_provider(main_window, (ClickConfigProvider) main_click_config_provider);
	window_stack_push(main_window, true /* Animated */);
}

void deinit(void) {
	window_destroy(main_window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
