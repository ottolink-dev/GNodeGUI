/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#pragma once
#include <memory>
#include <vector>

#include <QGraphicsPathItem>

#include "gnodegui/graphics_node.hpp"
#include "nlohmann/json.hpp"

namespace gngui
{

class GraphicsNode; // forward decl

enum LinkType
{
  BROKEN_LINE,
  CIRCUIT,
  CUBIC,
  DEPORTED,
  LINEAR,
  QUADRATIC,
  JAGGED
};

class GraphicsLink : public QGraphicsPathItem
{
public:
  GraphicsLink(QColor         color = QColor(0, 0, 0, 0),
               LinkType       link_type = LinkType::CUBIC,
               QGraphicsItem *parent = nullptr);
  ~GraphicsLink();

  // --- Serialization

  nlohmann::json json_to() const;

  // --- Getters

  GraphicsNode *get_node_out() { return this->node_out; }
  GraphicsNode *get_node_in() { return this->node_in; }
  int           get_port_out_index() const { return this->port_out_index; }
  int           get_port_in_index() const { return this->port_in_index; }

  // --- Node / Link Management
  void     set_endnodes(GraphicsNode *from,
                        int           port_from_index,
                        GraphicsNode *to,
                        int           port_to_index);
  void     set_endpoints(const QPointF &start_point, const QPointF &end_point);
  void     set_link_type(const LinkType &new_link_type);
  void     set_pen_style(const Qt::PenStyle &new_pen_style);
  void     set_port_in_index(int new_index);
  void     set_port_out_index(int new_index);
  LinkType toggle_link_type();
  void     update_path();

protected:
  // --- QGraphicsItem overrides

  QRectF       boundingRect() const override;
  void         hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
  void         hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
  void         paint(QPainter                       *painter,
                     const QStyleOptionGraphicsItem *option,
                     QWidget                        *widget) override;
  QPainterPath shape() const override;

private:
  // --- Members

  // visual properties
  QColor                color;
  LinkType              link_type;
  Qt::PenStyle          pen_style = Qt::DashLine;
  bool                  is_link_hovered = false;
  std::vector<LinkType> link_types = {LinkType::BROKEN_LINE,
                                      LinkType::CIRCUIT,
                                      LinkType::CUBIC,
                                      LinkType::DEPORTED,
                                      LinkType::LINEAR,
                                      LinkType::QUADRATIC};

  // node endpoints
  GraphicsNode *node_out = nullptr;
  GraphicsNode *node_in = nullptr;
  int           port_out_index;
  int           port_in_index;
};

} // namespace gngui
