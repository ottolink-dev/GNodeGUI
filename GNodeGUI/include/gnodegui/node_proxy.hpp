/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */

/* This file contains the declaration of the `NodeProxy` struct, which represents an
 * abstract interface for nodes with input and output ports. The class provides methods
 * for accessing the node's ID, label, port count, and port types, making it useful for
 * building node-based systems such as flow diagrams or signal processing pipelines.
 */
#pragma once
#include <memory>

#include <QObject>
#include <QWidget>

#include <string>

namespace gngui
{

enum PortType
{
  IN,  ///< Input port type.
  OUT, ///< Output port type.
};

/**
 * The `NodeProxy` struct is designed to represent a node with input and output ports.
 * It provides an interface for accessing and manipulating node-related information,
 * such as the node ID, labels, port counts, and port types. This is an abstract class,
 * so it cannot be instantiated directly.
 */
class NodeProxy : public QObject
{
  Q_OBJECT
public:
  NodeProxy() = default;
  virtual ~NodeProxy() = default;

  // --- ID / Identity
  virtual std::string get_id() const = 0;
  virtual void        set_id(const std::string &new_id) = 0;

  // --- Node description
  virtual std::string get_caption() const = 0;
  virtual std::string get_category() const = 0;
  virtual std::string get_tool_tip_text() const = 0;

  // --- Ports
  virtual int         get_nports() const = 0;
  virtual std::string get_port_caption(int port_index) const = 0;
  virtual std::string get_comment() const;
  virtual std::string get_port_id(int port_index) const;
  virtual PortType    get_port_type(int port_index) const = 0;
  virtual std::string get_data_type(int port_index) const = 0;
  virtual void       *get_data_ref(int /*port_index*/) const = 0;

  // --- Debugging
  void log_debug();

Q_SIGNALS:
  void ports_changed();
};

template <typename ModelNode> class TypedNodeProxy : public NodeProxy
{
public:
  TypedNodeProxy(std::weak_ptr<ModelNode> model) : model(model) {}

  std::shared_ptr<ModelNode> get_model() const { return this->model.lock(); }

  // --- ID / Identity
  std::string get_id() const override
  {
    if (auto m = this->model.lock())
      return m->get_id();
    return {};
  }

  void set_id(const std::string &new_id) override
  {
    if (auto m = this->model.lock())
      m->set_id(new_id);
  }

  // --- Node description
  std::string get_caption() const override
  {
    if (auto m = this->model.lock())
      return m->get_caption();
    return {};
  }

  std::string get_category() const override
  {
    if (auto m = this->model.lock())
      return m->get_category();
    return {};
  }

  std::string get_comment() const override
  {
    if (auto m = this->model.lock())
      return m->get_comment();
    return {};
  }

  std::string get_tool_tip_text() const override
  {
    if (auto m = this->model.lock())
      return m->get_tool_tip_text();
    return {};
  }

  // --- Ports
  int get_nports() const override
  {
    if (auto m = this->model.lock())
      return m->get_nports();
    return 0;
  }

  std::string get_port_caption(int port_index) const override
  {
    if (auto m = this->model.lock())
      return m->get_port_caption(port_index);
    return {};
  }

  PortType get_port_type(int port_index) const override
  {
    if (auto m = this->model.lock())
      return m->get_port_type(port_index);
    return PortType::IN; // default fallback
  }

  std::string get_data_type(int port_index) const override
  {
    if (auto m = this->model.lock())
      return m->get_data_type(port_index);
    return {};
  }

  void *get_data_ref(int port_index) const override
  {
    if (auto m = this->model.lock())
      return m->get_data_ref(port_index);
    return nullptr;
  }

private:
  std::weak_ptr<ModelNode> model;
};

} // namespace gngui
