#include <pebble.h>

#include "configuration.h"

// defined in editTzone.c
extern void showEditTimeZone();
extern void destroyEditTimeZone();

// Truncate n decimal digits to 2^n for 6 digits
#define DIGITS_TRUNCATE 1000000

#define SHA1_SIZE 20

Window *window;

TextLayer *label;
TextLayer *token;
TextLayer *ticker;

#define KEY_CURTOKEN 1
int curToken;
int curToken_orig;

#define KEY_TZONE 2
int tZone;
int tZone_orig; //used to track config changes

bool changed;

/* from sha1.c from liboauth */

/* This code is public-domain - it is based on libcrypt
 * placed in the public domain by Wei Dai and other contributors.
 */

#include <string.h>

/* header */

#define HASH_LENGTH 20
#define BLOCK_LENGTH 64

union _buffer {
	uint8_t b[BLOCK_LENGTH];
	uint32_t w[BLOCK_LENGTH/4];
};

union _state {
	uint8_t b[HASH_LENGTH];
	uint32_t w[HASH_LENGTH/4];
};

typedef struct sha1nfo {
	union _buffer buffer;
	uint8_t bufferOffset;
	union _state state;
	uint32_t byteCount;
	uint8_t keyBuffer[BLOCK_LENGTH];
	uint8_t innerHash[HASH_LENGTH];
} sha1nfo;

/* public API - prototypes - TODO: doxygen*/

/*
 void sha1_init(sha1nfo *s);
 void sha1_writebyte(sha1nfo *s, uint8_t data);
 void sha1_write(sha1nfo *s, const char *data, size_t len);
 uint8_t* sha1_result(sha1nfo *s);
 void sha1_initHmac(sha1nfo *s, const uint8_t* key, int keyLength);
 uint8_t* sha1_resultHmac(sha1nfo *s);
 */

char* itoa(int val, int base){
	static char buf[32] = {0};
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = "0123456789abcdef"[val % base];
	return &buf[i+1];
}

/* code */
#define SHA1_K0 0x5a827999
#define SHA1_K20 0x6ed9eba1
#define SHA1_K40 0x8f1bbcdc
#define SHA1_K60 0xca62c1d6

const uint8_t sha1InitState[] = {
	0x01,0x23,0x45,0x67, // H0
	0x89,0xab,0xcd,0xef, // H1
	0xfe,0xdc,0xba,0x98, // H2
	0x76,0x54,0x32,0x10, // H3
	0xf0,0xe1,0xd2,0xc3  // H4
};

void sha1_init(sha1nfo *s) {
	memcpy(s->state.b,sha1InitState,HASH_LENGTH);
	s->byteCount = 0;
	s->bufferOffset = 0;
}

uint32_t sha1_rol32(uint32_t number, uint8_t bits) {
	return ((number << bits) | (number >> (32-bits)));
}

void sha1_hashBlock(sha1nfo *s) {
	uint8_t i;
	uint32_t a,b,c,d,e,t;
    
	a=s->state.w[0];
	b=s->state.w[1];
	c=s->state.w[2];
	d=s->state.w[3];
	e=s->state.w[4];
	for (i=0; i<80; i++) {
		if (i>=16) {
			t = s->buffer.w[(i+13)&15] ^ s->buffer.w[(i+8)&15] ^ s->buffer.w[(i+2)&15] ^ s->buffer.w[i&15];
			s->buffer.w[i&15] = sha1_rol32(t,1);
		}
		if (i<20) {
			t = (d ^ (b & (c ^ d))) + SHA1_K0;
		} else if (i<40) {
			t = (b ^ c ^ d) + SHA1_K20;
		} else if (i<60) {
			t = ((b & c) | (d & (b | c))) + SHA1_K40;
		} else {
			t = (b ^ c ^ d) + SHA1_K60;
		}
		t+=sha1_rol32(a,5) + e + s->buffer.w[i&15];
		e=d;
		d=c;
		c=sha1_rol32(b,30);
		b=a;
		a=t;
	}
	s->state.w[0] += a;
	s->state.w[1] += b;
	s->state.w[2] += c;
	s->state.w[3] += d;
	s->state.w[4] += e;
}

void sha1_addUncounted(sha1nfo *s, uint8_t data) {
	s->buffer.b[s->bufferOffset ^ 3] = data;
	s->bufferOffset++;
	if (s->bufferOffset == BLOCK_LENGTH) {
		sha1_hashBlock(s);
		s->bufferOffset = 0;
	}
}

void sha1_writebyte(sha1nfo *s, uint8_t data) {
	++s->byteCount;
	sha1_addUncounted(s, data);
}

void sha1_write(sha1nfo *s, const char *data, size_t len) {
	for (;len--;) sha1_writebyte(s, (uint8_t) *data++);
}

void sha1_pad(sha1nfo *s) {
	// Implement SHA-1 padding (fips180-2 §5.1.1)
    
	// Pad with 0x80 followed by 0x00 until the end of the block
	sha1_addUncounted(s, 0x80);
	while (s->bufferOffset != 56) sha1_addUncounted(s, 0x00);
    
	// Append length in the last 8 bytes
	sha1_addUncounted(s, 0); // We're only using 32 bit lengths
	sha1_addUncounted(s, 0); // But SHA-1 supports 64 bit lengths
	sha1_addUncounted(s, 0); // So zero pad the top bits
	sha1_addUncounted(s, s->byteCount >> 29); // Shifting to multiply by 8
	sha1_addUncounted(s, s->byteCount >> 21); // as SHA-1 supports bitstreams as well as
	sha1_addUncounted(s, s->byteCount >> 13); // byte.
	sha1_addUncounted(s, s->byteCount >> 5);
	sha1_addUncounted(s, s->byteCount << 3);
}

uint8_t* sha1_result(sha1nfo *s) {
	int i;
	// Pad to complete the last block
	sha1_pad(s);
    
	// Swap byte order back
	for (i=0; i<5; i++) {
		uint32_t a,b;
		a=s->state.w[i];
		b=a<<24;
		b|=(a<<8) & 0x00ff0000;
		b|=(a>>8) & 0x0000ff00;
		b|=a>>24;
		s->state.w[i]=b;
	}
    
	// Return pointer to hash (20 characters)
	return s->state.b;
}

#define HMAC_IPAD 0x36
#define HMAC_OPAD 0x5c

void sha1_initHmac(sha1nfo *s, const uint8_t* key, int keyLength) {
	uint8_t i;
	memset(s->keyBuffer, 0, BLOCK_LENGTH);
	if (keyLength > BLOCK_LENGTH) {
		// Hash long keys
		sha1_init(s);
		for (;keyLength--;) sha1_writebyte(s, *key++);
		memcpy(s->keyBuffer, sha1_result(s), HASH_LENGTH);
	} else {
		// Block length keys are used as is
		memcpy(s->keyBuffer, key, keyLength);
	}
	// Start inner hash
	sha1_init(s);
	for (i=0; i<BLOCK_LENGTH; i++) {
		sha1_writebyte(s, s->keyBuffer[i] ^ HMAC_IPAD);
	}
}

uint8_t* sha1_resultHmac(sha1nfo *s) {
	uint8_t i;
	// Complete inner hash
	memcpy(s->innerHash,sha1_result(s),HASH_LENGTH);
	// Calculate outer hash
	sha1_init(s);
	for (i=0; i<BLOCK_LENGTH; i++) sha1_writebyte(s, s->keyBuffer[i] ^ HMAC_OPAD);
	for (i=0; i<HASH_LENGTH; i++) sha1_writebyte(s, s->innerHash[i]);
	return sha1_result(s);
}

/* end sha1.c */

void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {

    (void) units_changed;
	static char tokenText[] = "RYRYRY"; // Needs to be static because it's used by the system later.

	sha1nfo s;
	uint8_t ofs;
	uint32_t otp;
	int i;
	uint32_t unix_time;
    time_t current_time ;
	char sha1_time[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int curSeconds;

    current_time=time(NULL);
    unix_time=current_time + ((0-tZone)*3600) ; //still needed because time() is not GMT
    unix_time /= 30;
    if (tick_time == NULL) {
        tick_time = localtime(&current_time);
    }
	curSeconds = tick_time->tm_sec;

	if(curSeconds == 0 || curSeconds == 30 || changed)
	{
		changed = false;

		// TOTP uses seconds since epoch in the upper half of an 8 byte payload
		// TOTP is HOTP with a time based payload
		// HOTP is HMAC with a truncation function to get a short decimal key
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

void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    (void)recognizer ;
    (void)context ;
    
	if (curToken==0) {
		curToken=NUM_SECRETS-1;
	} else {
		curToken--;
	};
	changed = true;
	handle_second_tick(NULL,SECOND_UNIT);
}

void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    (void)recognizer;
    (void)context;
    
    curToken = (curToken + 1 ) % NUM_SECRETS ;
	changed = true;
	handle_second_tick(NULL,SECOND_UNIT);
}

void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	(void)recognizer;
	(void)context;

	showEditTimeZone();
}

void click_config_provider(void *context) {

    // window_set_click_context(BUTTON_ID_UP, context);
    window_single_repeating_click_subscribe(BUTTON_ID_UP,   100,   up_single_click_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_single_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
}


void handle_init(void) {

    // get timezone from persistent storage, if found
	tZone = persist_exists(KEY_TZONE) ? persist_read_int(KEY_TZONE) : DEFAULT_TIME_ZONE;
    tZone_orig=tZone ;
    
    // get saved current token
    curToken = persist_exists(KEY_CURTOKEN) ? persist_read_int(KEY_CURTOKEN) : 0 ;
    curToken_orig = curToken;
    
	changed = true;

	window = window_create();
    window_stack_push(window, true /* Animated */);
	window_set_background_color(window, GColorBlack);

	// Init the identifier label
    label=text_layer_create(GRect(5, 30, 144-4, 168-44));

	text_layer_set_text_color(label, GColorWhite);
	text_layer_set_background_color(label, GColorClear);
	text_layer_set_font(label, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));

    
	// Init the token label
	token=text_layer_create(GRect(10, 60, 144-4 /* width */, 168-44 /* height */));
	text_layer_set_text_color(token, GColorWhite);
	text_layer_set_background_color(token, GColorClear);
	text_layer_set_font(token, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));

	// Init the second ticker
	ticker=text_layer_create(GRect(60, 120, 144-4 /* width */, 168-44 /* height */));
	text_layer_set_text_color(ticker, GColorWhite);
	text_layer_set_background_color(ticker, GColorClear);
	text_layer_set_font(ticker, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));

	handle_second_tick(NULL, SECOND_UNIT);

    Layer *window_layer=window_get_root_layer(window);
	layer_add_child(window_layer, text_layer_get_layer(label));
	layer_add_child(window_layer, text_layer_get_layer(token));
	layer_add_child(window_layer, text_layer_get_layer(ticker));

    window_set_click_config_provider(window, (ClickConfigProvider) click_config_provider);
}

void handle_deinit(void) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "deinit called");
    
    //store timezone in persistence storage if needed
    if (tZone != tZone_orig) {
        if (tZone == DEFAULT_TIME_ZONE) persist_delete(KEY_TZONE) ;
        else persist_write_int(KEY_TZONE, tZone) ;
    }
    
    //save selected token
    if (curToken != curToken_orig) {
        if (curToken == 0) persist_delete(KEY_CURTOKEN);
        else persist_write_int(KEY_CURTOKEN, curToken);
    }
    
    destroyEditTimeZone();
    tick_timer_service_unsubscribe();
    text_layer_destroy(label) ;
    text_layer_destroy(token) ;
    text_layer_destroy(ticker) ;
    window_destroy(window);
}

int main() {
    handle_init();
    tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
    app_event_loop() ;
    handle_deinit() ;
}
