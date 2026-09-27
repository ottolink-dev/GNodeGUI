/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <algorithm>
#include <sstream>
#include <utility>

#include <QApplication>
#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "gnodegui/graphics_link.hpp"
#include "gnodegui/graphics_node.hpp"
#include "gnodegui/icons/reload_icon.hpp"
#include "gnodegui/icons/show_settings_icon.hpp"
#include "gnodegui/logger.hpp"
#include "gnodegui/style.hpp"
#include "gnodegui/utils.hpp"

namespace gngui
{

GraphicsNode::GraphicsNode(QPointer<NodeProxy> p_proxy, QGraphicsItem *parent)
    : QGraphicsRectItem(parent), p_proxy(p_proxy)
{
  if (!this->p_proxy)
  {
    Logger::log()->error("GraphicsNode::GraphicsNode: input p_proxy is nullptr");
    return;
  }

  // item flags
  this->setFlag(QGraphicsItem::ItemIsSelectable, true);
  this->setFlag(QGraphicsItem::ItemIsMovable, true);
  this->setFlag(QGraphicsItem::ItemDoesntPropagateOpacityToChildren, false);
  this->setFlag(QGraphicsItem::ItemIsFocusable, true);
  this->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
  this->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);

  this->setAcceptHoverEvents(true);
  this->setOpacity(1.f);
  this->setZValue(0);

  // tooltip
  const std::string tooltip = this->p_proxy->get_tool_tip_text();
  if (!tooltip.empty())
    this->setToolTip(QString::fromStdString(tooltip));

  // initialize port states
  this->is_port_hovered.resize(this->get_nports());
  this->connected_link_ref.resize(this->get_nports());

  // geometry
  this->update_geometry();
}

GraphicsNode::~GraphicsNode()
{
  Logger::log()->debug("GraphicsNode::~GraphicsNode: {}", this->get_id());

  // stop interactions
  this->setEnabled(false);
  this->setAcceptHoverEvents(false);
  this->setAcceptedMouseButtons(Qt::NoButton);

  // destroy proxy widget safely
  if (this->proxy_widget)
  {
    this->proxy_widget->setWidget(nullptr);
    this->proxy_widget->deleteLater();
    this->proxy_widget = nullptr;
  }
}

std::string GraphicsNode::get_caption() const
{
  if (!this->p_proxy)
    return std::string();

  return this->p_proxy->get_caption();
}

std::string GraphicsNode::get_category() const
{
  if (!this->p_proxy)
    return std::string();

  return this->p_proxy->get_category();
}

std::vector<std::string> GraphicsNode::get_category_splitted(char delimiter) const
{
  return split_string(this->get_category(), delimiter);
}

std::string GraphicsNode::get_data_type(int port_index) const
{
  if (!this->p_proxy)
    return std::string();

  return this->p_proxy->get_data_type(port_index);
}

const GraphicsNodeGeometry &GraphicsNode::get_geometry() const { return this->geometry; }

int GraphicsNode::get_hovered_port_index() const
{
  auto it = std::find(this->is_port_hovered.begin(), this->is_port_hovered.end(), true);

  // if found, calculate the index; otherwise, return -1
  if (it != this->is_port_hovered.end())
  {
    int index = std::distance(this->is_port_hovered.begin(), it);
    return index;
  }
  else
    return -1;
}

std::string GraphicsNode::get_id() const
{
  if (!this->p_proxy)
    return std::string();

  return this->p_proxy->get_id();
}

std::string GraphicsNode::get_main_category() const
{
  std::string node_category = this->get_category();
  int         pos = node_category.find("/");
  return node_category.substr(0, pos);
}

int GraphicsNode::get_nports() const
{
  if (!this->p_proxy)
    return 0;

  return this->p_proxy->get_nports();
}

std::string GraphicsNode::get_port_caption(int port_index) const
{
  if (!this->p_proxy)
    return std::string();

  return this->p_proxy->get_port_caption(port_index);
}

std::string GraphicsNode::get_port_id(int port_index) const
{
  if (!this->p_proxy)
    return std::string();

  return this->p_proxy->get_port_id(port_index);
}

int GraphicsNode::get_port_index(const std::string &id) const
{
  for (int k = 0; k < this->get_nports(); k++)
    if (this->get_port_id(k) == id)
      return k;

  return -1;
}

PortType GraphicsNode::get_port_type(int port_index) const
{
  if (!this->p_proxy)
    return PortType::OUT;

  return this->p_proxy->get_port_type(port_index);
}

const NodeProxy *GraphicsNode::get_proxy_ref() const { return this->p_proxy; }

QSizeF GraphicsNode::get_widget_size() const
{
  QSizeF size = QSizeF();

  if (this->proxy_widget)
  {
    if (QWidget *widget = this->proxy_widget->widget())
      size = widget->size();
  }

  return size;
}

void GraphicsNode::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
  this->is_node_hovered = true;
  this->update();

  QGraphicsRectItem::hoverEnterEvent(event);
}

void GraphicsNode::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
  this->is_node_hovered = false;
  this->setCursor(Qt::ArrowCursor);
  this->update();

  QGraphicsRectItem::hoverLeaveEvent(event);
}

void GraphicsNode::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
  QPointF pos = event->pos();
  QPointF scene_pos = this->mapToScene(pos);
  QPointF item_pos = scene_pos - this->scenePos();

  if (this->update_is_port_hovered(item_pos))
    this->update();

  QGraphicsRectItem::hoverMoveEvent(event);
}

bool GraphicsNode::is_port_available(int port_index)
{
  return this->get_port_type(port_index) == PortType::OUT ||
         !this->connected_link_ref[port_index];
}

QVariant GraphicsNode::itemChange(GraphicsItemChange change, const QVariant &value)
{
  if (change == QGraphicsItem::ItemSelectedHasChanged)
  {
    const bool new_selection_state = value.toBool();

    if (new_selection_state)
    {
      if (this->selected)
        this->selected(this->get_id());
    }
    else
    {
      if (this->deselected)
        this->deselected(this->get_id());
    }
  }

  if (change == QGraphicsItem::ItemPositionHasChanged)
  {
    this->update_links();
  }

  return QGraphicsItem::itemChange(change, value);
}

void GraphicsNode::json_from(const nlohmann::json &json)
{
  json_safe_get(json, "is_widget_visible", this->is_widget_visible);

  float x = 0;
  float y = 0;
  json_safe_get(json, "scene_position.x", x);
  json_safe_get(json, "scene_position.y", y);
  this->setPos(QPointF(x, y));
}

nlohmann::json GraphicsNode::json_to() const
{
  nlohmann::json json;

  json["is_widget_visible"] = this->is_widget_visible;
  json["scene_position.x"] = this->scenePos().x();
  json["scene_position.y"] = this->scenePos().y();

  // for info only
  {
    json["id"] = this->get_id();
    json["caption"] = this->get_caption();
  }

  return json;
}

void GraphicsNode::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  // let the base class handle normal movement
  QGraphicsItem::mouseMoveEvent(event);
}

void GraphicsNode::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    int hovered_port_index = this->get_hovered_port_index();

    // Ctrl+click on an output selects every link leaving it
    if (hovered_port_index >= 0 && (event->modifiers() & Qt::ControlModifier) &&
        this->get_port_type(hovered_port_index) == PortType::OUT)
    {
      this->scene()->clearSelection();
      for (QGraphicsItem *item : this->scene()->items())
        if (auto *p_link = dynamic_cast<GraphicsLink *>(item))
          if (p_link->get_node_out() == this &&
              p_link->get_port_out_index() == hovered_port_index)
            p_link->setSelected(true);
      this->is_port_pressed = true;
      event->accept();
      return;
    }

    if (hovered_port_index >= 0 && this->is_port_available(hovered_port_index))
    {
      Logger::log()->trace("GraphicsNode::mousePressEvent: connection_started {}:{}",
                           this->get_id(),
                           hovered_port_index);

      this->has_connection_started = true;
      this->setFlag(QGraphicsItem::ItemIsMovable, false);
      this->port_index_from = hovered_port_index;
      this->data_type_connecting = this->get_data_type(hovered_port_index);
      if (this->connection_started)
        this->connection_started(this, hovered_port_index);

      // accepted without the base handler, which would also select the node
      this->is_port_pressed = true;
      event->accept();
      return;
    }
    else
    {
      this->is_node_dragged = true;
    }
  }

  else if (event->button() == Qt::RightButton)
  {
    QPointF pos = event->pos();
    QPointF scene_pos = this->mapToScene(pos);
    if (this->right_clicked)
      this->right_clicked(this->get_id(), scene_pos);
  }

  QGraphicsRectItem::mousePressEvent(event);
}

void GraphicsNode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    if (this->is_node_dragged)
    {
      this->is_node_dragged = false;
    }
    else if (this->has_connection_started)
    {
      bool                   is_dropped = true;
      QList<QGraphicsItem *> items_under_mouse = scene()->items(event->scenePos());

      for (QGraphicsItem *item : items_under_mouse)
      {
        if (GraphicsNode *p_target_node = dynamic_cast<GraphicsNode *>(item))
        {

          int hovered_port_index = p_target_node ? p_target_node->get_hovered_port_index()
                                                 : -1;
          if (hovered_port_index >= 0 && p_target_node != this)
          {
            Logger::log()->trace(
                "GraphicsNode::mouseReleaseEvent: connection_finished {}:{}",
                p_target_node->get_id(),
                hovered_port_index);

            if (this->connection_finished)
              this->connection_finished(this,
                                        this->port_index_from,
                                        p_target_node,
                                        hovered_port_index);

            is_dropped = false;
            break;
          }
          else
          {
            is_dropped = true;
            break;
          }
        }
      }

      this->reset_is_port_hovered();
      this->update();

      if (is_dropped)
      {
        Logger::log()->trace("GraphicsNode::mouseReleaseEvent connection_dropped {}",
                             this->get_id());

        if (this->connection_dropped)
          this->connection_dropped(this, this->port_index_from, event->scenePos());
      }

      this->has_connection_started = false;

      // clean-up port color state
      for (QGraphicsItem *item : this->scene()->items())
      {
        if (GraphicsNode *node = dynamic_cast<GraphicsNode *>(item))
        {
          node->data_type_connecting = "";
          node->update();
        }
      }

      this->setFlag(QGraphicsItem::ItemIsMovable, true);
    }
  }

  // the base handler would select the node when the press was on a port
  if (std::exchange(this->is_port_pressed, false))
    return;

  QGraphicsRectItem::mouseReleaseEvent(event);
}

void GraphicsNode::on_compute_finished()
{
  Logger::log()->trace("GraphicsNode::on_compute_finished, node {}", this->get_caption());
  this->is_node_computing = false;
  this->update();
}

void GraphicsNode::on_compute_started()
{
  Logger::log()->trace("GraphicsNode::on_compute_started, node {}", this->get_caption());
  this->is_node_computing = true;
  this->update();
}

QRectF GraphicsNode::boundingRect() const
{
  // the pinned outline is drawn up to 2 pen widths past the body, which
  // reaches past the node rect at the bottom
  const qreal m = 2. * GN_STYLE->node.pen_width_selected + 1.;
  return QGraphicsRectItem::boundingRect().adjusted(-m, -m, m, m);
}

void GraphicsNode::paint(QPainter                       *painter,
                         const QStyleOptionGraphicsItem *option,
                         QWidget * /* widget */)
{
  if (!this->p_proxy)
    return;

  const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

  if (current_widget_size != this->get_widget_size())
    this->update_geometry();

  painter->save();

  // --- Background rectangle

  painter->setBrush(QBrush(GN_STYLE->node.color_bg));
  painter->setPen(Qt::NoPen);
  painter->drawRoundedRect(this->geometry.body_rect,
                           GN_STYLE->node.rounding_radius,
                           GN_STYLE->node.rounding_radius);

  // --- Add outer border for pinned node

  if (this->is_node_pinned)
  {
    painter->setBrush(Qt::NoBrush);

    QPen pen = QPen(GN_STYLE->node.color_pinned, 2.f * GN_STYLE->node.pen_width_selected);
    pen.setStyle(Qt::DashLine);
    painter->setPen(pen);

    float w = GN_STYLE->node.pen_width_selected;

    painter->drawRoundedRect(this->geometry.body_rect.adjusted(-w, -w, w, w),
                             GN_STYLE->node.rounding_radius,
                             GN_STYLE->node.rounding_radius);

    pen.setStyle(Qt::SolidLine);
    painter->setPen(pen);
  }

  // --- Caption

  // Set pen based on whether the node is selected or not
  painter->setPen(this->isSelected() ? GN_STYLE->node.color_selected
                                     : GN_STYLE->node.color_caption);
  painter->drawText(this->geometry.caption_pos, this->get_caption().c_str());

  // --- Header

  std::string main_category = this->get_main_category();
  QColor      header_color = GN_STYLE->node.color_bg_light;

  if (GN_STYLE->node.color_category.contains(main_category))
    header_color = GN_STYLE->node.color_category.at(main_category);

  if (this->is_node_computing)
  {
    QColor dim_color = header_color;
    dim_color.setAlphaF(0.5f * header_color.alphaF());
    painter->setBrush(dim_color);
  }
  else
  {
    painter->setBrush(header_color);
  }

  painter->setPen(Qt::NoPen);
  QPainterPath path;
  QRectF       rect = this->geometry.header_rect;
  float        radius = GN_STYLE->node.rounding_radius;

  path.moveTo(rect.left(), rect.bottom());
  path.lineTo(rect.left(), rect.top() + radius);
  path.arcTo(rect.left(), rect.top(), radius * 2, radius * 2, 180, -90);
  path.lineTo(rect.right() - radius, rect.top());
  path.arcTo(rect.right() - radius * 2, rect.top(), radius * 2, radius * 2, 90, -90);
  path.lineTo(rect.right(), rect.bottom());
  path.closeSubpath();

  painter->drawPath(path);

  // --- Border

  painter->setBrush(Qt::NoBrush);
  if (this->isSelected())
    painter->setPen(
        QPen(GN_STYLE->node.color_selected, GN_STYLE->node.pen_width_selected));
  else if (this->is_node_hovered)
    painter->setPen(
        QPen(GN_STYLE->node.color_border_hovered, GN_STYLE->node.pen_width_hovered));
  else
    painter->setPen(QPen(GN_STYLE->node.color_border, GN_STYLE->node.pen_width));

  painter->drawRoundedRect(this->geometry.body_rect,
                           GN_STYLE->node.rounding_radius,
                           GN_STYLE->node.rounding_radius);

  // --- Ports

  for (int k = 0; k < this->p_proxy->get_nports(); k++)
  {
    // Set alignment based on port type (IN/OUT)
    int align_flag = (this->get_port_type(k) == PortType::IN) ? Qt::AlignLeft
                                                              : Qt::AlignRight;

    // Draw port labels
    painter->setPen(Qt::white); // Assuming labels are always white
    painter->drawText(this->geometry.port_label_rects[k],
                      align_flag,
                      this->get_port_caption(k).c_str());

    // Port appearance when selected or not. The hovered ring keeps a couple of
    // screen pixels when zoomed out (capped to stay inside the node rect)
    if (this->is_port_hovered[k])
      painter->setPen(
          QPen(GN_STYLE->node.color_port_hovered,
               std::clamp(2. / lod, qreal(GN_STYLE->node.pen_width_hovered), 8.)));
    else if (this->is_node_hovered)
      painter->setPen(
          QPen(GN_STYLE->node.color_border_hovered, GN_STYLE->node.pen_width_hovered));
    else
      painter->setPen(QPen(GN_STYLE->node.color_border, GN_STYLE->node.pen_width));

    // Set port brush based on data type compatibility
    std::string data_type = this->get_data_type(k);
    float       port_radius = GN_STYLE->node.port_radius;
    if (!this->data_type_connecting.empty() && data_type != this->data_type_connecting)
    {
      painter->setBrush(GN_STYLE->node.color_port_not_selectable);
      port_radius = GN_STYLE->node.port_radius_not_selectable;
    }
    else
      painter->setBrush(get_color_from_data_type(data_type));

    // Draw the port as a circle (ellipse with equal width and height)
    painter->drawEllipse(this->geometry.port_rects[k].center(), port_radius, port_radius);
  }

  // --- Comment

  std::string comment = this->p_proxy->get_comment();

  if (!comment.empty())
  {
    if (comment != this->current_comment)
      this->update_geometry();

    painter->setPen(GN_STYLE->node.color_comment);
    painter->drawText(this->geometry.comment_rect,
                      Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                      comment.c_str());

    this->current_comment = comment;
  }

  painter->restore();
}

void GraphicsNode::set_is_node_pinned(bool new_state)
{
  this->is_node_pinned = new_state;
  this->update();
}

void GraphicsNode::set_is_port_connected(int port_index, GraphicsLink *p_link)
{
  this->connected_link_ref[port_index] = p_link;
}

void GraphicsNode::reset_is_port_hovered()
{
  this->is_port_hovered.assign(this->is_port_hovered.size(), false);
}

bool GraphicsNode::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
  // Try to cast the watched item to a GraphicsNode
  if (GraphicsNode *p_node = dynamic_cast<GraphicsNode *>(watched))
  {
    // Check for mouse move while connection started
    if (event->type() == QEvent::GraphicsSceneMouseMove && p_node &&
        p_node->has_connection_started)
    {
      QGraphicsSceneMouseEvent *mouse_event = static_cast<QGraphicsSceneMouseEvent *>(
          event);
      QPointF item_pos = mouse_event->scenePos() - this->scenePos();

      // Update current data type of the building connection
      if (p_node && this->data_type_connecting != p_node->data_type_connecting)
      {
        this->data_type_connecting = p_node->data_type_connecting;
        this->update();
      }

      if (!p_node)
        return false; // one got deleted during update

      // Update hovering port status
      if (this->update_is_port_hovered(item_pos))
      {
        for (int k = 0; k < this->get_nports(); k++)
        {
          if (this->is_port_hovered[k])
          {
            if (!p_node)
              break; // avoid deref if deleted mid-loop

            int      from_pidx = p_node->port_index_from;
            PortType from_ptype = p_node->get_port_type(from_pidx);
            PortType to_ptype = this->get_port_type(k);

            std::string from_pdata = p_node->get_data_type(from_pidx);
            std::string to_pdata = this->get_data_type(k);

            // Incompatible or same type → deactivate hover
            if (from_ptype == to_ptype || from_pdata != to_pdata)
              this->is_port_hovered[k] = false;
          }
        }
      }
    }
  }

  return QGraphicsRectItem::sceneEventFilter(watched, event);
}

void GraphicsNode::set_p_proxy(QPointer<NodeProxy> new_p_proxy)
{
  this->p_proxy = new_p_proxy;
}

void GraphicsNode::set_widget(QWidget *new_widget, QSize new_widget_size)
{
  Logger::log()->debug("GraphicsNode::set_widget");

  if (!this->p_proxy || !new_widget)
    return;

  // erase current parenting
  if (new_widget->parentWidget())
    new_widget->setParent(nullptr);

  // clean-up existing container
  if (this->proxy_widget)
  {
    QWidget *old = this->proxy_widget->widget();
    this->proxy_widget->setWidget(nullptr);
    if (old)
      old->deleteLater();
    this->proxy_widget->deleteLater();
  }

  // eventually set widget
  this->proxy_widget = new QGraphicsProxyWidget(this);
  this->proxy_widget->setWidget(new_widget);

  if (!new_widget_size.isValid())
    new_widget_size = new_widget->sizeHint();
  this->proxy_widget->resize(new_widget_size);

  // update the geometry
  this->update_geometry();
  this->proxy_widget->setPos(this->geometry.widget_pos);
  this->update();
}

void GraphicsNode::set_widget_visibility(bool is_visible)
{
  if (!this->proxy_widget)
    return;

  QWidget *widget = this->proxy_widget->widget();

  if (!widget)
    return;

  widget->setVisible(is_visible);

  this->update_geometry();
  this->update();
}

void GraphicsNode::update_geometry()
{
  if (!this->p_proxy)
    return;

  // determine widget size (if any)
  QSizeF widget_size = this->get_widget_size();

  // geometry
  this->geometry = GraphicsNodeGeometry(this->p_proxy, widget_size);
  this->setRect(0.f, 0.f, this->geometry.full_width, this->geometry.full_height);
}

bool GraphicsNode::update_is_port_hovered(QPointF item_pos)
{
  // the nearest port within reach. Zoomed out, a port is only a couple of
  // pixels wide on screen, so the reach grows to a few pixels, but never
  // past a quarter of the node so the body can still be dragged
  qreal scale = 1.;
  if (this->scene() && !this->scene()->views().isEmpty())
    scale = this->scene()->views().first()->transform().m11();

  qreal reach = std::clamp<qreal>(
      5. / scale,
      GN_STYLE->node.port_radius,
      std::max<qreal>(GN_STYLE->node.port_radius,
                      0.25 * this->geometry.body_rect.width()));
  int hovered = -1;

  for (size_t k = 0; k < this->geometry.port_rects.size(); k++)
  {
    const qreal distance = QLineF(item_pos, this->geometry.port_rects[k].center())
                               .length();
    if (distance <= reach)
    {
      reach = distance;
      hovered = int(k);
    }
  }

  // true while a port is hovered, or when one just stopped being hovered
  bool changed = false;
  for (size_t k = 0; k < this->is_port_hovered.size(); k++)
    if (this->is_port_hovered[k] != (int(k) == hovered))
    {
      this->is_port_hovered[k] = int(k) == hovered;
      changed = true;
    }

  return changed || hovered >= 0;
}

void GraphicsNode::update_links()
{
  if (!this->scene())
    return;

  for (QGraphicsItem *item : this->scene()->items())
    if (GraphicsLink *p_link = dynamic_cast<GraphicsLink *>(item))
    {
      if (p_link->get_node_out() == this || p_link->get_node_in() == this)
        p_link->update_path();
    }
}

// --- helper

bool is_valid(GraphicsNode *node) { return node && node->scene() != nullptr; }

bool is_valid(GraphicsLink *link) { return link && link->scene() != nullptr; }

} // namespace gngui
