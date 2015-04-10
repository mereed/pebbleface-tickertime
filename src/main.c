#include <pebble.h>

static Window *s_window;
static TextLayer *s_time_layer;
static TextLayer *static_time_layer;

static GFont *custom_font;
static GFont *custom_font1;
static GFont *custom_font2;

int charge_percent = 0;

TextLayer *battery_text_layer;
InverterLayer *inverter_layer_bt = NULL;

static GBitmap *background_image;
static BitmapLayer *background_layer;

#define WINDOW_HEIGHT 168
#define WINDOW_WIDTH 144  
#define TEXTBOX_HEIGHT 44
#define TEXTBOX_WIDTH 2000
	


// animates layer by number of pixels
PropertyAnimation *s_box_animation;

// prototype so anim_stopped_handler can compile (implementation below)
void animate_text();

void anim_stopped_handler(Animation *animation, bool finished, void *context) {
	
   // Schedule the reverse animation, unless the app is exiting
  if (finished) {
    animate_text();
  }
}

void animate_text() {
  GRect start_frame = GRect(0, 79, WINDOW_WIDTH, TEXTBOX_HEIGHT);
  GRect finish_frame =  GRect(-1500, 79, TEXTBOX_WIDTH, TEXTBOX_HEIGHT);
  
  s_box_animation = property_animation_create_layer_frame(text_layer_get_layer(s_time_layer), &start_frame, &finish_frame);
  animation_set_handlers((Animation*)s_box_animation, (AnimationHandlers) {
    .stopped = anim_stopped_handler
  }, NULL);
  animation_set_duration((Animation*)s_box_animation, 30000 );
  animation_set_curve((Animation*)s_box_animation, AnimationCurveLinear);
  animation_set_delay((Animation*)s_box_animation, 1000);

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

  static char s_time_text[] = "Wednesday 00 September  00:00pm";
  static char s_time_text1[] = "00:00pm";
  char *time_format;
  char *time_format1;

    if (clock_is_24h_style()) {
        time_format = "%A %d %B  %R";	
        time_format1 = "%R";	
    } else {
        time_format = "%A %d %B  %l:%M%P";
        time_format1 = "%l:%M%P";
    }

    strftime(s_time_text, sizeof(s_time_text), time_format, tick_time);
    strftime(s_time_text1, sizeof(s_time_text1), time_format1, tick_time);

	text_layer_set_text(s_time_layer, s_time_text);	
	text_layer_set_text(static_time_layer, s_time_text1);
	
	  animate_text();

}

static void main_window_load(Window *window) {

  background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACK);
  background_layer = bitmap_layer_create(GRect(0, 82, 144, 44));
  bitmap_layer_set_bitmap(background_layer, background_image);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(background_layer));
	
  custom_font = fonts_load_custom_font( resource_get_handle( RESOURCE_ID_FONT_CUSTOM_34 ) );
  custom_font1 = fonts_load_custom_font( resource_get_handle( RESOURCE_ID_FONT_CUSTOM_18 ) );
  custom_font2 = fonts_load_custom_font( resource_get_handle( RESOURCE_ID_FONT_CUSTOM_14 ) );

  s_time_layer = text_layer_create(GRect(0, 0, 2000, 40));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, custom_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
	
  static_time_layer = text_layer_create(GRect(0, 0, 144, 40));
  text_layer_set_text_color(static_time_layer, GColorWhite);
  text_layer_set_background_color(static_time_layer, GColorClear);
  text_layer_set_text_alignment(static_time_layer, GTextAlignmentCenter);
  text_layer_set_font(static_time_layer, custom_font1);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(static_time_layer));
	
  battery_text_layer = text_layer_create(GRect(0, 25, 144, 20));
  text_layer_set_text_color(battery_text_layer, GColorWhite);
  text_layer_set_background_color(battery_text_layer, GColorClear);
  text_layer_set_text_alignment(battery_text_layer, GTextAlignmentCenter);
  text_layer_set_font(battery_text_layer, custom_font2);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(battery_text_layer));	
  
	
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

  fonts_unload_custom_font( custom_font );
  fonts_unload_custom_font( custom_font1 );
  fonts_unload_custom_font( custom_font2 );

  window_destroy(s_window);

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
