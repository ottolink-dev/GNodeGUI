/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#pragma once
#include <functional>
#include <memory>

#include <QEvent>
#include <QGraphicsRectItem>
#include <QMouseEvent>
#include <QPointer>
#include <QWidget>

#include "nlohmann/json.hpp"

#include "gnodegui/graphics_link.hpp"
#include "gnodegui/graphics_node_geometry.hpp"
#include "gnodegui/logger.hpp"
#include "gnodegui/node_proxy.hpp"

namespace gngui
{

class GraphicsLink; // forward decl

class GraphicsNode : public QGraphicsRectItem
{
public:
  GraphicsNode(QPointer<NodeProxy> p_proxy, QGraphicsItem *parent = nullptr);
  ~GraphicsNode();

  // --- Serializzation

  void           json_from(const nlohmann::json &json);
  nlohmann::json json_to() const;

  // --- Getters

  std::string                 get_caption() const;
  std::string                 get_category() const;
  std::vector<std::string>    get_category_splitted(char delimiter = '/') const;
  std::string                 get_data_type(int port_index) const;
  const GraphicsNodeGeometry &get_geometry() const;
  std::string                 get_id() const;
  std::string                 get_main_category() const;
  int                         get_nports() const;
  std::string                 get_port_caption(int port_index) const;
  std::string                 get_port_id(int port_index) const;
  int                         get_port_index(const std::string &id) const;
  PortType                    get_port_type(int port_index) const;
  const NodeProxy            *get_proxy_ref() const;
  bool                        is_port_available(int port_index);

  // --- Setters

  void set_is_node_pinned(bool new_state);
  void set_is_port_connected(int port_index, GraphicsLink *p_link);
  void set_p_proxy(QPointer<NodeProxy> new_p_proxy);

  void set_widget(QWidget *new_widget, QSize widget_size = QSize());
  void set_widget_visibility(bool is_visible);

  // --- UI

  void update_geometry();

  // --- "slots" equivalent
  void on_compute_finished();
  void on_compute_started();

  // --- Callbacks - "signals" equivalent
  std::function<void(GraphicsNode *from, int port_index, QPointF scene_pos)>
      connection_dropped;
  std::function<
      void(GraphicsNode *from, int port_from_index, GraphicsNode *to, int port_to_index)>
                                                                connection_finished;
  std::function<void(GraphicsNode *from, int port_index)>       connection_started;
  std::function<void(const std::string &id)>                    selected;
  std::function<void(const std::string &id)>                    deselected;
  std::function<void(const std::string &id, QPointF scene_pos)> right_clicked;

protected:
  // --- Qt methods override

  void     hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
  void     hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
  void     hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
  QRectF   boundingRect() const override;
  QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
  void     mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  void     mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  void     mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
  bool     sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

  virtual void paint(QPainter                       *painter,
                     const QStyleOptionGraphicsItem *option,
                     QWidget                        *widget) override;

private:
  QSizeF get_widget_size() const;

  // --- Hover state

  int  get_hovered_port_index() const;
  bool update_is_port_hovered(QPointF scene_pos);
  void update_links();
  void reset_is_port_hovered();

  // --- Members

  QPointer<NodeProxy>         p_proxy;
  GraphicsNodeGeometry        geometry;
  std::string                 current_comment;
  QSizeF                      current_widget_size;
  bool                        is_node_dragged = false;
  bool                        is_node_hovered = false;
  bool                        is_node_pinned = false;
  std::vector<bool>           is_port_hovered;
  std::vector<GraphicsLink *> connected_link_ref; // owned by GraphViewer
  bool                        is_node_computing = false;
  bool                        is_widget_visible = true;
  bool                        has_connection_started = false;
  bool                        is_port_pressed = false;
  int                         port_index_from;
  std::string                 data_type_connecting = "";
  QGraphicsProxyWidget       *proxy_widget = nullptr; // owned by this
};

// --- helper

bool is_valid(GraphicsNode *node);
bool is_valid(GraphicsLink *link);

} // namespace gngui
