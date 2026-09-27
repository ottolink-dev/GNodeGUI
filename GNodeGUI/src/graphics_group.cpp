/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <QAction>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QTimer>

#include "gnodegui/graphics_group.hpp"
#include "gnodegui/graphics_link.hpp"
#include "gnodegui/logger.hpp"
#include "gnodegui/style.hpp"
#include "gnodegui/utils.hpp"

namespace gngui
{

QPixmap create_colored_pixmap(const QColor &color, QSize size = QSize(64, 16))
{
  QPixmap pixmap(size);
  pixmap.fill(Qt::transparent); // Transparent background
  QPainter painter(&pixmap);
  painter.setBrush(QBrush(color));
  painter.setPen(Qt::NoPen);
  painter.drawRect(0, 0, size.width(), size.height());
  return pixmap;
}

GraphicsGroup::GraphicsGroup(QGraphicsItem *parent)
    : QGraphicsRectItem(parent), resizing(false), resize_handle_size(20.f)
{
  this->setFlag(QGraphicsItem::ItemIsSelectable, true);
  this->setFlag(QGraphicsItem::ItemIsMovable, true);
  this->setAcceptHoverEvents(true);
  this->setZValue(-3.); // behind everything else, see rearrange_z_order_by_size
  this->setRect(0.f, 0.f, GN_STYLE->group.default_width, GN_STYLE->group.default_height);

  // Create a caption text item at the top middle of the rectangle
  this->caption_item = new QGraphicsTextItem("Double-click to edit caption", this);
  this->caption_item->setTextInteractionFlags(Qt::NoTextInteraction);
  this->caption_item->setFlag(QGraphicsItem::ItemIsSelectable, false);

  QFont font = this->caption_item->font();
  font.setBold(GN_STYLE->group.bold_caption);
  this->caption_item->setFont(font);

  this->set_color(GN_STYLE->group.color);
  this->update_caption_position();
}

void GraphicsGroup::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
  // first check that there is no node underneath, if so, nothing is
  // done and priority is given to the node context menu
  for (auto &item : this->scene()->items())
    if (GraphicsNode *p_node = dynamic_cast<GraphicsNode *>(item))
      if (p_node->contains(p_node->mapFromScene(event->scenePos())))
        return;

  // if not, generate the context menu
  QMenu menu;

  QAction *select_contents = menu.addAction("Select contents");
  QAction *ungroup = menu.addAction("Ungroup");
  menu.addSeparator();

  // get the default icon size for the QMenu
  int   icon_size = menu.style()->pixelMetric(QStyle::PM_SmallIconSize);
  QSize psize = QSize(icon_size, icon_size);

  // create actions with colored rectangles
  std::vector<QAction *> actions = {};

  for (auto &[name, color] : GN_STYLE->group.color_map)
  {
    QAction *action = menu.addAction(create_colored_pixmap(color, psize),
                                     QString::fromStdString(name));
    actions.push_back(action);
  }

  // show the menu at the event's position
  QAction *selected_action = menu.exec(event->screenPos());

  // both select what the group holds, so it can still be moved as one
  if (selected_action == select_contents || selected_action == ungroup)
  {
    this->scene()->clearSelection();
    this->update_selected_items();
    for (QGraphicsItem *item : this->selected_items)
      item->setSelected(true);

    // ungroup: the contents stay in place and only the frame goes, once this
    // event is done with it
    if (selected_action == ungroup)
    {
      QGraphicsScene *scene = this->scene();
      QTimer::singleShot(0,
                         scene,
                         [scene, this]()
                         {
                           if (scene->items().contains(this))
                             clean_delete_graphics_item(this);
                         });
    }

    event->accept();
    return;
  }

  // set the color based on the selected action
  auto it = std::find(actions.begin(), actions.end(), selected_action);
  if (it != actions.end())
  {
    size_t index = std::distance(actions.begin(), it);
    auto   color_it = std::next(GN_STYLE->group.color_map.begin(), index);
    this->set_color(color_it->second);
  }

  event->accept();
}

GraphicsGroup::Corner GraphicsGroup::get_resize_corner(const QPointF &pos) const
{
  QRectF rect = this->rect();
  QSizeF handle_size(this->resize_handle_size, this->resize_handle_size);

  QPointF top_left = rect.topLeft();
  QPointF top_right = rect.topRight() - QPointF(this->resize_handle_size, 0);
  QPointF bottom_left = rect.bottomLeft() - QPointF(0, this->resize_handle_size);
  QPointF bottom_right = rect.bottomRight() -
                         QPointF(this->resize_handle_size, this->resize_handle_size);

  QRectF top_left_rect(top_left, handle_size);
  QRectF top_right_rect(top_right, handle_size);
  QRectF bottom_left_rect(bottom_left, handle_size);
  QRectF bottom_right_rect(bottom_right, handle_size);

  if (top_left_rect.contains(pos))
    return Corner::TOP_LEFT;
  if (top_right_rect.contains(pos))
    return Corner::TOP_RIGHT;
  if (bottom_left_rect.contains(pos))
    return Corner::BOTTOM_LEFT;
  if (bottom_right_rect.contains(pos))
    return Corner::BOTTOM_RIGHT;

  return Corner::NONE;
}

void GraphicsGroup::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
  this->is_hovered = true;
  QGraphicsRectItem::hoverEnterEvent(event);
}

void GraphicsGroup::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
  this->is_hovered = false;
  QGraphicsRectItem::hoverLeaveEvent(event);
}

void GraphicsGroup::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
  // Change the cursor when hovering near the edges or corners for resizing
  Corner corner = get_resize_corner(event->pos());

  switch (corner)
  {
  case Corner::TOP_LEFT:
  case Corner::BOTTOM_RIGHT:
    setCursor(Qt::SizeFDiagCursor);
    break;
  case Corner::TOP_RIGHT:
  case Corner::BOTTOM_LEFT:
    setCursor(Qt::SizeBDiagCursor);
    break;
  default:
    setCursor(Qt::ArrowCursor);
    break;
  }

  QGraphicsRectItem::hoverMoveEvent(event);
}

QVariant GraphicsGroup::itemChange(GraphicsItemChange change, const QVariant &value)
{
  if (change == QGraphicsItem::ItemSceneChange && scene())
    rearrange_z_order_by_size();
  return QGraphicsRectItem::itemChange(change, value);
}

void GraphicsGroup::json_from(const nlohmann::json &json)
{
  // Caption
  if (json.contains("caption") && json["caption"].is_string())
    this->set_caption(json["caption"].get<std::string>());

  // Color
  if (json.contains("color") && json["color"].is_array() && json["color"].size() == 4)
  {
    auto cvec = json["color"];
    this->set_color(QColor(cvec[0].get<int>(),
                           cvec[1].get<int>(),
                           cvec[2].get<int>(),
                           cvec[3].get<int>()));
  }

  // Position
  if (json.contains("position") && json["position"].is_array() &&
      json["position"].size() >= 2)
  {
    auto  pos = json["position"];
    float x = pos[0].get<float>();
    float y = pos[1].get<float>();
    this->setPos(x, y);
  }

  // Width and height
  float width = GN_STYLE->group.default_width;
  float height = GN_STYLE->group.default_height;

  if (json.contains("width") && json["width"].is_number())
    width = json["width"].get<float>();
  if (json.contains("height") && json["height"].is_number())
    height = json["height"].get<float>();

  this->setRect(0, 0, width, height);

  this->update_caption_position();
}

nlohmann::json GraphicsGroup::json_to() const
{
  nlohmann::json json;

  json["caption"] = this->caption_item->document()->toRawText().toStdString();
  json["color"] = {this->color.red(),
                   this->color.green(),
                   this->color.blue(),
                   this->color.alpha()};

  QRectF box = this->sceneBoundingRect();
  json["position"] = {box.x(), box.y()};
  json["width"] = box.width();
  json["height"] = box.height();

  return json;
}

void GraphicsGroup::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
  // check if double-click is on the caption to start editing
  QRectF bbox = this->caption_item->boundingRect();
  bbox.moveTo(this->caption_item->pos());

  // event position with respect current graphic item
  QPointF item_pos = event->scenePos() - this->scenePos();

  if (bbox.contains(item_pos))
  {
    bool    ok;
    QString new_caption = QInputDialog::getText(nullptr,
                                                "Edit Caption",
                                                "Enter new caption:",
                                                QLineEdit::Normal,
                                                this->caption_item->toPlainText(),
                                                &ok);
    if (ok && !new_caption.isEmpty())
    {
      this->caption_item->setPlainText(new_caption);
      this->update_caption_position();
    }
  }
  QGraphicsRectItem::mouseDoubleClickEvent(event);
}

void GraphicsGroup::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    // --- resizing and dragging

    // check if mouse is near a corner or edge for resizing
    this->current_corner = this->get_resize_corner(event->pos());

    if (this->current_corner != NONE)
    {
      this->resizing = true;
      this->resize_start_pos = event->pos();
    }
    else
    {
      // begin dragging all items inside the rectangle
      this->dragging = true;
      this->drag_start_pos = event->scenePos();

      QRectF bbox = this->rect();
      bbox.moveTo(this->scenePos());
      this->update_selected_items();
    }
  }

  QGraphicsRectItem::mousePressEvent(event);
}

void GraphicsGroup::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  if (resizing)
  {
    // Resize the rectangle based on the corner being resized
    QRectF  new_rect = this->rect();
    QPointF delta = event->pos() - this->resize_start_pos;

    switch (this->current_corner)
    {
    case Corner::TOP_LEFT:
      new_rect.setTopLeft(new_rect.topLeft() + delta);
      break;
    case Corner::TOP_RIGHT:
      new_rect.setTopRight(new_rect.topRight() + delta);
      break;
    case Corner::BOTTOM_LEFT:
      new_rect.setBottomLeft(new_rect.bottomLeft() + delta);
      break;
    case Corner::BOTTOM_RIGHT:
      new_rect.setBottomRight(new_rect.bottomRight() + delta);
      break;
    default:
      break;
    }

    this->setRect(new_rect);
    this->resize_start_pos = event->pos();
    this->update_selected_items();

    this->update_caption_position();
    return;
  }
  else if (dragging)
  {
    // move the rectangle and items inside it
    QPointF delta = event->scenePos() - this->drag_start_pos;

    // move the nodes first
    for (QGraphicsItem *item : this->selected_items)
    {
      GraphicsNode  *p_node = dynamic_cast<GraphicsNode *>(item);
      GraphicsGroup *p_group = dynamic_cast<GraphicsGroup *>(item);

      if (p_node || p_group)
        item->moveBy(delta.x(), delta.y());
    }

    // then make the links follow (all of them)
    for (QGraphicsItem *item : this->scene()->items())
    {
      if (GraphicsLink *p_link = dynamic_cast<GraphicsLink *>(item))
        p_link->update_path();
    }

    // move the rectangle itself
    this->setPos(pos() + delta);
    this->drag_start_pos = event->scenePos();

    this->update_caption_position();
  }

  QGraphicsRectItem::mouseMoveEvent(event);
}

void GraphicsGroup::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  this->resizing = false;
  this->dragging = false;
  this->rearrange_z_order_by_size();
  QGraphicsRectItem::mouseReleaseEvent(event);
}

void GraphicsGroup::paint(QPainter                       *painter,
                          const QStyleOptionGraphicsItem *option,
                          QWidget                        *widget)
{
  Q_UNUSED(option);
  Q_UNUSED(widget);

  painter->save();

  // set the pen depending on the state (selected, hovered, or default)
  qreal pen_width = GN_STYLE->group.pen_width;
  if (this->isSelected())
  {
    pen_width = GN_STYLE->group.pen_width_selected;
  }
  else if (this->is_hovered)
  {
    pen_width = GN_STYLE->group.pen_width_hovered;
  }
  painter->setPen(QPen(this->color, pen_width));

  // set the fill color with the defined transparency
  QColor fill_color = this->color;
  fill_color.setAlphaF(GN_STYLE->group.background_fill_alpha);
  painter->setBrush(fill_color);

  // draw the rounded rectangle
  painter->drawRoundedRect(this->rect(),
                           GN_STYLE->group.rounding_radius,
                           GN_STYLE->group.rounding_radius);

  painter->restore();
}

void GraphicsGroup::rearrange_z_order_by_size()
{
  if (!this->scene())
    return;

  // reorder Z-order of groups by group size (smaller groups in front)
  std::vector<GraphicsGroup *> groups;

  for (QGraphicsItem *item : this->scene()->items())
  {
    if (GraphicsGroup *p_group = dynamic_cast<GraphicsGroup *>(item))
      groups.push_back(p_group);
  }

  if (groups.empty())
    return;

  std::sort(groups.begin(),
            groups.end(),
            [](QGraphicsItem *a, QGraphicsItem *b)
            {
              QRectF rect_a = a->boundingRect();
              QRectF rect_b = b->boundingRect();
              qreal  area_a = rect_a.width() * rect_a.height();
              qreal  area_b = rect_b.width() * rect_b.height();
              return area_a > area_b; // smallest last, largest first
            });

  // all behind the comments (-2), links (-1) and nodes (0), so a click on a
  // node inside a group reaches the node
  qreal z_value = -3.0 - qreal(groups.size());
  for (auto item : groups)
  {
    item->setZValue(z_value);
    z_value += 1.0; // each item is above the previous
  }
}

void GraphicsGroup::fit_to(const QRectF &scene_rect)
{
  this->setPos(scene_rect.topLeft());
  this->setRect(QRectF(QPointF(0., 0.), scene_rect.size()));
  this->update_caption_position();
  this->rearrange_z_order_by_size();
}

void GraphicsGroup::set_caption(const std::string &new_caption)
{
  this->caption_item->setPlainText(new_caption.c_str());
  this->update_caption_position();
}

void GraphicsGroup::set_color(const QColor &new_color)
{
  this->color = new_color;
  this->caption_item->setDefaultTextColor(this->color);
  this->update();
}

void GraphicsGroup::update_caption_position()
{
  QRectF  rect = this->rect();
  QRectF  caption_bbox = this->caption_item->boundingRect();
  QPointF top_center = rect.topLeft() +
                       QPointF(0.5f * (rect.width() - caption_bbox.width()), 0.f);
  this->caption_item->setPos(top_center);
}

void GraphicsGroup::update_selected_items()
{
  if (!scene())
    return;

  QRectF bbox = this->sceneBoundingRect();

  this->selected_items.clear();
  for (QGraphicsItem *item : this->scene()->items(bbox))
  {
    if (item == this)
      continue;

    if (bbox.contains(item->sceneBoundingRect()))
      this->selected_items.push_back(item);
  }
}

} // namespace gngui
