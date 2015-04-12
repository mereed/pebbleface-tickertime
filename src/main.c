#include <pebble.h>

static Window *s_window;

Layer* layer;
static TextLayer *s_time_layer;
TextLayer *battery_text_layer;
InverterLayer *inverter_layer_bt = NULL;

GBitmap* background;
static GBitmap *background_image;
static BitmapLayer *background_layer;

static GFont *custom_font;
static GFont *custom_font1;

int charge_percent = 0;
bool text_Displayed = false;
bool weekday = true;
bool date = true;
bool month = true;

#define WINDOW_HEIGHT 168
#define WINDOW_WIDTH 144  
#define TEXTBOX_HEIGHT 44
#define TEXTBOX_WIDTH 2000


// animates layer by number of pixels
PropertyAnimation *s_box_animation;

// prototype so anim_stopped_handler can compile (implementation below)
void animate_text();


// when watch is shaken or tapped
static void accel_tap_handler(AccelAxisType axis, int32_t direction) {   
	
    layer_set_hidden(layer, true);
    layer_set_hidden(bitmap_layer_get_layer(background_layer), false);

    if (text_Displayed) {
      animate_text();
      text_Displayed = false;
    } else {
      animate_text();
      text_Displayed = true;
    }   
}


void update_layer(Layer *me, GContext* ctx) 
{
	//watchface drawing
	
	graphics_context_set_stroke_color(ctx,GColorWhite);
	graphics_context_set_text_color(ctx, GColorWhite);
	
	//draw background
	graphics_draw_bitmap_in_rect(ctx,background,GRect(1,0,144,168));
	
	//get tick_time
	time_t temp = time(NULL); 
  	struct tm *tick_time = localtime(&temp);

	//draw hands
	GPoint center = GPoint(71,99);
	int16_t minuteHandLength = 60;
	int16_t hourHandLength = 35;
	GPoint minuteHand;
	GPoint hourHand;

	
	int32_t minute_angle = TRIG_MAX_ANGLE * tick_time->tm_min / 60;
	minuteHand.y = (int16_t)(-cos_lookup(minute_angle) * (int32_t)minuteHandLength / TRIG_MAX_RATIO) + center.y;
	minuteHand.x = (int16_t)(sin_lookup(minute_angle) * (int32_t)minuteHandLength / TRIG_MAX_RATIO) + center.x;
	graphics_draw_line(ctx, center, minuteHand);
	
	int32_t hour_angle = (TRIG_MAX_ANGLE * (((tick_time->tm_hour % 12) * 6) + (tick_time->tm_min / 10))) / (12 * 6);
	hourHand.y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)hourHandLength / TRIG_MAX_RATIO) + center.y;
	hourHand.x = (int16_t)(sin_lookup(hour_angle) * (int32_t)hourHandLength / TRIG_MAX_RATIO) + center.x;
	graphics_draw_line(ctx, center, hourHand);
	
	
	//I didn't like how a 2px path rotated, so I'm using two lines next to each other
	//I need to move the pixels from vertically adjacent to horizontally adjacent based on the position
	bool addX = (tick_time->tm_min > 20 && tick_time->tm_min < 40) || tick_time->tm_min < 10 || tick_time->tm_min > 50;
	center.x+=addX?1:0;
	center.y+=!addX?1:0;
	minuteHand.x+=addX?1:0;
	minuteHand.y+=!addX?1:0;
	graphics_draw_line(ctx, center, minuteHand);
	
	center.x-=addX?1:0;
	center.y-=!addX?1:0;
	
	addX = (tick_time->tm_hour >= 4 && tick_time->tm_hour <= 8) || tick_time->tm_hour < 2 || tick_time->tm_hour > 10;
	center.x+=addX?0:0;
	center.y+=!addX?0:0;
	hourHand.x+=addX?0:0;
	hourHand.y+=!addX?0:0;
	graphics_draw_line(ctx, center, hourHand);

}

void anim_stopped_handler(Animation *animation, bool finished, void *context) {
	
   // Schedule the reverse animation, unless the app is exiting
  if (finished) {
      layer_set_hidden(bitmap_layer_get_layer(background_layer), true);
	  layer_set_hidden(layer, false);
  }
}

void animate_text() {
  GRect start_frame = GRect(144, 79, WINDOW_WIDTH, TEXTBOX_HEIGHT);
  GRect finish_frame =  GRect(-1500, 79, TEXTBOX_WIDTH, TEXTBOX_HEIGHT);
  
  s_box_animation = property_animation_create_layer_frame(text_layer_get_layer(s_time_layer), &start_frame, &finish_frame);
  animation_set_handlers((Animation*)s_box_animation, (AnimationHandlers) {
    .stopped = anim_stopped_handler
  }, NULL);
  animation_set_duration((Animation*)s_box_animation, 20000 );
  animation_set_curve((Animation*)s_box_animation, AnimationCurveLinear);
  animation_set_delay((Animation*)s_box_animation, 0);

  animation_schedule((Animation*)s_box_animation);
}

void update_battery_state(BatteryChargeState charge_state) {
    static char battery_text[] = "100%";

    if (charge_state.is_charging) {

        snprintf(battery_text, sizeof(battery_text), "+%d%%", charge_state.charge_percent);
    } else {
        snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
        	
    } 
    charge_percent = charge_state.charge_percent;
    
    text_layer_set_text(battery_text_layer, battery_text);
	
	
	//if(charge_percent >= 30) {
    //  layer_set_hidden(text_layer_get_layer(battery_text_layer), true);
	//}
	if (charge_state.is_charging) {
    layer_set_hidden(text_layer_get_layer(battery_text_layer), false);
	}      	
} 

void handle_bluetooth(bool connected) {
	
    if (connected) {

	// Remove Inverter layer
    layer_remove_from_parent(inverter_layer_get_layer(inverter_layer_bt));
    inverter_layer_destroy(inverter_layer_bt);
    inverter_layer_bt = NULL;
    
    } else {
		
	// Add inverter layer
    Layer *window_layer = window_get_root_layer(s_window);

    inverter_layer_bt = inverter_layer_create(GRect(0, 0, 144, 168));
    layer_add_child(window_layer, inverter_layer_get_layer(inverter_layer_bt));

   }
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {

	//redraw every tick
	layer_mark_dirty(layer);
	
  static char s_time_text[] = "Wednesday 00 September  00:00pm";
  char *time_format;

    if (clock_is_24h_style()) {
        time_format = "%A %d %B  %R";	
    } else {
        time_format = "%A %d %B  %l:%M%P";
    }

    strftime(s_time_text, sizeof(s_time_text), time_format, tick_time);

	text_layer_set_text(s_time_layer, s_time_text);		

}

static void main_window_load(Window *window) {

	//create layer
	layer = layer_create(GRect(0,0,144,168));
	layer_set_update_proc(layer, update_layer);
	layer_add_child(window_get_root_layer(window), layer);	

	//load background
	background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
	
	
  background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACK);
  background_layer = bitmap_layer_create(GRect(0, 82, 144, 44));
  bitmap_layer_set_bitmap(background_layer, background_image);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(background_layer));
  layer_set_hidden(bitmap_layer_get_layer(background_layer), true);
	
  custom_font = fonts_load_custom_font( resource_get_handle( RESOURCE_ID_FONT_CUSTOM_33 ) );
  custom_font1 = fonts_load_custom_font( resource_get_handle( RESOURCE_ID_FONT_CUSTOM_16 ) );

  s_time_layer = text_layer_create(GRect(0, 0, 720, 40));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, custom_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
	
  battery_text_layer = text_layer_create(GRect(0, 1, 144, 20));
  text_layer_set_text_color(battery_text_layer, GColorWhite);
  text_layer_set_background_color(battery_text_layer, GColorClear);
  text_layer_set_text_alignment(battery_text_layer, GTextAlignmentCenter);
  text_layer_set_font(battery_text_layer, custom_font1);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(battery_text_layer));	
  
  accel_tap_service_subscribe(&accel_tap_handler);

  bluetooth_connection_service_subscribe(&handle_bluetooth);
  battery_state_service_subscribe(update_battery_state);
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
  handle_bluetooth(bluetooth_connection_service_peek());

}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy( battery_text_layer );
  animation_unschedule_all();

  layer_remove_from_parent(bitmap_layer_get_layer(background_layer));
  bitmap_layer_destroy(background_layer);
  gbitmap_destroy(background_image);
  background_image = NULL;
}

static void init() {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_window, true);
	
  // update the battery on launch
  update_battery_state(battery_state_service_peek());
	
}

static void deinit() {
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();

  accel_tap_service_unsubscribe();

  fonts_unload_custom_font( custom_font );
  fonts_unload_custom_font( custom_font1 );

  window_destroy(s_window);

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
