/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <QFontMetrics>

#include "gnodegui/graphics_node_geometry.hpp"
#include "gnodegui/logger.hpp"
#include "gnodegui/style.hpp"

namespace gngui
{

GraphicsNodeGeometry::GraphicsNodeGeometry(NodeProxy *p_node_proxy,
                                           QSizeF     widget_size,
                                           bool       footer)
    : p_node_proxy(p_node_proxy)
{
  if (!p_node_proxy)
  {
    Logger::log()->error(
        "GraphicsNodeGeometry::GraphicsNodeGeometry: p_node_proxy is nullptr");
    return;
  }

  // base increment
  QFont        font;
  QFontMetrics fm(font);

  // in this order...
  this->compute_base_metrics(fm);
  this->footer_height = footer ? this->line_height : 0.f;
  this->compute_node_width(widget_size);
  this->compute_caption(fm);
  this->compute_comment_height(fm, this->p_node_proxy->get_comment());
  this->compute_full_dimensions(widget_size);
  this->compute_body_and_header();
  this->compute_ports(fm);
  this->compute_widget_position();
}

//

void GraphicsNodeGeometry::compute_base_metrics(QFontMetrics &fm)
{
  this->line_height = GN_STYLE->node.vertical_stretching * fm.height();
  this->margin = 2.f * GN_STYLE->node.port_radius;
  this->header_gap = GN_STYLE->node.header_height_scale * this->line_height;
}

void GraphicsNodeGeometry::compute_body_and_header()
{
  float body_top = this->caption_pos.y() + GN_STYLE->node.padding;
  this->body_rect = QRectF(this->margin,
                           body_top,
                           this->node_width,
                           this->full_height - body_top - this->comment_height -
                               this->footer_height);

  this->header_rect = this->body_rect;
  this->header_rect.setHeight(this->header_gap);

  this->comment_rect = QRectF(this->body_rect.bottomLeft(),
                              QSizeF(this->node_width, this->comment_height));

  this->footer_rect = QRectF(this->comment_rect.bottomLeft(),
                             QSizeF(this->node_width, this->footer_height));
}

void GraphicsNodeGeometry::compute_caption(const QFontMetrics &fm)
{
  this->caption_size = fm.size(Qt::TextSingleLine,
                               this->p_node_proxy->get_caption().c_str());
  this->caption_pos = QPointF(this->margin + GN_STYLE->node.padding, this->line_height);
}

void GraphicsNodeGeometry::compute_comment_height(const QFontMetrics &fm,
                                                  const std::string  &comment)
{
  // compute wrapped comment text height and store it
  if (comment.empty())
  {
    this->comment_height = 0.f;
    return;
  }

  const int max_width = int(this->node_width - 2.f * GN_STYLE->node.padding);

  QRect rect = fm.boundingRect(0,
                               0,
                               max_width,
                               10000,
                               Qt::TextWordWrap,
                               QString::fromStdString(comment));

  this->comment_height = rect.height();
}

void GraphicsNodeGeometry::compute_full_dimensions(const QSizeF &widget_size)
{
  float min_width_caption = this->caption_size.width() + 2.f * GN_STYLE->node.padding;
  this->full_width = std::max(min_width_caption, this->node_width) + 2.f * this->margin;

  this->full_height = this->line_height * (0.5f + this->p_node_proxy->get_nports()) +
                      this->header_gap + this->comment_height + this->footer_height +
                      2.f * this->margin;

  if (widget_size.height() > 0)
  {
    this->full_height += widget_size.height() +
                         2.f * GN_STYLE->node.padding_widget_height;
  }
}

void GraphicsNodeGeometry::compute_node_width(const QSizeF &widget_size)
{
  float min_from_widget = widget_size.width() + 2.f * GN_STYLE->node.padding_widget_width;
  this->node_width = std::max(GN_STYLE->node.width, (float)min_from_widget);
}

void GraphicsNodeGeometry::compute_ports(const QFontMetrics &fm)
{
  float y = this->header_rect.bottom() + GN_STYLE->node.padding;
  float diameter = 2.f * GN_STYLE->node.port_radius;
  float label_x = this->margin + 2.f * GN_STYLE->node.padding;
  float label_w = this->node_width - 4.f * GN_STYLE->node.padding;

  for (int i = 0; i < this->p_node_proxy->get_nports(); i++)
  {
    this->port_label_rects.emplace_back(QRectF(label_x, y, label_w, this->line_height));

    float cy = y + 0.5f * fm.height() - GN_STYLE->node.port_radius;

    float cx = (this->p_node_proxy->get_port_type(i) == PortType::IN)
                   ? this->margin - GN_STYLE->node.port_radius
                   : this->margin + this->node_width - GN_STYLE->node.port_radius;

    this->port_rects.emplace_back(QRectF(cx, cy, diameter, diameter));

    y += this->line_height;
  }

  this->ports_end_y = y;
}

void GraphicsNodeGeometry::compute_widget_position()
{
  this->widget_pos = QPointF(this->margin + GN_STYLE->node.padding_widget_width,
                             this->ports_end_y + GN_STYLE->node.padding_widget_height);
}

} // namespace gngui
