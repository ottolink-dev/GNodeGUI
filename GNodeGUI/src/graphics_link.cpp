/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <QGraphicsScene>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include "gnodegui/graphics_link.hpp"
#include "gnodegui/logger.hpp"
#include "gnodegui/style.hpp"
#include "gnodegui/utils.hpp"

namespace gngui
{

GraphicsLink::GraphicsLink(QColor color, LinkType link_type, QGraphicsItem *parent)
    : QGraphicsPathItem(parent), color(color), link_type(link_type)
{
  // item flags

  this->setFlag(QGraphicsItem::ItemIsSelectable, true);
  this->setFlag(QGraphicsItem::ItemIsMovable, false);
  this->setAcceptHoverEvents(true);

  if (this->color == QColor(0, 0, 0, 0))
    this->color = GN_STYLE->link.color_default;

  this->setPen(QPen(this->color, GN_STYLE->link.pen_width));
  this->setZValue(-1);
}

GraphicsLink::~GraphicsLink()
{
  if (is_valid(this->node_out))
    this->node_out->set_is_port_connected(this->port_out_index, nullptr);
  if (is_valid(this->node_in))
    this->node_in->set_is_port_connected(this->port_in_index, nullptr);
}

QRectF GraphicsLink::boundingRect() const
{
  QRectF bbox = this->path().boundingRect();
  bbox.adjust(-GN_STYLE->link.port_tip_radius,
              -GN_STYLE->link.port_tip_radius,
              GN_STYLE->link.port_tip_radius,
              GN_STYLE->link.port_tip_radius);
  return bbox;
}

void GraphicsLink::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
  this->is_link_hovered = true;
  this->setCursor(Qt::PointingHandCursor);
  this->update();

  QGraphicsPathItem::hoverEnterEvent(event);
}

void GraphicsLink::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
  this->is_link_hovered = false;
  this->setCursor(Qt::ArrowCursor);
  this->update();

  QGraphicsPathItem::hoverLeaveEvent(event);
}

nlohmann::json GraphicsLink::json_to() const
{
  nlohmann::json json;

  if (is_valid(this->node_out))
  {
    json["node_out_id"] = this->node_out->get_id();
    json["port_out_id"] = this->node_out->get_port_id(this->port_out_index);
  }

  if (is_valid(this->node_in))
  {
    json["node_in_id"] = this->node_in->get_id();
    json["port_in_id"] = this->node_in->get_port_id(this->port_in_index);
  }

  json["link_type"] = this->link_type;

  return json;
}

void GraphicsLink::paint(QPainter                       *painter,
                         const QStyleOptionGraphicsItem *option,
                         QWidget                        *widget)
{
  Q_UNUSED(option);
  Q_UNUSED(widget);

  painter->save();

  QColor pcolor = this->isSelected() ? GN_STYLE->link.color_selected : this->color;
  float  pwidth = this->is_link_hovered
                      ? GN_STYLE->link.pen_width_hovered
                      : (this->isSelected() ? GN_STYLE->link.pen_width_selected
                                            : GN_STYLE->link.pen_width);

  // link
  QPen pen(pcolor);
  pen.setWidth(pwidth);
  pen.setStyle(this->pen_style);
  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);

  // draw path
  painter->drawPath(this->path());

  // port tips
  if (this->path().elementCount() > 0)
  {
    QPointF start_point = this->path().elementAt(0);
    QPointF end_point = this->path().elementAt(this->path().elementCount() - 1);

    painter->setBrush(pcolor);
    painter->drawEllipse(start_point,
                         GN_STYLE->link.port_tip_radius,
                         GN_STYLE->link.port_tip_radius);
    painter->drawEllipse(end_point,
                         GN_STYLE->link.port_tip_radius,
                         GN_STYLE->link.port_tip_radius);
  }

  painter->restore();
}

void GraphicsLink::set_endnodes(GraphicsNode *from,
                                int           port_from_index,
                                GraphicsNode *to,
                                int           port_to_index)
{
  if (!from || !to)
  {
    Logger::log()->warn("GraphicsLink::set_endnodes: invalid nodes provided.");
    return;
  }

  // put the ports in the right order (from output to input)
  if (from->get_proxy_ref()->get_port_type(port_from_index) == PortType::OUT)
  {
    // 'from' is the output node, 'to' is the input node
    this->node_out = from;
    this->port_out_index = port_from_index;
    this->node_in = to;
    this->port_in_index = port_to_index;
  }
  else
  {
    // 'from' is the input node, 'to' is the output node
    this->node_in = from;
    this->port_in_index = port_from_index;
    this->node_out = to;
    this->port_out_index = port_to_index;
  }
}

void GraphicsLink::set_endpoints(const QPointF &start_point, const QPointF &end_point)
{
  QPainterPath new_path(start_point);

  if (this->link_type == LinkType::BROKEN_LINE)
  {
    float dx = std::copysign(20.f, end_point.x() - start_point.x());
    new_path.lineTo(QPointF(start_point.x() + dx, start_point.y()));
    new_path.lineTo(QPointF(end_point.x() - dx, end_point.y()));
    new_path.lineTo(end_point);
  }
  else if (this->link_type == LinkType::CIRCUIT)
  {
    QPointF mid_point = 0.5f * (start_point + end_point);
    new_path.lineTo(QPointF(mid_point.x(), start_point.y()));
    new_path.lineTo(QPointF(mid_point.x(), end_point.y()));
    new_path.lineTo(end_point);
  }
  else if (this->link_type == LinkType::CUBIC)
  {
    float   dx = std::abs(end_point.x() - start_point.x()) * GN_STYLE->link.curvature;
    QPointF control_point1(start_point.x() + dx, start_point.y());
    QPointF control_point2(end_point.x() - dx, end_point.y());
    new_path.cubicTo(control_point1, control_point2, end_point);
  }
  else if (this->link_type == LinkType::DEPORTED)
  {
    QPointF mid_point = QPointF(0.5f * (start_point.x() + end_point.x()),
                                start_point.y());
    new_path.lineTo(mid_point);

    float   dx = std::abs(end_point.x() - mid_point.x()) * GN_STYLE->link.curvature;
    QPointF control_point1(mid_point.x() + dx, mid_point.y());
    QPointF control_point2(end_point.x() - dx, end_point.y());
    new_path.cubicTo(control_point1, control_point2, end_point);
  }
  else if (this->link_type == LinkType::LINEAR)
  {
    new_path.lineTo(end_point);
  }
  else if (this->link_type == LinkType::QUADRATIC)
  {
    QPointF control_point((start_point.x() + end_point.x()) * 0.5f,
                          std::min(start_point.y(), end_point.y()) - 20.f);
    new_path.quadTo(control_point, end_point);
  }
  else if (this->link_type == LinkType::JAGGED)
  {
    int segments = 6; // number of zig-zag segments
    for (int i = 1; i <= segments; ++i)
    {
      float t = float(i) / segments;
      float x = start_point.x() + t * (end_point.x() - start_point.x());
      float y = start_point.y() + t * (end_point.y() - start_point.y()) +
                ((i % 2 == 0) ? -10.f : 10.f); // alternate up/down
      new_path.lineTo(QPointF(x, y));
    }
    new_path.lineTo(end_point);
  }

  this->setPath(new_path);
}

void GraphicsLink::set_link_type(const LinkType &new_link_type)
{
  this->link_type = new_link_type;
  this->update();
}

void GraphicsLink::set_pen_style(const Qt::PenStyle &new_pen_style)
{
  this->pen_style = new_pen_style;
}

void GraphicsLink::set_port_in_index(int new_index) { this->port_in_index = new_index; }

void GraphicsLink::set_port_out_index(int new_index) { this->port_out_index = new_index; }

QPainterPath GraphicsLink::shape() const
{
  QPainterPathStroker stroker;
  stroker.setWidth(40);
  QPainterPath widened_path = stroker.createStroke(path());
  return widened_path.united(path());
}

LinkType GraphicsLink::toggle_link_type()
{
  // current link type in the list
  auto it = std::find(this->link_types.begin(), this->link_types.end(), this->link_type);

  size_t index = std::distance(this->link_types.begin(), it);
  index = (index + 1) % this->link_types.size();

  this->set_link_type(this->link_types[index]);
  this->update_path();

  return this->link_types[index];
}

void GraphicsLink::update_path()
{
  // update path (only establisged ones, not the temporary one)
  if (is_valid(this->node_out) && is_valid(this->node_in))
  {
    // guards
    if (port_out_index >= (int)this->node_out->get_geometry().port_rects.size())
      return;
    if (port_in_index >= (int)this->node_in->get_geometry().port_rects.size())
      return;

    QPointF start_point = this->node_out->scenePos() + this->node_out->get_geometry()
                                                           .port_rects[port_out_index]
                                                           .center();
    QPointF end_point = this->node_in->scenePos() +
                        this->node_in->get_geometry().port_rects[port_in_index].center();

    this->set_endpoints(start_point, end_point);
  }

  this->update();
}

} // namespace gngui
