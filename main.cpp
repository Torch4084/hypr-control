#include <adwaita.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

static std::string exec_output(const std::string &cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"),
                                                pclose);
  if (!pipe)
    return "";
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
    result.pop_back();
  }
  return result;
}

static std::string get_hyprland_option(const std::string &option) {
  std::string cmd = "hyprctl getoption " + option;
  std::string output = exec_output(cmd);
  if (output.empty())
    return "";

  
  std::stringstream ss(output);
  std::string line;
  if (!std::getline(ss, line))
    return "";

  size_t colon_pos = line.find(':');
  if (colon_pos != std::string::npos && colon_pos + 2 < line.length()) {
    return line.substr(colon_pos + 2);
  }
  return "";
}

static double get_float_option(const std::string &option, double def_val) {
  std::string s = get_hyprland_option(option);
  if (s.empty())
    return def_val;
  try {
    return std::stod(s);
  } catch (...) {
    return def_val;
  }
}

static int get_int_option(const std::string &option, int def_val) {
  std::string s = get_hyprland_option(option);
  if (s.empty())
    return def_val;
  try {
    return std::stoi(s);
  } catch (...) {
    return def_val;
  }
}

static bool get_bool_option(const std::string &option, bool def_val) {
  return get_int_option(option, def_val) != 0;
}

static std::string get_string_option(const std::string &option) {
  return get_hyprland_option(option);
}

struct LayoutInfo {
  const char *code;
  const char *name;
};

static const LayoutInfo all_layouts[] = {{"us", "United States"},
                                         {"gb", "United Kingdom"},
                                         {"de", "German"},
                                         {"fr", "French"},
                                         {"es", "Spanish"},
                                         {"it", "Italian"},
                                         {"pt", "Portuguese"},
                                         {"br", "Brazilian"},
                                         {"ru", "Russian"},
                                         {"ua", "Ukrainian"},
                                         {"pl", "Polish"},
                                         {"cz", "Czech"},
                                         {"sk", "Slovak"},
                                         {"hu", "Hungarian"},
                                         {"ro", "Romanian"},
                                         {"bg", "Bulgarian"},
                                         {"hr", "Croatian"},
                                         {"si", "Slovenian"},
                                         {"rs", "Serbian"},
                                         {"mk", "Macedonian"},
                                         {"gr", "Greek"},
                                         {"tr", "Turkish"},
                                         {"il", "Hebrew"},
                                         {"ara", "Arabic"},
                                         {"ir", "Persian"},
                                         {"iq", "Iraqi"},
                                         {"sy", "Syrian"},
                                         {"eg", "Egyptian"},
                                         {"ma", "Moroccan"},
                                         {"dz", "Algerian"},
                                         {"in", "Indian"},
                                         {"jp", "Japanese"},
                                         {"kr", "Korean"},
                                         {"cn", "Chinese"},
                                         {"tw", "Taiwanese"},
                                         {"th", "Thai"},
                                         {"vn", "Vietnamese"},
                                         {"id", "Indonesian"},
                                         {"my", "Malaysian"},
                                         {"ph", "Filipino"},
                                         {"pk", "Pakistani"},
                                         {"bd", "Bangladeshi"},
                                         {"np", "Nepali"},
                                         {"lk", "Sri Lankan"},
                                         {"se", "Swedish"},
                                         {"no", "Norwegian"},
                                         {"dk", "Danish"},
                                         {"fi", "Finnish"},
                                         {"is", "Icelandic"},
                                         {"nl", "Dutch"},
                                         {"be", "Belgian"},
                                         {"ch", "Swiss"},
                                         {"at", "Austrian"},
                                         {"ca", "Canadian"},
                                         {"latam", "Latin American"},
                                         {"ie", "Irish"},
                                         {"al", "Albanian"},
                                         {"am", "Armenian"},
                                         {"az", "Azerbaijani"},
                                         {"ge", "Georgian"},
                                         {"by", "Belarusian"},
                                         {"lt", "Lithuanian"},
                                         {"lv", "Latvian"},
                                         {"ee", "Estonian"},
                                         {"mt", "Maltese"},
                                         {"me", "Montenegrin"},
                                         {"af", "Afghan"},
                                         {"kz", "Kazakh"},
                                         {"uz", "Uzbek"},
                                         {"kg", "Kyrgyz"},
                                         {"tj", "Tajik"},
                                         {"tm", "Turkmen"},
                                         {"mn", "Mongolian"},
                                         {"mm", "Myanmar"},
                                         {"kh", "Khmer"},
                                         {"la", "Lao"},
                                         {"ke", "Kenyan"},
                                         {"tz", "Tanzanian"},
                                         {"za", "South African"},
                                         {"gh", "Ghanaian"},
                                         {"ng", "Nigerian"},
                                         {"epo", "Esperanto"},
                                         {nullptr, nullptr}};

static std::vector<std::string> selected_layouts;
static GtkWidget *layouts_list_box = nullptr;
static GtkWidget *main_window = nullptr;
static GtkWidget *layout_switch_key_entry = nullptr;
static GtkWidget *keybinds_list_box = nullptr;
static int selected_modifier_index = 0;
static std::string current_layout_switch_bind = "";

static void execute_hyprctl(const std::string &command) {
  std::string full_command = "hyprctl keyword " + command;
  std::system(full_command.c_str());
}

static void execute_hyprctl_bind(const std::string &command) {
  std::string full_command = "hyprctl " + command;
  std::system(full_command.c_str());
}

static void refresh_layouts_list();

static void apply_keyboard_layouts() {
  if (selected_layouts.empty()) {
    execute_hyprctl(
        "input:kb_layout us"); 
    return;
  }
  std::string layouts_str;
  for (size_t i = 0; i < selected_layouts.size(); ++i) {
    if (i > 0)
      layouts_str += ",";
    layouts_str += selected_layouts[i];
  }
  execute_hyprctl("input:kb_layout " + layouts_str);
}

static gboolean deferred_refresh_layouts(gpointer) {
  refresh_layouts_list();
  return FALSE;
}

static void on_sensitivity_changed(GtkRange *range, gpointer) {
  double value = gtk_range_get_value(range);
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << value;
  execute_hyprctl("input:sensitivity " + oss.str());
}

static void on_accel_profile_changed(GObject *row, GParamSpec *, gpointer) {
  guint selected = adw_combo_row_get_selected(ADW_COMBO_ROW(row));
  const char *profiles[] = {"", "flat", "adaptive"};
  if (selected > 0 && selected < 3) {
    execute_hyprctl(std::string("input:accel_profile ") + profiles[selected]);
  }
}

static void on_scroll_method_changed(GObject *row, GParamSpec *, gpointer) {
  guint selected = adw_combo_row_get_selected(ADW_COMBO_ROW(row));
  const char *methods[] = {"", "2fg", "edge", "on_button_down", "no_scroll"};
  if (selected > 0 && selected < 5) {
    execute_hyprctl(std::string("input:scroll_method ") + methods[selected]);
  }
}

static void on_follow_mouse_changed(GObject *row, GParamSpec *, gpointer) {
  guint selected = adw_combo_row_get_selected(ADW_COMBO_ROW(row));
  execute_hyprctl("input:follow_mouse " + std::to_string(selected));
}

static void on_force_no_accel_changed(GObject *row, GParamSpec *, gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("input:force_no_accel ") +
                  (active ? "true" : "false"));
}

static void on_left_handed_changed(GObject *row, GParamSpec *, gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("input:left_handed ") +
                  (active ? "true" : "false"));
}

static void on_natural_scroll_changed(GObject *row, GParamSpec *, gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("input:touchpad:natural_scroll ") +
                  (active ? "true" : "false"));
}

static void on_natural_scroll_mouse_changed(GObject *row, GParamSpec *,
                                            gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("input:natural_scroll ") +
                  (active ? "true" : "false"));
}

static void on_tap_to_click_changed(GObject *row, GParamSpec *, gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("input:touchpad:tap-to-click ") +
                  (active ? "true" : "false"));
}

static void on_disable_while_typing_changed(GObject *row, GParamSpec *,
                                            gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("input:touchpad:disable_while_typing ") +
                  (active ? "true" : "false"));
}

static void on_drag_lock_changed(GObject *row, GParamSpec *, gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("input:touchpad:drag_lock ") +
                  (active ? "true" : "false"));
}

static void on_tap_and_drag_changed(GObject *row, GParamSpec *, gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("input:touchpad:tap-and-drag ") +
                  (active ? "true" : "false"));
}

static void on_middle_button_emulation_changed(GObject *row, GParamSpec *,
                                               gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("input:touchpad:middle_button_emulation ") +
                  (active ? "true" : "false"));
}

static void on_clickfinger_behavior_changed(GObject *row, GParamSpec *,
                                            gpointer) {
  guint selected = adw_combo_row_get_selected(ADW_COMBO_ROW(row));
  execute_hyprctl("input:touchpad:clickfinger_behavior " +
                  std::to_string(selected));
}

static void on_scroll_factor_changed(GtkRange *range, gpointer) {
  double value = gtk_range_get_value(range);
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << value;
  execute_hyprctl("input:touchpad:scroll_factor " + oss.str());
}

static void on_repeat_rate_changed(GtkRange *range, gpointer) {
  int value = static_cast<int>(gtk_range_get_value(range));
  execute_hyprctl("input:repeat_rate " + std::to_string(value));
}

static void on_repeat_delay_changed(GtkRange *range, gpointer) {
  int value = static_cast<int>(gtk_range_get_value(range));
  execute_hyprctl("input:repeat_delay " + std::to_string(value));
}

static void on_numlock_by_default_changed(GObject *row, GParamSpec *,
                                          gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("input:numlock_by_default ") +
                  (active ? "true" : "false"));
}

static void on_resolve_binds_by_sym_changed(GObject *row, GParamSpec *,
                                            gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("input:resolve_binds_by_sym ") +
                  (active ? "true" : "false"));
}

static void on_float_switch_override_changed(GObject *row, GParamSpec *,
                                             gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("input:float_switch_override_focus ") +
                  (active ? "2" : "0"));
}

static void on_special_fallthrough_changed(GObject *row, GParamSpec *,
                                           gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("input:special_fallthrough ") +
                  (active ? "true" : "false"));
}

static void on_touchpad_toggle_changed(GObject *row, GParamSpec *, gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("input:touchpad:enabled ") +
                  (active ? "true" : "false"));
}

static void on_touchdevice_toggle_changed(GObject *row, GParamSpec *,
                                          gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("input:touchdevice:enabled ") +
                  (active ? "true" : "false"));
}

static void on_workspace_swipe_changed(GObject *row, GParamSpec *, gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("gestures:workspace_swipe ") +
                  (active ? "true" : "false"));
}

static void on_workspace_swipe_fingers_changed(GObject *row, GParamSpec *,
                                               gpointer) {
  guint selected = adw_combo_row_get_selected(ADW_COMBO_ROW(row));
  execute_hyprctl("gestures:workspace_swipe_fingers " +
                  std::to_string(selected + 3));
}

static void on_workspace_swipe_distance_changed(GtkRange *range, gpointer) {
  int value = static_cast<int>(gtk_range_get_value(range));
  execute_hyprctl("gestures:workspace_swipe_distance " + std::to_string(value));
}

static void on_workspace_swipe_invert_changed(GObject *row, GParamSpec *,
                                              gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("gestures:workspace_swipe_invert ") +
                  (active ? "true" : "false"));
}

static void on_workspace_swipe_forever_changed(GObject *row, GParamSpec *,
                                               gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("gestures:workspace_swipe_forever ") +
                  (active ? "true" : "false"));
}

static void on_cursor_timeout_changed(GtkRange *range, gpointer) {
  int value = static_cast<int>(gtk_range_get_value(range));
  execute_hyprctl("cursor:inactive_timeout " + std::to_string(value));
}

static void on_cursor_zoom_factor_changed(GtkRange *range, gpointer) {
  double value = gtk_range_get_value(range);
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(1) << value;
  execute_hyprctl("cursor:zoom_factor " + oss.str());
}

static void on_cursor_hide_on_key_changed(GObject *row, GParamSpec *,
                                          gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("cursor:hide_on_key_press ") +
                  (active ? "true" : "false"));
}

static void on_cursor_hide_on_touch_changed(GObject *row, GParamSpec *,
                                            gpointer) {
  gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
  execute_hyprctl(std::string("cursor:hide_on_touch ") +
                  (active ? "true" : "false"));
}

static void apply_layout_switch_keybind() {
  if (!layout_switch_key_entry)
    return;

  const char *key =
      gtk_editable_get_text(GTK_EDITABLE(layout_switch_key_entry));
  if (!key || !*key)
    return;

  const char *modifiers[] = {"SUPER",    "ALT",         "CTRL",
                             "SHIFT",    "SUPER_SHIFT", "ALT_SHIFT",
                             "CTRL_ALT", "SUPER_ALT"};
  if (selected_modifier_index < 0 || selected_modifier_index >= 8)
    return;

  if (!current_layout_switch_bind.empty()) {
    execute_hyprctl_bind("keyword unbind " + current_layout_switch_bind);
  }

  std::string bind_key =
      std::string(modifiers[selected_modifier_index]) + ", " + key;
  current_layout_switch_bind = bind_key;

  execute_hyprctl_bind("keyword bind " + bind_key +
                       ", exec, hyprctl switchxkblayout all next");
}

static void on_modifier_changed(GObject *row, GParamSpec *, gpointer) {
  selected_modifier_index = adw_combo_row_get_selected(ADW_COMBO_ROW(row));
}

static void refresh_keybinds_list();

static void on_remove_keybind(GtkButton *, gpointer) {
  if (!current_layout_switch_bind.empty()) {
    execute_hyprctl_bind("keyword unbind " + current_layout_switch_bind);
    current_layout_switch_bind = "";
    refresh_keybinds_list();
  }
}

static void refresh_keybinds_list() {
  if (!keybinds_list_box)
    return;

  GtkWidget *child = gtk_widget_get_first_child(keybinds_list_box);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(keybinds_list_box), child);
    child = next;
  }

  if (!current_layout_switch_bind.empty()) {
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row),
                                  current_layout_switch_bind.c_str());
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), "Layout Switch Keybind");

    GtkWidget *remove_btn =
        gtk_button_new_from_icon_name("user-trash-symbolic");
    gtk_widget_add_css_class(remove_btn, "flat");
    gtk_widget_add_css_class(remove_btn, "circular");
    gtk_widget_set_valign(remove_btn, GTK_ALIGN_CENTER);
    g_signal_connect(remove_btn, "clicked", G_CALLBACK(on_remove_keybind),
                     nullptr);

    adw_action_row_add_suffix(ADW_ACTION_ROW(row), remove_btn);
    gtk_list_box_append(GTK_LIST_BOX(keybinds_list_box), row);
  } else {
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row),
                                  "No active keybind");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), "Add one above");
    gtk_list_box_append(GTK_LIST_BOX(keybinds_list_box), row);
  }

  gtk_widget_set_visible(keybinds_list_box, TRUE);
}

static void on_apply_keybind_clicked(GtkButton *, gpointer) {
  apply_layout_switch_keybind();
  refresh_keybinds_list();
}

static const char *get_layout_name(const std::string &code) {
  for (int i = 0; all_layouts[i].code != nullptr; ++i) {
    if (code == all_layouts[i].code) {
      return all_layouts[i].name;
    }
  }
  return code.c_str();
}

static void refresh_layouts_list();

static void on_remove_layout(GtkButton *, gpointer user_data) {
  const char *layout = static_cast<const char *>(user_data);
  auto it = std::find(selected_layouts.begin(), selected_layouts.end(), layout);
  if (it != selected_layouts.end()) {
    selected_layouts.erase(it);
    g_idle_add(deferred_refresh_layouts, nullptr);
    apply_keyboard_layouts();
  }
}

static void refresh_layouts_list() {
  GtkWidget *child = gtk_widget_get_first_child(layouts_list_box);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(layouts_list_box), child);
    child = next;
  }

  for (const auto &layout : selected_layouts) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(row, 12);
    gtk_widget_set_margin_end(row, 12);
    gtk_widget_set_margin_top(row, 8);
    gtk_widget_set_margin_bottom(row, 8);

    std::string display_text =
        std::string(get_layout_name(layout)) + " (" + layout + ")";
    GtkWidget *label = gtk_label_new(display_text.c_str());
    gtk_widget_set_hexpand(label, TRUE);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(row), label);

    GtkWidget *remove_btn =
        gtk_button_new_from_icon_name("window-close-symbolic");
    gtk_widget_add_css_class(remove_btn, "flat");
    gtk_widget_add_css_class(remove_btn, "circular");
    g_signal_connect_data(remove_btn, "clicked", G_CALLBACK(on_remove_layout),
                          g_strdup(layout.c_str()), (GClosureNotify)g_free,
                          (GConnectFlags)0);
    gtk_box_append(GTK_BOX(row), remove_btn);

    gtk_list_box_append(GTK_LIST_BOX(layouts_list_box), row);
  }

  gtk_widget_set_visible(layouts_list_box, !selected_layouts.empty());
}

static void on_layout_selected(GtkListBox *, GtkListBoxRow *row,
                               gpointer dialog) {
  if (!row)
    return;
  int index = gtk_list_box_row_get_index(row);
  if (all_layouts[index].code) {
    std::string layout = all_layouts[index].code;
    if (std::find(selected_layouts.begin(), selected_layouts.end(), layout) ==
        selected_layouts.end()) {
      selected_layouts.push_back(layout);
      refresh_layouts_list();
      apply_keyboard_layouts();
    }
  }
  adw_dialog_close(ADW_DIALOG(dialog));
}

static void on_add_layout_clicked(GtkButton *, gpointer parent_window) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Add Keyboard Layout");
  adw_dialog_set_content_width(dialog, 360);
  adw_dialog_set_content_height(dialog, 500);

  GtkWidget *toolbar_view = adw_toolbar_view_new();

  GtkWidget *header = adw_header_bar_new();
  adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar_view), header);

  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scrolled, TRUE);

  GtkWidget *list_box = gtk_list_box_new();
  gtk_widget_add_css_class(list_box, "boxed-list");
  gtk_widget_set_margin_start(list_box, 12);
  gtk_widget_set_margin_end(list_box, 12);
  gtk_widget_set_margin_top(list_box, 12);
  gtk_widget_set_margin_bottom(list_box, 12);

  for (int i = 0; all_layouts[i].code != nullptr; ++i) {
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row),
                                  all_layouts[i].name);
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), all_layouts[i].code);
    gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);

    bool already_added =
        std::find(selected_layouts.begin(), selected_layouts.end(),
                  all_layouts[i].code) != selected_layouts.end();
    if (already_added) {
      GtkWidget *check = gtk_image_new_from_icon_name("emblem-ok-symbolic");
      adw_action_row_add_suffix(ADW_ACTION_ROW(row), check);
      gtk_widget_set_sensitive(row, FALSE);
    }

    gtk_list_box_append(GTK_LIST_BOX(list_box), row);
  }

  g_signal_connect(list_box, "row-activated", G_CALLBACK(on_layout_selected),
                   dialog);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_box);
  adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar_view), scrolled);
  adw_dialog_set_child(dialog, toolbar_view);

  adw_dialog_present(dialog, GTK_WIDGET(parent_window));
}

static GtkWidget *create_mouse_page() {
  GtkWidget *page = adw_preferences_page_new();
  adw_preferences_page_set_title(ADW_PREFERENCES_PAGE(page), "Mouse");
  adw_preferences_page_set_icon_name(ADW_PREFERENCES_PAGE(page),
                                     "input-mouse-symbolic");

  GtkWidget *general_group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(general_group),
                                  "General");

  GtkWidget *sensitivity_row = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sensitivity_row),
                                "Sensitivity");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(sensitivity_row),
                              "Mouse cursor speed multiplier");
  GtkWidget *sensitivity_scale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -1.0, 1.0, 0.05);
  gtk_scale_set_draw_value(GTK_SCALE(sensitivity_scale), TRUE);
  gtk_scale_set_value_pos(GTK_SCALE(sensitivity_scale), GTK_POS_LEFT);
  gtk_range_set_value(GTK_RANGE(sensitivity_scale),
                      get_float_option("input:sensitivity", 0.0));
  gtk_widget_set_size_request(sensitivity_scale, 180, -1);
  gtk_widget_set_valign(sensitivity_scale, GTK_ALIGN_CENTER);
  g_signal_connect(sensitivity_scale, "value-changed",
                   G_CALLBACK(on_sensitivity_changed), nullptr);
  adw_action_row_add_suffix(ADW_ACTION_ROW(sensitivity_row), sensitivity_scale);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(general_group),
                            sensitivity_row);

  const char *accel_options[] = {"Default", "Flat", "Adaptive", nullptr};
  GtkStringList *accel_list = gtk_string_list_new(accel_options);
  GtkWidget *accel_row = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(accel_row),
                                "Acceleration Profile");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(accel_row),
                              "Pointer acceleration curve");
  adw_combo_row_set_model(ADW_COMBO_ROW(accel_row), G_LIST_MODEL(accel_list));

  std::string accel_val = get_string_option("input:accel_profile");
  if (accel_val == "flat")
    adw_combo_row_set_selected(ADW_COMBO_ROW(accel_row), 1);
  else if (accel_val == "adaptive")
    adw_combo_row_set_selected(ADW_COMBO_ROW(accel_row), 2);

  g_signal_connect(accel_row, "notify::selected",
                   G_CALLBACK(on_accel_profile_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(general_group), accel_row);

  GtkWidget *no_accel_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(no_accel_row),
                                "Disable Acceleration");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(no_accel_row),
                              "Force no pointer acceleration");
  adw_switch_row_set_active(ADW_SWITCH_ROW(no_accel_row),
                            get_bool_option("input:force_no_accel", false));
  g_signal_connect(no_accel_row, "notify::active",
                   G_CALLBACK(on_force_no_accel_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(general_group), no_accel_row);

  GtkWidget *left_handed_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(left_handed_row),
                                "Left Handed Mode");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(left_handed_row),
                              "Swap left and right buttons");
  adw_switch_row_set_active(ADW_SWITCH_ROW(left_handed_row),
                            get_bool_option("input:left_handed", false));
  g_signal_connect(left_handed_row, "notify::active",
                   G_CALLBACK(on_left_handed_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(general_group),
                            left_handed_row);

  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(general_group));

  GtkWidget *scroll_group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(scroll_group),
                                  "Scrolling");

  GtkWidget *natural_scroll_mouse_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(natural_scroll_mouse_row),
                                "Natural Scrolling");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(natural_scroll_mouse_row),
                              "Invert scroll direction");
  adw_switch_row_set_active(ADW_SWITCH_ROW(natural_scroll_mouse_row),
                            get_bool_option("input:natural_scroll", false));
  g_signal_connect(natural_scroll_mouse_row, "notify::active",
                   G_CALLBACK(on_natural_scroll_mouse_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(scroll_group),
                            natural_scroll_mouse_row);

  const char *scroll_options[] = {"Default",        "Two Finger", "Edge",
                                  "On Button Down", "No Scroll",  nullptr};
  GtkStringList *scroll_list = gtk_string_list_new(scroll_options);
  GtkWidget *scroll_method_row = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(scroll_method_row),
                                "Scroll Method");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(scroll_method_row),
                              "How scrolling is triggered");
  adw_combo_row_set_model(ADW_COMBO_ROW(scroll_method_row),
                          G_LIST_MODEL(scroll_list));

  std::string scroll_val = get_string_option("input:scroll_method");
  if (scroll_val == "2fg")
    adw_combo_row_set_selected(ADW_COMBO_ROW(scroll_method_row), 1);
  else if (scroll_val == "edge")
    adw_combo_row_set_selected(ADW_COMBO_ROW(scroll_method_row), 2);
  else if (scroll_val == "on_button_down")
    adw_combo_row_set_selected(ADW_COMBO_ROW(scroll_method_row), 3);
  else if (scroll_val == "no_scroll")
    adw_combo_row_set_selected(ADW_COMBO_ROW(scroll_method_row), 4);

  g_signal_connect(scroll_method_row, "notify::selected",
                   G_CALLBACK(on_scroll_method_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(scroll_group),
                            scroll_method_row);

  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(scroll_group));

  GtkWidget *focus_group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(focus_group),
                                  "Focus Behavior");

  const char *follow_options[] = {"Disabled", "Always", "Loose", "Strict",
                                  nullptr};
  GtkStringList *follow_list = gtk_string_list_new(follow_options);
  GtkWidget *follow_row = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(follow_row),
                                "Follow Mouse");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(follow_row),
                              "Window focus follows mouse cursor");
  adw_combo_row_set_model(ADW_COMBO_ROW(follow_row), G_LIST_MODEL(follow_list));

  int follow_val = get_int_option("input:follow_mouse", 1);
  if (follow_val >= 0 && follow_val <= 3)
    adw_combo_row_set_selected(ADW_COMBO_ROW(follow_row), follow_val);

  g_signal_connect(follow_row, "notify::selected",
                   G_CALLBACK(on_follow_mouse_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(focus_group), follow_row);

  GtkWidget *float_focus_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(float_focus_row),
                                "Float Switch Override Focus");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(float_focus_row),
                              "Focus floats on mouse hover");
  adw_switch_row_set_active(
      ADW_SWITCH_ROW(float_focus_row),
      get_int_option("input:float_switch_override_focus", 1) == 2);
  g_signal_connect(float_focus_row, "notify::active",
                   G_CALLBACK(on_float_switch_override_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(focus_group),
                            float_focus_row);

  GtkWidget *special_fallthrough_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(special_fallthrough_row),
                                "Special Fallthrough");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(special_fallthrough_row),
                              "Click through special workspaces");
  adw_switch_row_set_active(
      ADW_SWITCH_ROW(special_fallthrough_row),
      get_bool_option("input:special_fallthrough", false));
  g_signal_connect(special_fallthrough_row, "notify::active",
                   G_CALLBACK(on_special_fallthrough_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(focus_group),
                            special_fallthrough_row);

  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(focus_group));

  GtkWidget *cursor_group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(cursor_group),
                                  "Cursor");

  GtkWidget *cursor_timeout_row = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(cursor_timeout_row),
                                "Hide Timeout");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(cursor_timeout_row),
                              "Seconds before cursor hides (0 = never)");
  GtkWidget *timeout_scale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 60, 1);
  gtk_scale_set_draw_value(GTK_SCALE(timeout_scale), TRUE);
  gtk_scale_set_value_pos(GTK_SCALE(timeout_scale), GTK_POS_LEFT);
  gtk_range_set_value(GTK_RANGE(timeout_scale),
                      get_int_option("cursor:inactive_timeout", 0));
  gtk_widget_set_size_request(timeout_scale, 180, -1);
  gtk_widget_set_valign(timeout_scale, GTK_ALIGN_CENTER);
  g_signal_connect(timeout_scale, "value-changed",
                   G_CALLBACK(on_cursor_timeout_changed), nullptr);
  adw_action_row_add_suffix(ADW_ACTION_ROW(cursor_timeout_row), timeout_scale);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(cursor_group),
                            cursor_timeout_row);

  GtkWidget *cursor_zoom_row = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(cursor_zoom_row),
                                "Zoom Factor");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(cursor_zoom_row),
                              "Cursor size multiplier");
  GtkWidget *zoom_scale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1.0, 4.0, 0.1);
  gtk_scale_set_draw_value(GTK_SCALE(zoom_scale), TRUE);
  gtk_scale_set_value_pos(GTK_SCALE(zoom_scale), GTK_POS_LEFT);
  gtk_range_set_value(GTK_RANGE(zoom_scale),
                      get_float_option("cursor:zoom_factor", 1.0));
  gtk_widget_set_size_request(zoom_scale, 180, -1);
  gtk_widget_set_valign(zoom_scale, GTK_ALIGN_CENTER);
  g_signal_connect(zoom_scale, "value-changed",
                   G_CALLBACK(on_cursor_zoom_factor_changed), nullptr);
  adw_action_row_add_suffix(ADW_ACTION_ROW(cursor_zoom_row), zoom_scale);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(cursor_group),
                            cursor_zoom_row);

  GtkWidget *hide_on_key_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(hide_on_key_row),
                                "Hide on Key Press");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(hide_on_key_row),
                              "Hide cursor when typing");
  adw_switch_row_set_active(ADW_SWITCH_ROW(hide_on_key_row),
                            get_bool_option("cursor:hide_on_key_press", false));
  g_signal_connect(hide_on_key_row, "notify::active",
                   G_CALLBACK(on_cursor_hide_on_key_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(cursor_group),
                            hide_on_key_row);

  GtkWidget *hide_on_touch_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(hide_on_touch_row),
                                "Hide on Touch");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(hide_on_touch_row),
                              "Hide cursor when touching screen");
  adw_switch_row_set_active(ADW_SWITCH_ROW(hide_on_touch_row),
                            get_bool_option("cursor:hide_on_touch", false));
  g_signal_connect(hide_on_touch_row, "notify::active",
                   G_CALLBACK(on_cursor_hide_on_touch_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(cursor_group),
                            hide_on_touch_row);

  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(cursor_group));

  return page;
}

static GtkWidget *create_touchpad_page() {
  GtkWidget *page = adw_preferences_page_new();
  adw_preferences_page_set_title(ADW_PREFERENCES_PAGE(page), "Touchpad");
  adw_preferences_page_set_icon_name(ADW_PREFERENCES_PAGE(page),
                                     "input-touchpad-symbolic");

  GtkWidget *device_group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(device_group),
                                  "Device");

  GtkWidget *touchpad_enabled_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(touchpad_enabled_row),
                                "Touchpad Enabled");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(touchpad_enabled_row),
                              "Enable or disable touchpad");
  adw_switch_row_set_active(ADW_SWITCH_ROW(touchpad_enabled_row),
                            get_bool_option("input:touchpad:enabled", true));
  g_signal_connect(touchpad_enabled_row, "notify::active",
                   G_CALLBACK(on_touchpad_toggle_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(device_group),
                            touchpad_enabled_row);

  GtkWidget *touchscreen_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(touchscreen_row),
                                "Touchscreen Enabled");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(touchscreen_row),
                              "Enable or disable touchscreen");
  adw_switch_row_set_active(ADW_SWITCH_ROW(touchscreen_row),
                            get_bool_option("input:touchdevice:enabled", true));
  g_signal_connect(touchscreen_row, "notify::active",
                   G_CALLBACK(on_touchdevice_toggle_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(device_group),
                            touchscreen_row);

  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(device_group));

  GtkWidget *tap_group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(tap_group), "Tapping");

  GtkWidget *tap_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(tap_row), "Tap to Click");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(tap_row),
                              "Tap the touchpad to click");
  adw_switch_row_set_active(
      ADW_SWITCH_ROW(tap_row),
      get_bool_option("input:touchpad:tap-to-click", true));
  g_signal_connect(tap_row, "notify::active",
                   G_CALLBACK(on_tap_to_click_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(tap_group), tap_row);

  GtkWidget *tap_drag_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(tap_drag_row),
                                "Tap and Drag");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(tap_drag_row),
                              "Tap and hold to drag");
  adw_switch_row_set_active(
      ADW_SWITCH_ROW(tap_drag_row),
      get_bool_option("input:touchpad:tap-and-drag", true));
  g_signal_connect(tap_drag_row, "notify::active",
                   G_CALLBACK(on_tap_and_drag_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(tap_group), tap_drag_row);

  GtkWidget *drag_lock_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(drag_lock_row),
                                "Drag Lock");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(drag_lock_row),
                              "Continue drag after lifting finger");
  adw_switch_row_set_active(ADW_SWITCH_ROW(drag_lock_row),
                            get_bool_option("input:touchpad:drag_lock", false));
  g_signal_connect(drag_lock_row, "notify::active",
                   G_CALLBACK(on_drag_lock_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(tap_group), drag_lock_row);

  const char *click_options[] = {"Button Areas", "Clickfinger", nullptr};
  GtkStringList *click_list = gtk_string_list_new(click_options);
  GtkWidget *click_row = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(click_row), "Click Method");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(click_row),
                              "How right/middle click is detected");
  adw_combo_row_set_model(ADW_COMBO_ROW(click_row), G_LIST_MODEL(click_list));
  adw_combo_row_set_selected(
      ADW_COMBO_ROW(click_row),
      get_int_option("input:touchpad:clickfinger_behavior", 1));
  g_signal_connect(click_row, "notify::selected",
                   G_CALLBACK(on_clickfinger_behavior_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(tap_group), click_row);

  GtkWidget *middle_emu_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(middle_emu_row),
                                "Middle Button Emulation");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(middle_emu_row),
                              "Press left+right for middle click");
  adw_switch_row_set_active(
      ADW_SWITCH_ROW(middle_emu_row),
      get_bool_option("input:touchpad:middle_button_emulation", false));
  g_signal_connect(middle_emu_row, "notify::active",
                   G_CALLBACK(on_middle_button_emulation_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(tap_group), middle_emu_row);

  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(tap_group));

  GtkWidget *scroll_group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(scroll_group),
                                  "Scrolling");

  GtkWidget *natural_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(natural_row),
                                "Natural Scrolling");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(natural_row),
                              "Content follows finger direction");
  adw_switch_row_set_active(
      ADW_SWITCH_ROW(natural_row),
      get_bool_option("input:touchpad:natural_scroll", true));
  g_signal_connect(natural_row, "notify::active",
                   G_CALLBACK(on_natural_scroll_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(scroll_group), natural_row);

  GtkWidget *scroll_factor_row = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(scroll_factor_row),
                                "Scroll Speed");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(scroll_factor_row),
                              "Scroll distance multiplier");
  GtkWidget *scroll_scale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 3.0, 0.1);
  gtk_scale_set_draw_value(GTK_SCALE(scroll_scale), TRUE);
  gtk_scale_set_value_pos(GTK_SCALE(scroll_scale), GTK_POS_LEFT);
  gtk_range_set_value(GTK_RANGE(scroll_scale),
                      get_float_option("input:touchpad:scroll_factor", 1.0));
  gtk_widget_set_size_request(scroll_scale, 180, -1);
  gtk_widget_set_valign(scroll_scale, GTK_ALIGN_CENTER);
  g_signal_connect(scroll_scale, "value-changed",
                   G_CALLBACK(on_scroll_factor_changed), nullptr);
  adw_action_row_add_suffix(ADW_ACTION_ROW(scroll_factor_row), scroll_scale);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(scroll_group),
                            scroll_factor_row);

  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(scroll_group));

  GtkWidget *behavior_group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(behavior_group),
                                  "Behavior");

  GtkWidget *dwt_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(dwt_row),
                                "Disable While Typing");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(dwt_row),
                              "Ignore touchpad while typing");
  adw_switch_row_set_active(
      ADW_SWITCH_ROW(dwt_row),
      get_bool_option("input:touchpad:disable_while_typing", true));
  g_signal_connect(dwt_row, "notify::active",
                   G_CALLBACK(on_disable_while_typing_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(behavior_group), dwt_row);

  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(behavior_group));

  GtkWidget *gesture_group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(gesture_group),
                                  "Gestures");

  GtkWidget *swipe_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(swipe_row),
                                "Workspace Swipe");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(swipe_row),
                              "Swipe to change workspaces");
  adw_switch_row_set_active(ADW_SWITCH_ROW(swipe_row),
                            get_bool_option("gestures:workspace_swipe", true));
  g_signal_connect(swipe_row, "notify::active",
                   G_CALLBACK(on_workspace_swipe_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(gesture_group), swipe_row);

  const char *fingers_options[] = {"3 Fingers", "4 Fingers", nullptr};
  GtkStringList *fingers_list = gtk_string_list_new(fingers_options);
  GtkWidget *fingers_row = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(fingers_row),
                                "Swipe Fingers");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(fingers_row),
                              "Number of fingers for gesture");
  adw_combo_row_set_model(ADW_COMBO_ROW(fingers_row),
                          G_LIST_MODEL(fingers_list));
  adw_combo_row_set_selected(
      ADW_COMBO_ROW(fingers_row),
      get_int_option("gestures:workspace_swipe_fingers", 3) == 4 ? 1 : 0);
  g_signal_connect(fingers_row, "notify::selected",
                   G_CALLBACK(on_workspace_swipe_fingers_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(gesture_group), fingers_row);

  GtkWidget *swipe_dist_row = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(swipe_dist_row),
                                "Swipe Distance");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(swipe_dist_row),
                              "Pixels needed for workspace switch");
  GtkWidget *dist_scale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 100, 500, 10);
  gtk_scale_set_draw_value(GTK_SCALE(dist_scale), TRUE);
  gtk_scale_set_value_pos(GTK_SCALE(dist_scale), GTK_POS_LEFT);
  gtk_range_set_value(GTK_RANGE(dist_scale),
                      get_int_option("gestures:workspace_swipe_distance", 300));
  gtk_widget_set_size_request(dist_scale, 180, -1);
  gtk_widget_set_valign(dist_scale, GTK_ALIGN_CENTER);
  g_signal_connect(dist_scale, "value-changed",
                   G_CALLBACK(on_workspace_swipe_distance_changed), nullptr);
  adw_action_row_add_suffix(ADW_ACTION_ROW(swipe_dist_row), dist_scale);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(gesture_group),
                            swipe_dist_row);

  GtkWidget *swipe_invert_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(swipe_invert_row),
                                "Invert Swipe");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(swipe_invert_row),
                              "Reverse swipe direction");
  adw_switch_row_set_active(
      ADW_SWITCH_ROW(swipe_invert_row),
      get_bool_option("gestures:workspace_swipe_invert", true));
  g_signal_connect(swipe_invert_row, "notify::active",
                   G_CALLBACK(on_workspace_swipe_invert_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(gesture_group),
                            swipe_invert_row);

  GtkWidget *swipe_forever_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(swipe_forever_row),
                                "Continuous Swipe");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(swipe_forever_row),
                              "Keep swiping through all workspaces");
  adw_switch_row_set_active(
      ADW_SWITCH_ROW(swipe_forever_row),
      get_bool_option("gestures:workspace_swipe_forever", false));
  g_signal_connect(swipe_forever_row, "notify::active",
                   G_CALLBACK(on_workspace_swipe_forever_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(gesture_group),
                            swipe_forever_row);

  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(gesture_group));

  return page;
}

static void load_keybind_state() {
  current_layout_switch_bind = "";
  std::string output =
      exec_output("hyprctl binds | grep -B 4 'arg: hyprctl switchxkblayout'");
  if (output.empty())
    return;

  int modmask = 0;
  std::string key_code;
  std::stringstream ss(output);
  std::string line;

  while (std::getline(ss, line)) {
    if (line.find("modmask:") != std::string::npos) {
      try {
        modmask = std::stoi(line.substr(line.find("modmask:") + 9));
      } catch (...) {
      }
    }
    if (line.find("key:") != std::string::npos) {
      key_code = line.substr(line.find("key:") + 5);
      key_code.erase(0, key_code.find_first_not_of(" \t"));
    }
  }

  std::string mod_str;
  if (modmask == 64)
    mod_str = "SUPER";
  else if (modmask == 8)
    mod_str = "ALT";
  else if (modmask == 4)
    mod_str = "CTRL";
  else if (modmask == 1)
    mod_str = "SHIFT";
  else if (modmask == 65)
    mod_str = "SUPER_SHIFT";
  else if (modmask == 9)
    mod_str = "ALT_SHIFT";
  else if (modmask == 12)
    mod_str = "CTRL_ALT";
  else if (modmask == 72)
    mod_str = "SUPER_ALT";

  if (!mod_str.empty() && !key_code.empty()) {
    current_layout_switch_bind = mod_str + ", " + key_code;
  }
}

static GtkWidget *create_keyboard_page() {
  GtkWidget *page = adw_preferences_page_new();
  adw_preferences_page_set_title(ADW_PREFERENCES_PAGE(page), "Keyboard");
  adw_preferences_page_set_icon_name(ADW_PREFERENCES_PAGE(page),
                                     "input-keyboard-symbolic");

  
  selected_layouts.clear();
  std::string layouts_str = get_string_option("input:kb_layout");
  if (!layouts_str.empty()) {
    std::stringstream ss(layouts_str);
    std::string segment;
    while (std::getline(ss, segment, ',')) {
      segment.erase(0, segment.find_first_not_of(" \t"));
      selected_layouts.push_back(segment);
    }
  }

  
  load_keybind_state();

  GtkWidget *layout_group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(layout_group),
                                  "Layouts");

  layouts_list_box = gtk_list_box_new();
  gtk_widget_add_css_class(layouts_list_box, "boxed-list");
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(layouts_list_box),
                                  GTK_SELECTION_NONE);
  gtk_widget_set_visible(layouts_list_box, FALSE);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(layout_group),
                            layouts_list_box);
  refresh_layouts_list(); 

  GtkWidget *add_row = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(add_row), "Add Layout");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(add_row),
                              "Add a new keyboard layout");
  gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(add_row), TRUE);
  GtkWidget *add_icon = gtk_image_new_from_icon_name("list-add-symbolic");
  adw_action_row_add_suffix(ADW_ACTION_ROW(add_row), add_icon);
  g_signal_connect(add_row, "activated", G_CALLBACK(on_add_layout_clicked),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(layout_group), add_row);

  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(layout_group));

  GtkWidget *keybind_group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(keybind_group),
                                  "Layout Switching");
  adw_preferences_group_set_description(
      ADW_PREFERENCES_GROUP(keybind_group),
      "Set a keybind to cycle through layouts");

  keybinds_list_box = gtk_list_box_new();
  gtk_widget_add_css_class(keybinds_list_box, "boxed-list");
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(keybinds_list_box),
                                  GTK_SELECTION_NONE);
  gtk_widget_set_visible(keybinds_list_box, FALSE);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(keybind_group),
                            keybinds_list_box);

  const char *modifier_options[] = {"Super",    "Alt",         "Ctrl",
                                    "Shift",    "Super+Shift", "Alt+Shift",
                                    "Ctrl+Alt", "Super+Alt",   nullptr};
  GtkStringList *mod_list = gtk_string_list_new(modifier_options);
  GtkWidget *mod_row = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(mod_row), "Modifier");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(mod_row),
                              "Key modifier for shortcut");
  adw_combo_row_set_model(ADW_COMBO_ROW(mod_row), G_LIST_MODEL(mod_list));
  g_signal_connect(mod_row, "notify::selected", G_CALLBACK(on_modifier_changed),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(keybind_group), mod_row);

  GtkWidget *key_row = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(key_row), "Key");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(key_row),
                              "Key to press (e.g., Space, Tab, grave)");
  layout_switch_key_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(layout_switch_key_entry), "Space");
  gtk_widget_set_size_request(layout_switch_key_entry, 120, -1);
  gtk_widget_set_valign(layout_switch_key_entry, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix(ADW_ACTION_ROW(key_row), layout_switch_key_entry);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(keybind_group), key_row);

  GtkWidget *apply_row = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(apply_row),
                                "Apply Keybind");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(apply_row),
                              "Set the keyboard layout switch shortcut");
  gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(apply_row), TRUE);
  GtkWidget *apply_icon = gtk_image_new_from_icon_name("emblem-ok-symbolic");
  adw_action_row_add_suffix(ADW_ACTION_ROW(apply_row), apply_icon);
  g_signal_connect(apply_row, "activated", G_CALLBACK(on_apply_keybind_clicked),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(keybind_group), apply_row);

  keybinds_list_box = gtk_list_box_new();
  gtk_widget_add_css_class(keybinds_list_box, "boxed-list");
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(keybinds_list_box),
                                  GTK_SELECTION_NONE);
  refresh_keybinds_list(); 
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(keybind_group),
                            keybinds_list_box);

  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(keybind_group));

  GtkWidget *repeat_group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(repeat_group),
                                  "Key Repeat");

  GtkWidget *rate_row = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(rate_row), "Repeat Rate");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(rate_row),
                              "Keys per second when held");
  GtkWidget *rate_scale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 10, 100, 5);
  gtk_scale_set_draw_value(GTK_SCALE(rate_scale), TRUE);
  gtk_scale_set_value_pos(GTK_SCALE(rate_scale), GTK_POS_LEFT);
  gtk_range_set_value(GTK_RANGE(rate_scale),
                      get_int_option("input:repeat_rate", 25));
  gtk_widget_set_size_request(rate_scale, 180, -1);
  gtk_widget_set_valign(rate_scale, GTK_ALIGN_CENTER);
  g_signal_connect(rate_scale, "value-changed",
                   G_CALLBACK(on_repeat_rate_changed), nullptr);
  adw_action_row_add_suffix(ADW_ACTION_ROW(rate_row), rate_scale);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(repeat_group), rate_row);

  GtkWidget *delay_row = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(delay_row), "Repeat Delay");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(delay_row),
                              "Milliseconds before repeat starts");
  GtkWidget *delay_scale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 100, 1000, 50);
  gtk_scale_set_draw_value(GTK_SCALE(delay_scale), TRUE);
  gtk_scale_set_value_pos(GTK_SCALE(delay_scale), GTK_POS_LEFT);
  gtk_range_set_value(GTK_RANGE(delay_scale),
                      get_int_option("input:repeat_delay", 600));
  gtk_widget_set_size_request(delay_scale, 180, -1);
  gtk_widget_set_valign(delay_scale, GTK_ALIGN_CENTER);
  g_signal_connect(delay_scale, "value-changed",
                   G_CALLBACK(on_repeat_delay_changed), nullptr);
  adw_action_row_add_suffix(ADW_ACTION_ROW(delay_row), delay_scale);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(repeat_group), delay_row);

  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(repeat_group));

  GtkWidget *options_group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(options_group),
                                  "Options");

  GtkWidget *numlock_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(numlock_row),
                                "Numlock by Default");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(numlock_row),
                              "Enable numlock on startup");
  g_signal_connect(numlock_row, "notify::active",
                   G_CALLBACK(on_numlock_by_default_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(options_group), numlock_row);

  GtkWidget *binds_sym_row = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(binds_sym_row),
                                "Resolve Binds by Symbol");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(binds_sym_row),
                              "Use keysym instead of keycode");
  g_signal_connect(binds_sym_row, "notify::active",
                   G_CALLBACK(on_resolve_binds_by_sym_changed), nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(options_group),
                            binds_sym_row);

  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(options_group));

  return page;
}

static void on_activate(GtkApplication *app, gpointer) {
  main_window = adw_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(main_window), "Hypr Control");
  gtk_window_set_default_size(GTK_WINDOW(main_window), 600, 750);

  GtkWidget *view = adw_toolbar_view_new();

  GtkWidget *header = adw_header_bar_new();
  GtkWidget *title =
      adw_window_title_new("Hypr Control", "Input Device Settings");
  adw_header_bar_set_title_widget(ADW_HEADER_BAR(header), title);
  adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(view), header);

  GtkWidget *view_stack = adw_view_stack_new();

  GtkWidget *mouse_page = create_mouse_page();
  adw_view_stack_add_titled_with_icon(ADW_VIEW_STACK(view_stack), mouse_page,
                                      "mouse", "Mouse", "input-mouse-symbolic");

  GtkWidget *touchpad_page = create_touchpad_page();
  adw_view_stack_add_titled_with_icon(ADW_VIEW_STACK(view_stack), touchpad_page,
                                      "touchpad", "Touchpad",
                                      "input-touchpad-symbolic");

  GtkWidget *keyboard_page = create_keyboard_page();
  adw_view_stack_add_titled_with_icon(ADW_VIEW_STACK(view_stack), keyboard_page,
                                      "keyboard", "Keyboard",
                                      "input-keyboard-symbolic");

  GtkWidget *switcher = adw_view_switcher_bar_new();
  adw_view_switcher_bar_set_stack(ADW_VIEW_SWITCHER_BAR(switcher),
                                  ADW_VIEW_STACK(view_stack));
  adw_view_switcher_bar_set_reveal(ADW_VIEW_SWITCHER_BAR(switcher), TRUE);

  GtkWidget *header_switcher = adw_view_switcher_new();
  adw_view_switcher_set_stack(ADW_VIEW_SWITCHER(header_switcher),
                              ADW_VIEW_STACK(view_stack));
  adw_view_switcher_set_policy(ADW_VIEW_SWITCHER(header_switcher),
                               ADW_VIEW_SWITCHER_POLICY_WIDE);
  adw_header_bar_set_title_widget(ADW_HEADER_BAR(header), header_switcher);

  adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(view), view_stack);
  adw_toolbar_view_add_bottom_bar(ADW_TOOLBAR_VIEW(view), switcher);

  adw_application_window_set_content(ADW_APPLICATION_WINDOW(main_window), view);
  gtk_window_present(GTK_WINDOW(main_window));
}

int main(int argc, char *argv[]) {
  AdwApplication *app = adw_application_new("com.github.hyprcontrol",
                                            G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), nullptr);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
