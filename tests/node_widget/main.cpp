#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QtTest>

#include "gnodegui/graph_viewer.hpp"
#include "gnodegui/graphics_node.hpp"
#include "gnodegui/style.hpp"
#include "gnodegui/utils.hpp"

// windows.h compatibility
#undef IN
#undef OUT

namespace
{

class SimpleProxy : public gngui::NodeProxy
{
public:
  std::string get_id() const override { return this->id; }
  void        set_id(const std::string &value) override { this->id = value; }
  std::string get_caption() const override { return "WidgetNode"; }
  std::string get_category() const override { return "Test"; }
  std::string get_tool_tip_text() const override { return "Widget Node Tooltip"; }
  int         get_nports() const override { return 2; }
  std::string get_port_caption(int port) const override { return port ? "out" : "in"; }
  gngui::PortType get_port_type(int port) const override
  {
    return port ? gngui::PortType::OUT : gngui::PortType::IN;
  }
  std::string get_data_type(int) const override { return "int"; }
  void       *get_data_ref(int) const override { return nullptr; }

private:
  std::string id = "simple_node";
};

} // namespace

class NodeWidgetTest : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void initTestCase()
  {
    GN_STYLE->viewer.add_toolbar = false;
    qRegisterMetaType<std::string>();
  }

  void widget_lifecycle_and_getter()
  {
    auto *proxy = new SimpleProxy();
    auto *node = new gngui::GraphicsNode(proxy);

    // Initially no widget attached
    QCOMPARE(node->get_widget(), nullptr);

    // Attach a widget
    auto *label = new QLabel("Hello Node");
    node->set_widget(label, QSize(100, 30));

    QCOMPARE(node->get_widget(), label);
    QVERIFY(node->get_geometry().full_width > 0);
    QVERIFY(node->get_geometry().full_height > 0);

    // Replace with a new widget
    auto *button = new QPushButton("Click Me");
    node->set_widget(button, QSize(120, 40));

    QCOMPARE(node->get_widget(), button);

    // Visibility toggle
    node->set_widget_visibility(false);
    QVERIFY(!button->isVisible());

    node->set_widget_visibility(true);
    QVERIFY(button->isVisible());

    delete node;
    delete proxy;
  }

  void port_bounds_check()
  {
    auto *proxy = new SimpleProxy();
    auto *node = new gngui::GraphicsNode(proxy);

    // In-bounds checks
    QVERIFY(node->is_port_available(0)); // IN port, not connected
    QVERIFY(node->is_port_available(1)); // OUT port, always available

    // Out-of-bounds checks should safely return false, no crash/assertion
    QVERIFY(!node->is_port_available(-1));
    QVERIFY(!node->is_port_available(2));
    QVERIFY(!node->is_port_available(99));

    // set_is_port_connected out-of-bounds should safely do nothing
    node->set_is_port_connected(-1, nullptr);
    node->set_is_port_connected(99, nullptr);

    delete node;
    delete proxy;
  }

  void clean_delete_unparented()
  {
    // Items created without a scene or already removed
    auto *proxy = new SimpleProxy();
    auto *node = new gngui::GraphicsNode(proxy);

    // Should not crash when deleting item not in scene
    gngui::clean_delete_graphics_item(node);

    delete proxy;
  }
};

QTEST_MAIN(NodeWidgetTest)
#include "main.moc"
