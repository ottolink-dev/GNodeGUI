/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#pragma once

#include "gnodegui/node_proxy.hpp"

namespace gngui
{

/**
 * The `GraphicsNodeGeometry` class provides the geometry specifications for
 * node layout, such as caption positioning, widget placement, and dimensions
 * for key elements like ports and settings. It allows customization of node
 * dimensions and facilitates the layout process for graphical nodes.
 */
class GraphicsNodeGeometry
{
public:
  GraphicsNodeGeometry() = default;
  GraphicsNodeGeometry(NodeProxy *p_node_proxy,
                       QSizeF     widget_size = QSizeF(0.f, 0.f),
                       bool       footer = false); // a line under the node

  QSizeF  caption_size;
  QPointF caption_pos;
  QPointF widget_pos;
  QRectF  body_rect;
  QRectF  header_rect;
  QRectF  comment_rect;
  QRectF  footer_rect;
  int     full_width;
  int     full_height;

  std::vector<QRectF> port_label_rects;
  std::vector<QRectF> port_rects;

private:
  void compute_base_metrics(QFontMetrics &fm);

  void compute_body_and_header();
  void compute_caption(const QFontMetrics &fm);
  void compute_comment_height(const QFontMetrics &fm, const std::string &comment);
  void compute_full_dimensions(const QSizeF &widget_size);
  void compute_node_width(const QSizeF &widget_size);
  void compute_ports(const QFontMetrics &fm);
  void compute_widget_position();

  NodeProxy *p_node_proxy; // Pointer to the associated node proxy

  float line_height;
  float margin;
  float header_gap;
  float node_width;
  float comment_height;
  float footer_height = 0.f;

  float ports_end_y;
};

} // namespace gngui
