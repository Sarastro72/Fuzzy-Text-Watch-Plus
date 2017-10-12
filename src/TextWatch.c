#include <pebble.h>
#ifdef PBL_COLOR
  #include "gcolor_definitions.h" // Allows the use of color
#endif

#include "num2words.h"
#include "TextWatch.h"

Window *window;

Line lines[NUM_LINES];

int currentMinutes;
int currentNLines;

// Corrent color of normal text
GColor8 regularTextColor;
// Corrent color of bold text
GColor8 boldTextColor;

// Time in seconds to add to the current time before calculating which
// 5-minute period we should display
int timeOffset;

// Variale to keep track of the last minute that we updated the time
// Used to optimise so we only need to run time logic once per minute.
int lastMinute = -1;

// Time in seconds since epoch when a displayed message should be removed.
// Only set to non zero when a message is displaying.
time_t resetMessageTime = 0;

// Time in seconds since epoch when connection lost message will be displayed,
// if connection is still lost... (attempt to reduce false notifications)
time_t connectionLostTime = 0;

// Time in seconds to diplay the greeting messages at startup. 0 means disable
// greeting messages.
int messageTime = 3;

// Which gesture to activate date screen
// 0 = off
// 1 = Boxing move (X-axis)
// 2 = flick wrist (Y-axis)
// 3 = Shake up/down (Z-axis)
// 4 = Any shake
int dateGesture = GESTURE_ANY;

// Notify when BT connection is lost?
// 0 = off
// 1 = text and light only
// 2 = on
int bt_lost_notification = BT_NOTIFY_ON;

// Screen resolution. Set in the init function.
int xres;
int yres;

// UTF8 aware strlen() for a sequence of bytes
int strlenUtf8(char *start, char *end) 
{
	int span = end - start;
	int i = 0, j = 0;
	while (start[i] && i < span) 
	{
		if ((start[i] & 0xc0) != 0x80) j++;
		i++;
	}
	return j;
}

// Animation handler
void animationStoppedHandler(struct Animation *animation, bool finished, void *context)
{
	TextLayer *current = (TextLayer *)context;
	GRect rect = layer_get_frame((Layer *)current);
	rect.origin.x = xres;
	layer_set_frame((Layer *)current, rect);
}

// Animate line
void makeAnimationsForLayer(Line *line, int delay)
{
	TextLayer *current = line->currentLayer;
	TextLayer *next = line->nextLayer;

#ifdef PBL_PLATFORM_APLITE
	// Destroy old animations 
	if (line->animation1 != NULL)
	{
		 property_animation_destroy(line->animation1);
	}
	if (line->animation2 != NULL)
	{
		 property_animation_destroy(line->animation2);
	}
#endif

	// Configure animation for current layer to move out
	GRect rect = layer_get_frame((Layer *)current);
	rect.origin.x =  -xres;
	line->animation1 = property_animation_create_layer_frame((Layer *)current, NULL, &rect);
	Animation *animation = property_animation_get_animation(line->animation1);
	animation_set_duration(animation, ANIMATION_DURATION);
	animation_set_delay(animation, delay);
	animation_set_curve(animation, AnimationCurveEaseIn); // Accelerate

	// Configure animation for current layer to move in
	GRect rect2 = layer_get_frame((Layer *)next);
	rect2.origin.x = 0;
	line->animation2 = property_animation_create_layer_frame((Layer *)next, NULL, &rect2);
	animation = property_animation_get_animation(line->animation2);	
	animation_set_duration(animation, ANIMATION_DURATION);
	animation_set_delay(animation, delay + ANIMATION_OUT_IN_DELAY);
	animation_set_curve(animation, AnimationCurveEaseOut); // Deaccelerate

	// Set a handler to rearrange layers after animation is finished
	animation_set_handlers(animation, (AnimationHandlers) {
		.stopped = (AnimationStoppedHandler)animationStoppedHandler
	}, current);

	// Start the animations
	animation_schedule(property_animation_get_animation(line->animation1));
	animation_schedule(property_animation_get_animation(line->animation2));	
}

void updateLayerText(TextLayer* layer, char* text)
{
	const char* layerText = text_layer_get_text(layer);
	strcpy((char*)layerText, text);
	// To mark layer dirty
	text_layer_set_text(layer, layerText);
    //layer_mark_dirty(&layer->layer);
}

// Update line
void updateLineTo(Line *line, char *value, int delay)
{
	updateLayerText(line->nextLayer, value);
	makeAnimationsForLayer(line, delay);

	// Swap current/next layers
	TextLayer *tmp = line->nextLayer;
	line->nextLayer = line->currentLayer;
	line->currentLayer = tmp;
}

// Check to see if the current line needs to be updated
bool needToUpdateLine(Line *line, char *nextValue)
{
	const char *currentStr = text_layer_get_text(line->currentLayer);

	if (strcmp(currentStr, nextValue) != 0) {
		return true;
	}
	return false;
}

// Configure bold line of text
void configureBoldLayer(TextLayer *textlayer)
{
	text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_color(textlayer, boldTextColor);
	text_layer_set_background_color(textlayer, GColorClear);
	text_layer_set_text_alignment(textlayer, TEXT_ALIGN);
}

// Configure bold line of text
void configureSmallBoldLayer(TextLayer *textlayer)
{
	text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
	text_layer_set_text_color(textlayer, boldTextColor);
	text_layer_set_background_color(textlayer, GColorClear);
	text_layer_set_text_alignment(textlayer, TEXT_ALIGN);
}

// Configure light line of text
void configureLightLayer(TextLayer *textlayer)
{
	text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
	text_layer_set_text_color(textlayer, regularTextColor);
	text_layer_set_background_color(textlayer, GColorClear);
	text_layer_set_text_alignment(textlayer, TEXT_ALIGN);
}

// Configure small line of text
void configureSmallLayer(TextLayer *textlayer)
{
	text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_color(textlayer, regularTextColor);
	text_layer_set_background_color(textlayer, GColorClear);
	text_layer_set_text_alignment(textlayer, TEXT_ALIGN);
}

// Configure the layers for the given text
int configureLayersForText(char text[NUM_LINES][BUFFER_SIZE], char format[])
{
	int numLines = 0;
	int height = 0;
	int offsets[4];

	// Set bold layer.
	int i;
	for (i = 0; i < NUM_LINES; i++) {
		if (strlen(text[i]) == 0) {
			break;
		}

		offsets[i] = ROW_OFFSET;
		if (format[i] == 'B') // Bold
		{
			configureBoldLayer(lines[i].nextLayer);
		}
		else if (format[i] == 'b') // Small bold
		{
			configureSmallBoldLayer(lines[i].nextLayer);
			offsets[i] = ROW_OFFSET_SMALL;
			// If there is a line above, increase its offset a bit 
			if (i > 0) {
				offsets[i - 1] += TOP_MARGIN_SMALL;
				height += TOP_MARGIN_SMALL;
			}
		}
		else if (format[i] == 's') // Small
		{
			configureSmallLayer(lines[i].nextLayer);
			offsets[i] = ROW_OFFSET_SMALL;
			// If there is a line above, increase its offset a bit 
			if (i > 0) {
				offsets[i - 1] += TOP_MARGIN_SMALL;
				height += TOP_MARGIN_SMALL;
			}
		}
		else // Normal line
		{
			configureLightLayer(lines[i].nextLayer);
		}
		height += offsets[i];
	}
	numLines = i;

	// Calculate y position of top Line
	int ypos = (yres - height) / 2 - TOP_MARGIN;

	// Set y positions for the lines
	for (int i = 0; i < numLines; i++)
	{
		layer_set_frame((Layer *)lines[i].nextLayer, GRect(xres, ypos, xres, ROW_HEIGHT));
		ypos += offsets[i];
	}

	return numLines;
}

void string_to_lines(char *str, char lines[NUM_LINES][BUFFER_SIZE], char format[])
{
	char *start = str;
	char *end = strstr(start, " ");
	int l = 0;

	// Empty all lines
	for (int i = 0; i < NUM_LINES; i++)
	{
		lines[i][0] = '\0';
	}

	while (end != NULL && l < NUM_LINES) {
		// Check word for bold prefix
		if (*start == '*' && end - start > 1)
		{
			// Mark line bold and move start to the first character of the word
			format[l] = 'B';
			start++;
			if (*start == '<') { // small bold, actually
				format[l] = 'b';
				start++;
			}
		}
		else if (*start == '<' && end - start > 1) {
			// Mark line small
			format[l] = 's';
			start++;
		} else {
			// Mark line normal
			format[l] = ' ';
		}

		char *nextWord = end + 1;

		// Can we add another word to the line?
		if (format[l] == ' ' && *nextWord != '*'  // are both lines formatted normal?
			&& *nextWord != ' '                   // no no-join annotation (double space)
			&& strlenUtf8(start, end) < LINE_APPEND_LIMIT)  // is the first word short enough?
		{
			// See if next word fits
			char *try = strstr(end + 1, " ");
			if (try != NULL
				&& strlenUtf8(start, try) <= LINE_LENGTH)
			{
				end = try;
			}
		}

		// copy to line
		*end = '\0';
		strcpy(lines[l++], start);

		// Look for next word
		start = end + 1;
		while (*start == ' ') start++; // Skip all spaces
		end = strstr(start, " ");
	}
}

void time_to_lines(int hours, int minutes, char lines[NUM_LINES][BUFFER_SIZE], char format[])
{
	int length = NUM_LINES * BUFFER_SIZE + 1;
	char timeStr[length];
	time_to_words(hours, minutes, timeStr, length);
	
	string_to_lines(timeStr, lines, format);
}

// Update screen based on new time
void display_message(char *message, int displayTime)
{
	if (displayTime > 0 && resetMessageTime == 0) {
		// The current time text will be stored in the following strings
		char textLine[NUM_LINES][BUFFER_SIZE];
		char format[NUM_LINES];

		string_to_lines(message, textLine, format);
		
		int nextNLines = configureLayersForText(textLine, format);

		int delay = 0;
		for (int i = 0; i < NUM_LINES; i++) {
		    updateLineTo(&lines[i], textLine[i], delay);
		    delay += ANIMATION_STAGGER_TIME;
		}
		
		currentNLines = nextNLines;

		time(&resetMessageTime);
		resetMessageTime += displayTime;
	}
}

// Update screen based on new time
void display_time(struct tm *t, bool force)
{
	if (resetMessageTime != 0) { // Don't update time if a message is showing
		return;
	}

	time_t timestamp = mktime(t);
	timestamp += timeOffset; // Add offset time
	t = localtime(&timestamp);

#if DEBUG == 0
	if (lastMinute == t->tm_min && !force) { // No change in time
		return;
	}
#endif

	// Mark this minute as checked;
	lastMinute = t->tm_min;

	// The current time text will be stored in the following strings
	char textLine[NUM_LINES][BUFFER_SIZE];
	char format[NUM_LINES];

#if DEBUG == 1
	time_to_lines(t->tm_hour, t->tm_sec, textLine, format);
#else
	time_to_lines(t->tm_hour, t->tm_min, textLine, format);
#endif
	
	int nextNLines = configureLayersForText(textLine, format);

	int delay = 0;
	for (int i = 0; i < NUM_LINES; i++) {
		if (force || nextNLines != currentNLines || needToUpdateLine(&lines[i], textLine[i])) {
			updateLineTo(&lines[i], textLine[i], delay);
			delay += ANIMATION_STAGGER_TIME;
		}
	}
	
	currentNLines = nextNLines;
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Tap event: %d  conf: %d", axis, dateGesture);

  if (dateGesture != GESTURE_ANY) {
  	if (axis == ACCEL_AXIS_X && dateGesture != GESTURE_X) {
	  return;
	}
  	if (axis == ACCEL_AXIS_Y && dateGesture != GESTURE_Y) {
	  return;
	}
  	if (axis == ACCEL_AXIS_Z && dateGesture != GESTURE_Z) {
	  return;
	}
  }

  struct tm* t = get_localtime();
  char message[32];
  snprintf(message, 32, "<%02d:%02d:%02d  %d/%d  %d ",
  	t->tm_hour,
  	t->tm_min,
  	t->tm_sec,
  	t->tm_mday,
  	t->tm_mon + 1,
  	t->tm_year + 1900);
  display_message(message, 6);
}

void check_connection(time_t *now) {
	if (connectionLostTime > 0 && connectionLostTime <= *now) {
		if (!connection_service_peek_pebble_app_connection()) {
			notify_bt_lost();
		}
		connectionLostTime = 0;
	}
}

// Time handler called every second by the system
void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  // If resetMessageTime != 0, then display_time() will not update screen
  bool force = false;
  time_t now;
  time(&now);
  if (resetMessageTime != 0) {
  	if (now >= resetMessageTime) {
  		resetMessageTime = 0;
  		force = true;
  	}
  }

  check_connection(&now);

  display_time(tick_time, force);
}

void init_line(Line* line) {
	// Create layers with dummy position to the right of the screen
	line->currentLayer = text_layer_create(GRect(xres, 0, xres, ROW_HEIGHT));
	line->nextLayer = text_layer_create(GRect(xres, 0, xres, ROW_HEIGHT));

	// Configure a style
	configureLightLayer(line->currentLayer);
	configureLightLayer(line->nextLayer);

	// Set the text buffers
	line->lineStr1[0] = '\0';
	line->lineStr2[0] = '\0';
	text_layer_set_text(line->currentLayer, line->lineStr1);
	text_layer_set_text(line->nextLayer, line->lineStr2);

	// Initially there are no animations
	line->animation1 = NULL;
	line->animation2 = NULL;
}

struct tm *get_localtime()
{
	time_t raw_time;
	time(&raw_time);
	return localtime(&raw_time);
}

void refresh_time() {
	display_time(get_localtime(), true);
}

void set_offset(int offset) {
	timeOffset = offset;
}

void set_message_time(int mTime) {
	messageTime = mTime;
}

void set_gesture(int gesture) {
	dateGesture = gesture;
	accel_tap_service_unsubscribe();
	if (gesture != GESTURE_OFF) {
		accel_tap_service_subscribe(accel_tap_handler);
	}
}

void set_bt_lost_notification(int bt_notification) {
	bt_lost_notification = bt_notification;
}

void inbox_received_handler(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received inbox message");

  // Language
  Tuple *language_t = dict_find(iter, KEY_LANGUAGE);
  if (language_t) {
  	set_language(language_t->value->uint8);
  	persist_write_int(KEY_LANGUAGE, language_t->value->uint8);
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Language is %d", language_t->value->uint8);
  }

  // Time offset
  Tuple *offset_t = dict_find(iter, KEY_OFFSET);
  if (offset_t) {
  	set_offset(offset_t->value->uint16);
  	persist_write_int(KEY_OFFSET, offset_t->value->uint16);
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Offset is %d", offset_t->value->uint16);
  }

  // Message time
  Tuple *message_time_t = dict_find(iter, KEY_MESSAGE_TIME);
  if (message_time_t) {
  	set_message_time(message_time_t->value->uint8);
  	persist_write_int(KEY_MESSAGE_TIME, message_time_t->value->uint8);
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Message time is %d", message_time_t->value->uint8);
  }

  // Gesture
  Tuple *gesture_t = dict_find(iter, KEY_GESTURE);
  if (gesture_t) {
  	set_gesture(gesture_t->value->uint8);
  	persist_write_int(KEY_GESTURE, gesture_t->value->uint8);
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Gesture is %d", gesture_t->value->uint8);
  }

  // BT lost notification
  Tuple *bt_notification_t = dict_find(iter, KEY_BT_NOTIFICATION);
  if (bt_notification_t) {
  	set_bt_lost_notification(bt_notification_t->value->uint8);
  	persist_write_int(KEY_BT_NOTIFICATION, bt_notification_t->value->uint8);
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "BT notification is %d", bt_notification_t->value->uint8);
  }

#ifdef PBL_COLOR
  // Background color
  Tuple *background_color_t = dict_find(iter, KEY_BACKGROUND);
  if(background_color_t) {
  	GColor8 bg_color;

  	bg_color.argb = background_color_t->value->uint8;
  	window_set_background_color(window, bg_color);
  	persist_write_int(KEY_BACKGROUND, bg_color.argb);	
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Offset is %d", background_color_t->value->uint8);
  }

  // Regular text color
  Tuple *regular_text_t = dict_find(iter, KEY_REGULAR_TEXT);
  if(regular_text_t) {
  	regularTextColor.argb = regular_text_t->value->uint8;
  	persist_write_int(KEY_REGULAR_TEXT, regularTextColor.argb);
  }

  // Bold text color
  Tuple *bold_text_t = dict_find(iter, KEY_BOLD_TEXT);
  if(bold_text_t) {
  	boldTextColor.argb = bold_text_t->value->uint8;
  	persist_write_int(KEY_BOLD_TEXT, boldTextColor.argb);
  }

#else
  // Inverse colors
  Tuple *color_inverse_t = dict_find(iter, KEY_INVERSE);
  if(color_inverse_t) {
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Inverse colors is %d", color_inverse_t->value->int8);
  	if (color_inverse_t->value->int8 > 0) {  // Read boolean as an integer
	    // Set inverse colors
	    window_set_background_color(window, GColorWhite);
	    regularTextColor.argb = GColorBlack.argb;
	    boldTextColor.argb = GColorBlack.argb;
	    // Persist value
	    persist_write_bool(KEY_INVERSE, true);
	} else {
	    // Set normal colors
	    window_set_background_color(window, GColorBlack);
	    regularTextColor.argb = GColorWhite.argb;
	    boldTextColor.argb = GColorWhite.argb;
	    // Persist value
	    persist_write_bool(KEY_INVERSE, false);
	}
  }

#endif

  refresh_time();
}

void notify_bt_lost() {
	if (bt_lost_notification != BT_NOTIFY_OFF) {
		if (bt_lost_notification == BT_NOTIFY_ON && !quiet_time_is_active()) {
			vibes_long_pulse();
		}
		light_enable_interaction();
		char message[48];
		get_connection_lost_message(message);
		display_message(message, BT_LOST_DISPLAY_TIME);	
	}
}

void bt_handler(bool connected) {
	if (connected) {
		connectionLostTime = 0;
	} else {
		time_t now;
		time(&now);
		connectionLostTime = now + CONNECTION_LOST_MARGIN;
	}
}

void readPersistedState() {
	if (persist_exists(KEY_LANGUAGE)) {
		set_language(persist_read_int(KEY_LANGUAGE));
	}

	if (persist_exists(KEY_OFFSET)) {
		set_offset(persist_read_int(KEY_OFFSET));
	}

	if (persist_exists(KEY_MESSAGE_TIME)) {
		set_message_time(persist_read_int(KEY_MESSAGE_TIME));
	}

	if (persist_exists(KEY_GESTURE)) {
		set_gesture(persist_read_int(KEY_GESTURE));
	}

	if (persist_exists(KEY_BT_NOTIFICATION)) {
		set_bt_lost_notification(persist_read_int(KEY_BT_NOTIFICATION));
	}

	// Set default colors
	GColor8 backgroundColor;
	backgroundColor.argb = GColorBlack.argb;
	regularTextColor.argb=GColorWhite.argb;
	boldTextColor.argb=GColorWhite.argb;


#ifdef PBL_COLOR
	if (persist_exists(KEY_BACKGROUND)) {
		backgroundColor.argb = persist_read_int(KEY_BACKGROUND);
	}

	if (persist_exists(KEY_REGULAR_TEXT)) {
		regularTextColor.argb = persist_read_int(KEY_REGULAR_TEXT);
	}

	if (persist_exists(KEY_BOLD_TEXT)) {
		boldTextColor.argb = persist_read_int(KEY_BOLD_TEXT);
	}
#else
	if (persist_read_bool(KEY_INVERSE)) {
		backgroundColor.argb = GColorWhite.argb;
		regularTextColor.argb=GColorBlack.argb;
		boldTextColor.argb=GColorBlack.argb;
	}
#endif

	// Set background color
	window_set_background_color(window, backgroundColor);
}

void handle_init() {
	window = window_create();
	window_stack_push(window, true);

	Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);
    xres = window_bounds.size.w;
    yres = window_bounds.size.h;

    // Subscribe to taps
	accel_tap_service_subscribe(accel_tap_handler);


	readPersistedState();

	// Init and load lines
	for (int i = 0; i < NUM_LINES; i++)
	{
		init_line(&lines[i]);
	  	layer_add_child(window_layer, (Layer *)lines[i].currentLayer);
		layer_add_child(window_layer, (Layer *)lines[i].nextLayer);
	}

	// Show greeting message
	char greeting[48];
	time_to_greeting(get_localtime()->tm_hour, greeting);
#if DEBUG == 1
	time_to_greeting(get_localtime()->tm_sec * 24 / 60, greeting);
#endif
	if (messageTime > 0) {
		display_message(greeting, messageTime);
	} else {
		refresh_time();
	}
	// Subscribe to ticks
	tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

	// Subscribe to bluetooth events
	connection_service_subscribe((ConnectionHandlers) {
	  .pebble_app_connection_handler = bt_handler
	});

	// Set up listener for configuration changes
	app_message_register_inbox_received(inbox_received_handler);
  	AppMessageResult result = app_message_open(512, 512);
  	if (result != APP_MSG_OK) {
  		APP_LOG(APP_LOG_LEVEL_WARNING, "app_message_open() failed with error %d", result);
  	}
}

void destroy_line(Line* line)
{
	// Free layers
	text_layer_destroy(line->currentLayer);
	text_layer_destroy(line->nextLayer);
}

void handle_deinit()
{
	// Free lines
	for (int i = 0; i < NUM_LINES; i++)
	{
		destroy_line(&lines[i]);
	}

	// Free window
	window_destroy(window);
}

int main(void)
{
	handle_init();
	app_event_loop();
	handle_deinit();
}

