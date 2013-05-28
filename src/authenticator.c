#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "configuration.h"
#include "sha1.h"

// defined in editTzone.c
extern void showEditTimeZone();

#define SHA1_SIZE 20

#define MY_UUID { 0xA4, 0xA6, 0x13, 0xB5, 0x8A, 0x6B, 0x4F, 0xF0, 0xBD, 0x80, 0x00, 0x38, 0xA1, 0x51, 0xCD, 0x86 }
PBL_APP_INFO(MY_UUID,
		"Authenticator", "pokey9000/IEF/rigel314/cwoac",
		1, 3, /* App version */
		RESOURCE_ID_IMAGE_MENU_ICON,
		APP_INFO_STANDARD_APP);

Window window;

TextLayer label;
TextLayer token;
TextLayer ticker;

int curToken = 0;
int tZone;
bool changed;


// return seconds since epoch compensating for Pebble's lack of location
// independent GMT

int curSeconds=0;

uint32_t get_epoch_seconds() {
	PblTm current_time;
	uint32_t unix_time;
	get_time(&current_time);
	
// shamelessly stolen from WhyIsThisOpen's Unix Time source: http://forums.getpebble.com/discussion/4324/watch-face-unix-time
	/* Convert time to seconds since epoch. */
	//curSeconds=current_time.tm_sec;
	unix_time = ((0-tZone)*3600) + /* time zone offset */          /* 0-tZone+current_time.tm_isdst if it ever starts working. */
		+ current_time.tm_sec /* start with seconds */
		+ current_time.tm_min*60 /* add minutes */
		+ current_time.tm_hour*3600 /* add hours */
		+ current_time.tm_yday*86400 /* add days */
		+ (current_time.tm_year-70)*31536000 /* add years since 1970 */
		+ ((current_time.tm_year-69)/4)*86400 /* add a day after leap years, starting in 1973 */                                                                       - ((current_time.tm_year-1)/100)*86400 /* remove a leap day every 100 years, starting in 2001 */                                                               + ((current_time.tm_year+299)/400)*86400; /* add a leap day back every 400 years, starting in 2001*/
	unix_time /= 30;
	return unix_time;
}


void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t) {

	(void)t;
	(void)ctx;

	static char tokenText[] = TOKEN_TEXT; // Needs to be static because it's used by the system later.

	sha1nfo s;
	uint8_t ofs;
	uint32_t otp;
	int i;
	uint32_t unix_time;
	unsigned char sha1_time[8] = {0, 0, 0, 0, 0, 0, 0, 0};

	PblTm curTime;
	get_time(&curTime);
	curSeconds = curTime.tm_sec;

	if(curSeconds == 0 || curSeconds == 30 || changed)
	{
		changed = false;

		// TOTP uses seconds since epoch in the upper half of an 8 byte payload
		// TOTP is HOTP with a time based payload
		// HOTP is HMAC with a truncation function to get a short decimal key
		unix_time = get_epoch_seconds();
		sha1_time[4] = (unix_time >> 24) & 0xFF;
		sha1_time[5] = (unix_time >> 16) & 0xFF;
		sha1_time[6] = (unix_time >> 8) & 0xFF;
		sha1_time[7] = unix_time & 0xFF;

		// First get the HMAC hash of the time payload with the shared key
		sha1_initHmac(&s, otpkeys[curToken], otpsizes[curToken]);
		sha1_write(&s, (const char *)sha1_time, 8);
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
		for(i = 1; i <= otplengths[curToken]; i++) {
			tokenText[otplengths[curToken]-i] = 0x30 + (otp % 10);
			otp /= 10;
		}
		tokenText[otplengths[curToken]]=0;

		char *labelText = otplabels[curToken];

		text_layer_set_text(&label, labelText);
		text_layer_set_text(&token, tokenText);
	}

	if ((curSeconds>=0) && (curSeconds<30)) {
		text_layer_set_text(&ticker, itoa((30-curSeconds),10));
	} else {
		text_layer_set_text(&ticker, itoa((60-curSeconds),10));
	}
}

void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	if (curToken==0) {
		curToken=NUM_SECRETS-1;
	} else {
		curToken--;
	};
	changed = true;
	handle_second_tick(NULL,NULL);
}

void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
	if ((curToken+1)==NUM_SECRETS) {
		curToken=0;
	} else {
		curToken++;
	};
	changed = true;
	handle_second_tick(NULL,NULL);
}

void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	(void)recognizer;
	(void)window;

	showEditTimeZone();
}

void click_config_provider(ClickConfig **config, Window *window) {
  (void)window;

  config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
  config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;

  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
  config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;

  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;
}

void handle_init(AppContextRef ctx) {
	(void)ctx;

	tZone = DEFAULT_TIME_ZONE;
	changed = true;

	window_init(&window, "auth");
	window_stack_push(&window, true /* Animated */);
	window_set_background_color(&window, GColorBlack);

	// Init the identifier label
	text_layer_init(&label, GRect(5, 30, 144-4, 168-44));
	text_layer_set_text_color(&label, GColorWhite);
	text_layer_set_background_color(&label, GColorClear);
	text_layer_set_font(&label, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));

	// Init the token label
	text_layer_init(&token, GRect(10, 60, 144-4 /* width */, 168-44 /* height */));
	text_layer_set_text_color(&token, GColorWhite);
	text_layer_set_background_color(&token, GColorClear);
	text_layer_set_font(&token, fonts_get_system_font(FONT_KEY_GOTHAM_34_MEDIUM_NUMBERS));

	// Init the second ticker
	text_layer_init(&ticker, GRect(60, 120, 144-4 /* width */, 168-44 /* height */));
	text_layer_set_text_color(&ticker, GColorWhite);
	text_layer_set_background_color(&ticker, GColorClear);
	text_layer_set_font(&ticker, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));

	handle_second_tick(ctx, NULL);
	layer_add_child(&window.layer, &label.layer);
	layer_add_child(&window.layer, &token.layer);
	layer_add_child(&window.layer, &ticker.layer);

	window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);
}


void pbl_main(void *params) {
	PebbleAppHandlers handlers = {
		.init_handler = &handle_init,
		.tick_info = {
			.tick_handler = &handle_second_tick,
			.tick_units = SECOND_UNIT
		}
	};
	app_event_loop(params, &handlers);
}
