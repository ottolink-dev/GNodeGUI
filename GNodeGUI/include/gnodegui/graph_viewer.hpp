/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#pragma once
#include <functional>

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QJsonObject>

#include "nlohmann/json.hpp"

#include "gnodegui/graphics_link.hpp"
#include "gnodegui/graphics_node.hpp"
#include "gnodegui/node_proxy.hpp"

namespace gngui
{

// A connection request owns its identifiers; it never exposes scene item pointers.
// Endpoints are always ordered output -> input, even for an input-to-output drag.
struct LinkEndpoints
{
  std::string node_out;
  std::string port_out;
  std::string node_in;
  std::string port_in;

  bool operator==(const LinkEndpoints &) const = default;
};

class GraphViewer : public QGraphicsView
{
  Q_OBJECT

public:
  explicit GraphViewer(std::string id = "graph", QWidget *parent = nullptr);

  // --- Serializzation

  void           json_from(nlohmann::json json, bool clear_existing_content = true);
  nlohmann::json json_to() const;

  // --- Add

  void        add_item(QGraphicsItem *item, QPointF scene_pos = QPointF(0.f, 0.f));
  void        add_link(const std::string &id_out,
                       const std::string &port_id_out,
                       const std::string &to_in,
                       const std::string &port_id_in);
  std::string add_node(NodeProxy         *p_node_proxy,
                       QPointF            scene_pos,
                       const std::string &node_id = "");

  // --- Remove

  void clear();
  void remove_link(const std::string &node_out_id,
                   int                port_out,
                   const std::string &node_in_id,
                   int                port_in,
                   bool               link_will_be_replaced = false);
  void remove_node(const std::string &node_id);

  // Scene synchronization only. These do not emit connection_deleted/node_deleted
  // or invoke edit requests. Removing a node also removes its incident graphics
  // links. Selection notifications are still delivered.
  bool erase_link(const LinkEndpoints &link);
  bool erase_node(const std::string &node_id);

  // --- Editing

  void                     deselect_all();
  std::vector<std::string> get_selected_node_ids(
      std::vector<QPointF> *p_scene_pos_list = nullptr);
  void select_all();
  void set_node_as_selected(const std::string &node_id);
  void unpin_nodes();

  // --- UI

  void add_toolbar(QPoint window_pos);
  bool execute_new_node_context_menu();
  void toggle_link_type();
  void zoom_to_content();

  // --- Getters

  QRectF                      get_bounding_box() const;
  QPointF                     get_center() const;
  GraphicsNode               *get_graphics_node_by_id(const std::string &node_id);
  std::string                 get_id() const;
  std::vector<GraphicsLink *> get_links() const;
  QPointF                     get_mouse_scene_pos() const;

  // --- Setters

  void set_enabled(bool state);
  void set_id(const std::string &new_id) { this->id = new_id; }
  void set_node_inventory(const std::map<std::string, std::string> &new_node_inventory);

  // --- Export

  // useful for debugging graph actual state, after export: to convert, command line: dot
  // export.dot -Tsvg > output.svg
  void export_to_graphviz(const std::string &fname = "export.dot");
  void save_screenshot(const std::string &fname = "screenshot.png");

public Q_SLOTS:

  // --- Qt slots

  void on_compute_finished(const std::string &node_id);
  void on_compute_started(const std::string &node_id);
  void on_node_reload_request(const std::string &node_id);
  void on_node_settings_request(const std::string &node_id);
  void on_node_right_clicked(const std::string &node_id, QPointF scene_pos);
  void on_update_finished();
  void on_update_started();

Q_SIGNALS:

  // NB - signal arguments are passed by value on purpose: a reference is only
  // valid for the duration of the emit and can dangle when the slot runs later
  // (queued connection) or when an earlier slot destroys the referenced object

  // --- Link signals

  void connection_deleted(std::string id_out,
                          std::string port_id_out,
                          std::string to_in,
                          std::string port_id_in,
                          bool        link_will_be_replaced);
  void connection_dropped(std::string node_id, std::string port_id, QPointF scene_pos);
  void connection_finished(std::string id_out,
                           std::string port_id_out,
                           std::string to_in,
                           std::string port_id_in);
  void connection_started(std::string id_from, std::string port_id_from);

  // --- Graph signals

  void graph_automatic_node_layout_request();
  void graph_clear_request();
  void graph_import_request();
  void graph_load_request();
  void graph_new_request();
  void graph_reload_request();
  void graph_save_as_request();
  void graph_save_request();
  void graph_settings_request();

  // --- Node signals

  void new_graphics_node_request(std::string node_id, QPointF scene_pos);
  void new_node_request(std::string type, QPointF scene_pos);
  void node_deleted(std::string node_id);
  void node_deselected(std::string node_id);
  void node_reload_request(std::string node_id);
  void node_selected(std::string node_id);
  void node_settings_request(std::string node_id);
  void node_right_clicked(std::string node_id, QPointF scene_pos);
  void node_type_dropped(std::string node_type, QPointF scene_pos);
  void nodes_copy_request(std::vector<std::string> id_list,
                          std::vector<QPointF>     scene_pos_list);
  void nodes_duplicate_request(std::vector<std::string> id_list,
                               std::vector<QPointF>     scene_pos_list);
  void nodes_paste_request();

  // --- Global signals

  void quit_request();
  void selection_has_changed();
  void viewport_request();
  void rubber_band_selection_started();
  void rubber_band_selection_finished();

protected:
  // User edits arrive here BEFORE established nodes/links are changed. Override
  // these to delegate to an application editor, which may reject the request or
  // synchronize accepted edits using add_*/erase_*. Do not call the base method
  // when the application owns the edit. The defaults retain the legacy editing
  // behavior and connection_finished/connection_deleted/node_deleted signals.
  // A selection deletion is one request, allowing the editor to batch updates.
  virtual void request_connection(const LinkEndpoints &link);
  virtual void request_deletion(const std::vector<std::string>   &node_ids,
                                const std::vector<LinkEndpoints> &links);

  // --- Qt events

  void contextMenuEvent(QContextMenuEvent *event) override;
  void delete_selected_items();
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private Q_SLOTS:
  void on_connection_dropped(GraphicsNode *from, int port_index, QPointF scene_pos);

  // reordered: 'from' is 'output' and 'to' is 'input'
  void on_connection_finished(GraphicsNode *from_node,
                              int           port_from_index,
                              GraphicsNode *to_node,
                              int           port_to_index);

  void on_connection_started(GraphicsNode *from_node, int port_index);

private:
  void delete_graphics_link(GraphicsLink *,
                            bool link_will_be_replaced = false,
                            bool notify = true);
  void delete_graphics_node(GraphicsNode *p_node, bool notify = true);

  // --- Members

  std::string id;

  // toolbar overlay, parented to the view (not part of the scene)
  QWidget *toolbar_widget = nullptr;

  // all nodes available store as a map of (node type, node category)
  std::map<std::string, std::string> node_inventory;

  GraphicsLink *temp_link = nullptr;   // Temporary link
  GraphicsNode *source_node = nullptr; // Source node for the connection
  LinkType      current_link_type = LinkType::CUBIC;
};

} // namespace gngui
