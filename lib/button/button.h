//
// Created by yplam on 13/1/2021.
//

#ifndef OT1M_APP_BUTTON_H
#define OT1M_APP_BUTTON_H
typedef void (*app_button_event_cb_t)(void);
void app_button_init(app_button_event_cb_t single_click_cb, app_button_event_cb_t double_click_cb,
                     app_button_event_cb_t long_press_cb, app_button_event_cb_t click_and_press_cb);
#endif //OT1M_APP_BUTTON_H
