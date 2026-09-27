/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */

/**
 * @file style.hpp
 * @author Otto Link (otto.link.bv@gmail.com)
 * @brief
 * @copyright Copyright (c) 2024 Otto Link. Distributed under the terms of the
 * GNU General Public License. See the file LICENSE for the full license.
 */
#pragma once
#include <map>

#include <QColor>
#include <QPoint>

#define GN_STYLE gngui::Style::get_style()

namespace gngui
{

class Style
{
public:
  Style() = default;

  static std::shared_ptr<Style> &get_style();

  struct Viewer
  {
    QColor color_bg = QColor(42, 42, 42, 255);
    QColor color_toolbar = Qt::lightGray;

    bool   add_toolbar = true;
    QPoint toolbar_window_pos = QPoint(10, 40);
    float  toolbar_width = 32.f;
    bool   add_viewport_icon = true;
    bool   add_new_icon = true;
    bool   add_import_icon = true;
    bool   add_load_save_icons = true;
    bool   add_group = true;

    bool disable_during_update = true;

    // zoom range (view scale); zooming out goes past zoom_min only as far as
    // needed to fit the whole graph in the view
    float zoom_min = 0.15f;
    float zoom_max = 3.f;
  } viewer;

  struct Node
  {
    float width = 128.f;
    float padding = 6.f;
    float padding_widget_width = 4.f;
    float padding_widget_height = 6.f;
    float rounding_radius = 8.f;
    float port_radius = 6.f;
    float port_radius_not_selectable = 5.f;
    float vertical_stretching = 1.3f;
    float header_height_scale = 1.2f;

    bool reload_button = true;
    bool settings_button = true;

    QColor color_bg = QColor(102, 102, 102, 255);
    QColor color_bg_light = QColor(108, 108, 108, 255);
    QColor color_border = Qt::black;
    QColor color_border_hovered = Qt::black;
    QColor color_caption = Qt::white;
    QColor color_selected = QColor(80, 250, 123, 255);
    QColor color_pinned = QColor(139, 233, 253, 255);
    QColor color_icon = Qt::lightGray;
    QColor color_comment = QColor(255, 121, 198, 255);

    QColor color_port_hovered = Qt::white; // QColor(180, 180, 180, 255);
    QColor color_port_selected = QColor(80, 250, 123, 255);

    float pen_width = 1.5;
    float pen_width_hovered = 2;
    float pen_width_selected = 2;

    QColor color_port_data_default = Qt::lightGray;
    QColor color_port_not_selectable = QColor(102, 102, 102, 255);

    std::map<std::string, QColor> color_port_data = {};
    std::map<std::string, QColor> color_category = {};
  } node;

  struct Link
  {
    float  pen_width = 1.f;
    float  pen_width_hovered = 2.f;
    float  pen_width_selected = 3.f;
    float  port_tip_radius = 2.f;
    float  curvature = 0.5f;
    QColor color_default = Qt::lightGray;
    QColor color_selected = QColor(80, 250, 123, 255);
  } link;

  struct Group
  {
    float default_width = 256.f;
    float default_height = 256.f;
    float pen_width = 1.f;
    float pen_width_hovered = 1.f;
    float pen_width_selected = 3.f;
    float rounding_radius = 16.f;

    QColor color = Qt::white;
    float  background_fill_alpha = 0.1f;
    QColor color_selected = QColor(80, 250, 123, 255);

    bool bold_caption = true;

    std::map<std::string, QColor> color_map = {{"White", Qt::white},
                                               {"Cyan", QColor(139, 233, 253)},
                                               {"Green", QColor(80, 250, 123)},
                                               {"Orange", QColor(255, 184, 108)},
                                               {"Pink", QColor(255, 121, 198)},
                                               {"Purple", QColor(189, 147, 249)},
                                               {"Red", QColor(255, 85, 85)},
                                               {"Yellow", QColor(241, 250, 140)},
                                               {"Black", Qt::black}};
  } group;

  struct Comment
  {
    float rounding_radius = 4.f;
    float width = 256.f;

    QColor color_text = Qt::lightGray;
    QColor color_bg = QColor(108, 108, 108, 255);
    float  background_fill_alpha = 0.1f;
  } comment;

private:
  // Disable copy constructor
  Style(const Style &) = delete;

  // Disable assignment operator
  Style &operator=(const Style &) = delete;

  // Static member to hold the singleton instance
  static std::shared_ptr<Style> instance;
};

QColor get_color_from_data_type(const std::string &data_type);

} // namespace gngui
