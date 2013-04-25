/*
%%%%%%%%%%%%%%%%%%%%%
Pebble Photo Watch Face
Copyright (C) 2013 Corey Klass

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
%%%%%%%%%%%%%%%%%%%%%

- Look for the tag #CK for where to modify the watchface for your own custom images

*/

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include <math.h>


#define MY_UUID { 0xB5, 0x1F, 0x2C, 0xEC, 0x0E, 0xDE, 0x40, 0xA0, 0x85, 0x16, 0x5F, 0x6E, 0x3C, 0x9B, 0x13, 0xDB }
PBL_APP_INFO(MY_UUID,
             "Photo Watch", "CK",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

// #CK - Set image count here
#define IMAGE_COUNT 7

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168

Window window;
int _currentImageID;
TextLayer _currentDateLayer;
TextLayer _currentTimeLayer;
TextLayer _currentDayLayer;
Layer _imageLayer;
Layer _textBackgroundLayer;
BmpContainer _imageContainer[2];
int _imageContainerIndex;
PropertyAnimation _imageAnimation;


void set_container_image(BmpContainer *bmp_container, const int resource_id) {
  layer_remove_from_parent(&bmp_container->layer.layer);
  bmp_deinit_container(bmp_container);

  bmp_init_container(resource_id, bmp_container);

  GRect frame = layer_get_frame(&bmp_container->layer.layer);
  frame.origin.x = 0;
  frame.origin.y = 0;

  // if the image isn't wide enough
  if (bmp_container->bmp.bounds.size.w < _imageLayer.frame.size.w)
    frame.origin.x += round((_imageLayer.frame.size.w - bmp_container->bmp.bounds.size.w) / 2);

  // if the image is too tall
  if (bmp_container->bmp.bounds.size.h > _imageLayer.frame.size.h)
    frame.size.h = _imageLayer.frame.size.h;
  
  layer_set_frame(&bmp_container->layer.layer, frame);
}



int get_image_id(int hours, int minutes) {
  int someValue = ((hours * 100) + minutes) % IMAGE_COUNT;

  int imageResourceID = RESOURCE_ID_IMAGE_1;
  
  switch (someValue) {
    // #CK - Set image resources here
    case 0: imageResourceID = RESOURCE_ID_IMAGE_1; break;
    case 1: imageResourceID = RESOURCE_ID_IMAGE_2; break;
    case 2: imageResourceID = RESOURCE_ID_IMAGE_3; break;
    case 3: imageResourceID = RESOURCE_ID_IMAGE_4; break;
    case 4: imageResourceID = RESOURCE_ID_IMAGE_5; break;
    case 5: imageResourceID = RESOURCE_ID_IMAGE_6; break;
    case 6: imageResourceID = RESOURCE_ID_IMAGE_7; break;
    }
  
  return imageResourceID;
  }
  
  
  



void imageLayer_update_callback(Layer *me, GContext* ctx ) {
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);

  graphics_fill_rect(ctx, GRect(0, 0, me->bounds.size.w, me->bounds.size.h), 0, GCornerNone);
}




void textBackgroundLayer_update_callback(Layer *me, GContext* ctx ) {
  graphics_context_set_stroke_color(ctx, GColorBlack); 
  graphics_context_set_fill_color(ctx, GColorBlack);

  graphics_fill_rect(ctx, GRect(0, 0, me->bounds.size.w, me->bounds.size.h), 0, GCornerNone);
}




// image animation finished handler
void image_animate_out_did_stop(struct Animation *animation, bool finished, void *context)
{
  // deinit the image container
  layer_remove_from_parent(&_imageContainer[_imageContainerIndex].layer.layer);
  bmp_deinit_container(&_imageContainer[_imageContainerIndex]);

  // reset the current index
  _imageContainerIndex = (_imageContainerIndex == 1 ? 0 : 1);
}



// will the image update?
int will_image_update(PblTm *tickTime) {
  int hours = tickTime->tm_hour;
  int minutes = tickTime->tm_min;  
  int nextImageID = get_image_id(hours, minutes);
  int returnVal = (nextImageID == _currentImageID ? 0 : 1);
  
  return returnVal;
}


// updates the image
void update_image(PblTm *tickTime) {
  int willImageUpdate = will_image_update(tickTime);
  
  if (willImageUpdate == 1) {
    int hours = tickTime->tm_hour;
    int minutes = tickTime->tm_min;
    
    int animateFlag = (_currentImageID == -1 ? 0 : 1);
    
    // get the new image ID
    _currentImageID = get_image_id(hours, minutes);
    
    // if there's an existing image
    if (animateFlag == 1) {
      int oldImageContainerIndex = _imageContainerIndex;
      int newImageContainerIndex = (_imageContainerIndex == 1 ? 0 : 1);
    
      // set the new container image and put it below the current one
      set_container_image(&_imageContainer[newImageContainerIndex], _currentImageID);
      layer_insert_below_sibling(&_imageContainer[newImageContainerIndex].layer.layer, &_imageContainer[oldImageContainerIndex].layer.layer);

      // animate out the existing image
      GRect rect = layer_get_frame(&_imageContainer[oldImageContainerIndex].layer.layer);
      rect.origin.x -= SCREEN_WIDTH;
  
      property_animation_init_layer_frame(&_imageAnimation, &_imageContainer[oldImageContainerIndex].layer.layer, NULL, &rect);
      animation_set_duration(&_imageAnimation.animation, 400);
      animation_set_curve(&_imageAnimation.animation, AnimationCurveEaseOut);
      animation_set_handlers(&_imageAnimation.animation, (AnimationHandlers) {
        .stopped = (AnimationStoppedHandler)image_animate_out_did_stop
      }, &_imageContainer[oldImageContainerIndex].layer.layer);
  
      animation_schedule(&_imageAnimation.animation);
    }
      
    // if there's no existing image
    else {
      // set the temp container image and put it below the current one
      set_container_image(&_imageContainer[_imageContainerIndex], _currentImageID);
      layer_add_child(&_imageLayer, &_imageContainer[_imageContainerIndex].layer.layer);
    } // else
  } // if (willImageUpdate == 1)
}


// updates the date
void update_date_text(PblTm *tickTime) {
  //
  // set the date - only changing it when the day changes
  // format strings here: http://www.gnu.org/software/emacs/manual/html_node/elisp/Time-Parsing.html
  //

  static char dateText[] = "XXX 00";
  static char dayText[] = "XXX";
  static int lastShownDate = -1;

  int theDay = tickTime->tm_yday;

  if (theDay != lastShownDate) {
    lastShownDate = theDay;

    string_format_time(dateText, sizeof(dateText), "%b %e", tickTime );
    text_layer_set_text(&_currentDateLayer, dateText);

    string_format_time(dayText, sizeof(dayText), "%a", tickTime);
    text_layer_set_text(&_currentDayLayer, dayText);
  }
}


// update the time
void update_time_text(PblTm *tickTime) {
  static char timeText[] = "00:00 XX";
  const char *timeFormat = clock_is_24h_style() ? "%R   " : "%I:%M %p";
  string_format_time(timeText, sizeof(timeText), timeFormat, tickTime);
  text_layer_set_text(&_currentTimeLayer, timeText);
}






// updates the entire display
void update_entire_display(PblTm *tickTime) {
  // update the image based on the time
  update_image(tickTime);

  // update the date based on the time
  update_date_text(tickTime);
  
  // update the time based on the time
  update_time_text(tickTime);
}





/* %%%%%%%%%%%%%%%%
   SYSTEM FUNCTIONS
   %%%%%%%%%%%%%%%% */


void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "Photo Watch");
  window_stack_push(&window, true /* Animated */);

  resource_init_current_app(&CK_PHOTO_WATCH);
  
  _currentImageID = -1;
  _imageContainerIndex = 0;
  
  int layerY = 0;
  int dateTimeDisplayHeight = 42;
  
  // create an empty image layer and add it
  int imageHeight = SCREEN_HEIGHT - dateTimeDisplayHeight;
  layer_init(&_imageLayer, GRect(0, layerY, SCREEN_WIDTH, imageHeight) );
  _imageLayer.update_proc = &imageLayer_update_callback;
  layer_add_child( &window.layer, &_imageLayer);
  
  layerY += imageHeight;
  
  int textTopY = layerY;
  
  // create the text background layer
  // - don't update layerY
  layer_init(&_textBackgroundLayer, GRect(0, layerY, SCREEN_WIDTH, dateTimeDisplayHeight));
  _textBackgroundLayer.update_proc = &textBackgroundLayer_update_callback;
  layer_add_child( &window.layer, &_textBackgroundLayer);

  // create the time layer
  GFont timeFont = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  int timeTopPadding = 2;
  int timeWidth = SCREEN_WIDTH * 0.65;
  int timeHeight = dateTimeDisplayHeight;
  int timeY = textTopY + timeTopPadding;

  text_layer_init(&_currentTimeLayer, GRect(0, timeY, timeWidth, timeHeight));
  text_layer_set_text_color(&_currentTimeLayer, GColorWhite);
  text_layer_set_text_alignment(&_currentTimeLayer, GTextAlignmentCenter);
  text_layer_set_background_color(&_currentTimeLayer, GColorBlack);
  text_layer_set_font(&_currentTimeLayer, timeFont );
  layer_add_child(&window.layer, &_currentTimeLayer.layer);
  
  layerY = timeY + timeHeight;

  // create the date layer
  // - don't update layerY
  // - this comes after the time layer so it goes above the time layer
  int dateRightPadding = 2;
  int dateWidth = SCREEN_WIDTH - timeWidth;
  int dateHeight = 23;
  int dateX = SCREEN_WIDTH - dateWidth - dateRightPadding;
  int dateY = SCREEN_HEIGHT - dateHeight;
  
  GFont dateFont = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  text_layer_init(&_currentDateLayer, GRect(dateX, dateY, dateWidth, dateHeight));
  text_layer_set_text_color(&_currentDateLayer, GColorWhite);
  text_layer_set_text_alignment(&_currentDateLayer, GTextAlignmentRight);
  text_layer_set_background_color(&_currentDateLayer, GColorBlack);
  text_layer_set_font(&_currentDateLayer, dateFont );
  layer_add_child(&window.layer, &_currentDateLayer.layer);


  // create the current day layer
  // - don't update layerY
  // - this comes after the time layer so it goes above the time layer
  int dayRightPadding = 2;
  int dayWidth = SCREEN_WIDTH - timeWidth;
  int dayHeight = 23;
  int dayX = SCREEN_WIDTH - dayWidth - dayRightPadding;
  int dayY = textTopY;
  
  GFont dayFont = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  text_layer_init(&_currentDayLayer, GRect(dayX, dayY, dayWidth, dayHeight));
  text_layer_set_text_color(&_currentDayLayer, GColorWhite);
  text_layer_set_text_alignment(&_currentDayLayer, GTextAlignmentRight);
  text_layer_set_background_color(&_currentDayLayer, GColorBlack);
  text_layer_set_font(&_currentDayLayer, dayFont );
  layer_add_child(&window.layer, &_currentDayLayer.layer);
  
  
  // retrieve the time and update the display
  PblTm tickTime;
  get_time(&tickTime);

  update_image(&tickTime);
  update_date_text(&tickTime);
  update_time_text(&tickTime);
}



void handle_deinit(AppContextRef ctx) {
  (void)ctx;

  // Note: Failure to de-init this here will result in instability and
  //       unable to allocate memory errors.

  layer_remove_from_parent(&_imageContainer[_imageContainerIndex].layer.layer);
  bmp_deinit_container(&_imageContainer[_imageContainerIndex]);
  
}


void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t)
{
  (void)ctx;

  PblTm *tickTime = t->tick_time;
  
  // updates the entire display
  update_entire_display(tickTime);
}



void pbl_main(void *params)
{
  PebbleAppHandlers handlers =
  {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,
    .tick_info =
    {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
