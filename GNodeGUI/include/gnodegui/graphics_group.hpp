/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#pragma once
#include <memory>

#include "nlohmann/json.hpp"

#include <QCursor>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>

#include "gnodegui/logger.hpp"

namespace gngui
{

class GraphicsGroup : public QGraphicsRectItem
{
public:
  GraphicsGroup(QGraphicsItem *parent = nullptr);

  void           json_from(const nlohmann::json &json);
  nlohmann::json json_to() const;

  // place and size the group to cover this scene rectangle
  void fit_to(const QRectF &scene_rect);
  void set_caption(const std::string &new_caption);
  void set_color(const QColor &new_color);

protected:
  enum Corner
  {
    NONE,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
  } current_corner;

  void     contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
  void     hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
  void     hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
  void     hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
  QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
  void     mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
  void     mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  void     mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  void     mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

  virtual void paint(QPainter                       *painter,
                     const QStyleOptionGraphicsItem *option,
                     QWidget                        *widget) override;

private:
  void rearrange_z_order_by_size();
  void update_selected_items();

  Corner get_resize_corner(const QPointF &pos) const;
  void   update_caption_position();

  QGraphicsTextItem *caption_item;
  QColor             color;

  bool is_hovered = false;

  bool    resizing;
  QPointF resize_start_pos;
  qreal   resize_handle_size; // Size of the corner area for resizing

  bool                   dragging;
  QPointF                drag_start_pos;
  QList<QGraphicsItem *> selected_items;
};

} // namespace gngui
