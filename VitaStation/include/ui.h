#ifndef VITA_UI_H
#define VITA_UI_H

#include "library.h"
#include "texture.h"
#include "catalog.h"
#include "plugins.h"
#define UI_LOCAL_SETTINGS_COUNT 19
#define UI_TAB_WIDTH 86
#include <stdbool.h>

typedef enum {
    UI_STATE_HOME = 0,
    UI_STATE_GAME_DETAILS = 1,
    UI_STATE_QUICK_MENU = 2,
    UI_STATE_SETTINGS = 3, UI_STATE_STORE = 4, UI_STATE_LOADING = 5, UI_STATE_SPLASH = 6, UI_STATE_CATALOG_DETAILS = 7,
    UI_STATE_POWER=8, UI_STATE_PLUGINS=9, UI_STATE_SYSTEM_INFO=10, UI_STATE_UNINSTALL=11, UI_STATE_GAME_SUPPORT=12,UI_STATE_REMOVE_GAME=13,UI_STATE_SYSTEM_APPS=14,UI_STATE_PLUGIN_CATALOG=15,UI_STATE_PERSONALIZATION=16,UI_STATE_ASSET_PICKER=17
} UIState;

typedef struct {
    StoreCatalog *catalog;
    size_t catalog_index;
    CatalogType catalog_type;
    bool catalog_tabs,catalog_load_requested;
    CatalogLink homebrew_links[24];
    size_t homebrew_link_count,homebrew_link_index;
    bool homebrew_info_mode;
    unsigned homebrew_text_scroll;
    int brightness_request;
    char plugins_message[256];
    bool autoboot_enabled;
    bool ps_routing_enabled;
    bool auto_dlcs_enabled;
    int personal_index,asset_index,asset_kind;
    char wallpaper_path[256],font_path[256],lock_path[256];
    char native_ids[MAX_APPS][10];size_t native_count;int native_index,online_plugin_index;
    char remove_game_id[32],remove_game_title[128];
    int game_option,game_result;
    bool uninstall_restore;
    int uninstall_scroll;size_t uninstall_count;char uninstall_lines[48][128];
    int power_index,plugin_index;
    UIState power_return_state;
    size_t plugin_count;
    PluginEntry plugin_entries[96];
    int information_page;
    size_t information_count;
    char information_lines[6][128];
    float information_poll;
    char store_id[32];
    int store_index;
    int store_action_index;
    bool rescan_requested;
    bool library_loading;
    int system_volume;
    bool volume_error_logged;
    float volume_timer,volume_poll;
    float splash_time;
    float updater_activity;
    float audio_preview_pause;
    UIState state;
    AppSection current_section; // library categories and the configured Store
    int selected_card_index;
    int details_button_index; // 0 = Play, 1 = Favorite
    float scroll_offset;
    float target_scroll_offset;

    // Dynamic background transition
    Texture *current_bg;
    Texture *target_bg;
    float bg_crossfade_alpha; // 0.0 to 1.0

    // Animation scale for cards
    float card_scales[MAX_APPS];

    // Quick Menu & Settings state
    int quick_menu_index;
    int settings_index;
    int settings_group,settings_root_index;
    bool exit_requested;
    char pending_launch_id[32];
    int environment_theme,text_theme;
    bool high_contrast;

    // Time & Battery status info
    char clock_str[16];
    int battery_percent;

    // Toast/OSD message
    char toast_msg[128];
    float toast_timer;

    bool sound_enabled;
    bool music_enabled;
    bool animations_enabled;
} UIContext;

void ui_init(UIContext *ui);
void ui_observe_volume(UIContext *ui,int level,bool key_event);
void ui_open_store_picker(UIContext *ui,Library *lib);
AppItem *ui_selected_app(UIContext *ui,Library *lib);
void ui_update(UIContext *ui, Library *lib, TextureCache *cache, float delta_time);
void ui_render(UIContext *ui, Library *lib, TextureCache *cache);

void ui_navigate_left(UIContext *ui, Library *lib);
void ui_navigate_right(UIContext *ui, Library *lib);
void ui_navigate_up(UIContext *ui, Library *lib);
void ui_navigate_down(UIContext *ui, Library *lib);
void ui_select_tab(UIContext *ui, AppSection section);
bool ui_handle_category_buttons(UIContext *ui, uint32_t pressed);
void ui_confirm(UIContext *ui, Library *lib);
void ui_finish_pending_launch(UIContext *ui,Library *lib);
void ui_cancel(UIContext *ui);
void ui_toggle_favorite(UIContext *ui, Library *lib);
void ui_open_options(UIContext *ui);
void ui_open_power_menu(UIContext *ui);
void ui_open_quick_menu(UIContext *ui);
void ui_show_toast(UIContext *ui, const char *msg);
void ui_handle_touch(UIContext *ui, Library *lib, int tx, int ty);

#endif // VITA_UI_H
