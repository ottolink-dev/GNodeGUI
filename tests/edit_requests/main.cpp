#include <functional>

#include <QSignalSpy>
#include <QtTest>

#include "gnodegui/graph_viewer.hpp"
#include "gnodegui/style.hpp"

using gngui::LinkEndpoints;

namespace
{
class TestNode : public gngui::NodeProxy
{
public:
  std::string get_id() const override { return id; }
  void        set_id(const std::string &value) override { id = value; }
  std::string get_caption() const override { return "Test"; }
  std::string get_category() const override { return "Test"; }
  std::string get_tool_tip_text() const override { return {}; }
  int         get_nports() const override { return 2; }
  std::string get_port_caption(int port) const override { return port ? "out" : "in"; }
  gngui::PortType get_port_type(int port) const override
  {
    return port ? gngui::PortType::OUT : gngui::PortType::IN;
  }
  std::string get_data_type(int) const override { return "float"; }
  void       *get_data_ref(int) const override { return nullptr; }

private:
  std::string id;
};

class TestViewer : public gngui::GraphViewer
{
public:
  TestViewer()
  {
    this->scene()->setParent(this);
    int x = 0;
    for (const auto &id : {"a", "b", "c"})
    {
      auto *proxy = new TestNode;
      proxy->setParent(this);
      this->add_node(proxy, QPointF(x, 0), id);
      x += 400;
    }
  }

  ~TestViewer() override
  {
    for (const auto &id : {"a", "b", "c"})
      this->erase_node(id);
  }

  // Exercise the callbacks installed on real GraphicsNodes by GraphViewer.
  void drag(const std::string &from, int from_port, const std::string &to, int to_port)
  {
    auto *source = this->get_graphics_node_by_id(from);
    auto *target = this->get_graphics_node_by_id(to);
    source->connection_started(source, from_port);
    source->connection_finished(source, from_port, target, to_port);
  }

  void delete_selection()
  {
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
    QApplication::sendEvent(this, &event);
  }

  std::function<void(const LinkEndpoints &)> connect_request;
  std::function<void(const std::vector<std::string> &,
                     const std::vector<LinkEndpoints> &)>
      delete_request;

protected:
  void request_connection(const LinkEndpoints &link) override
  {
    if (connect_request)
      connect_request(link);
    else
      GraphViewer::request_connection(link);
  }

  void request_deletion(const std::vector<std::string>   &ids,
                        const std::vector<LinkEndpoints> &links) override
  {
    if (delete_request)
      delete_request(ids, links);
    else
      GraphViewer::request_deletion(ids, links);
  }
};

LinkEndpoints link_endpoints(gngui::GraphicsLink *link)
{
  return {link->get_node_out()->get_id(),
          link->get_node_out()->get_port_id(link->get_port_out_index()),
          link->get_node_in()->get_id(),
          link->get_node_in()->get_port_id(link->get_port_in_index())};
}
} // namespace

class EditRequestsTest : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void initTestCase()
  {
    GN_STYLE->viewer.add_toolbar = false;
    qRegisterMetaType<std::string>();
  }

  void rejected_connection_data()
  {
    QTest::addColumn<bool>("reverse");
    QTest::newRow("output-to-input") << false;
    QTest::newRow("input-to-output") << true;
  }

  void rejected_connection()
  {
    QFETCH(bool, reverse);
    TestViewer viewer;
    QSignalSpy completed(&viewer, &gngui::GraphViewer::connection_finished);
    int        requests = 0;
    viewer.connect_request = [&](const LinkEndpoints &link)
    {
      ++requests;
      QVERIFY((link == LinkEndpoints{"a", "out", "b", "in"}));
      QVERIFY(viewer.get_links().empty()); // temporary drag was already removed
    };

    if (reverse)
      viewer.drag("b", 0, "a", 1);
    else
      viewer.drag("a", 1, "b", 0);

    QCOMPARE(requests, 1);
    QVERIFY(viewer.get_links().empty());
    QCOMPARE(completed.count(), 0);

    // Rejection must leave the next gesture usable.
    viewer.connect_request = {};
    viewer.drag("a", 1, "b", 0);
    QCOMPARE(viewer.get_links().size(), size_t(1));
    QCOMPARE(completed.count(), 1);
  }

  void rejected_replacement_preserves_existing_link()
  {
    TestViewer viewer;
    viewer.add_link("a", "out", "b", "in");
    QSignalSpy deleted(&viewer, &gngui::GraphViewer::connection_deleted);
    QSignalSpy completed(&viewer, &gngui::GraphViewer::connection_finished);
    int        requests = 0;
    viewer.connect_request = [&](const LinkEndpoints &link)
    {
      ++requests;
      QVERIFY((link == LinkEndpoints{"c", "out", "b", "in"}));
      QCOMPARE(viewer.get_links().size(), size_t(1));
    };
    viewer.drag("c", 1, "b", 0);

    QCOMPARE(requests, 1);
    QCOMPARE(viewer.get_links().size(), size_t(1));
    QVERIFY((link_endpoints(viewer.get_links().front()) ==
             LinkEndpoints{"a", "out", "b", "in"}));
    QCOMPARE(deleted.count(), 0);
    QCOMPARE(completed.count(), 0);
    QVERIFY(!viewer.get_graphics_node_by_id("b")->is_port_available(0));
  }

  void accepted_replacement_does_not_echo_edit_notifications()
  {
    TestViewer viewer;
    viewer.add_link("a", "out", "b", "in");
    QSignalSpy deleted(&viewer, &gngui::GraphViewer::connection_deleted);
    QSignalSpy completed(&viewer, &gngui::GraphViewer::connection_finished);
    int        requests = 0;
    viewer.connect_request = [&](const LinkEndpoints &link)
    {
      ++requests;
      QVERIFY(viewer.erase_link({"a", "out", "b", "in"}));
      viewer.add_link(link.node_out, link.port_out, link.node_in, link.port_in);
    };
    viewer.drag("c", 1, "b", 0);

    QCOMPARE(requests, 1);
    QCOMPARE(viewer.get_links().size(), size_t(1));
    QVERIFY((link_endpoints(viewer.get_links().front()) ==
             LinkEndpoints{"c", "out", "b", "in"}));
    QCOMPARE(deleted.count(), 0);
    QCOMPARE(completed.count(), 0);
  }

  void invalid_gesture_cleans_temporary_link_data()
  {
    QTest::addColumn<bool>("same_node");
    QTest::newRow("same-node") << true;
    QTest::newRow("same-direction") << false;
  }

  void invalid_gesture_cleans_temporary_link()
  {
    QFETCH(bool, same_node);
    TestViewer viewer;
    int        requests = 0;
    viewer.connect_request = [&](const LinkEndpoints &) { ++requests; };
    viewer.drag("a", 1, same_node ? "a" : "b", same_node ? 0 : 1);
    QCOMPARE(requests, 0);
    QVERIFY(viewer.get_links().empty());
    viewer.drag("a", 1, "b", 0);
    QCOMPARE(requests, 1);
    QVERIFY(viewer.get_links().empty());
  }

  void selection_deletion_is_one_request_data()
  {
    QTest::addColumn<bool>("accept");
    QTest::newRow("reject") << false;
    QTest::newRow("accept") << true;
  }

  void selection_deletion_is_one_request()
  {
    QFETCH(bool, accept);
    TestViewer viewer;
    viewer.add_link("a", "out", "b", "in");
    viewer.add_link("b", "out", "c", "in");
    viewer.set_node_as_selected("a");
    viewer.set_node_as_selected("b");
    for (auto *link : viewer.get_links())
      link->setSelected(true);

    QSignalSpy deleted_nodes(&viewer, &gngui::GraphViewer::node_deleted);
    QSignalSpy deleted_links(&viewer, &gngui::GraphViewer::connection_deleted);
    QSignalSpy selection(&viewer, &gngui::GraphViewer::selection_has_changed);
    int        requests = 0;
    viewer.delete_request =
        [&](const std::vector<std::string> &ids, const std::vector<LinkEndpoints> &links)
    {
      ++requests;
      QCOMPARE(ids.size(), size_t(2));
      QCOMPARE(links.size(), size_t(2));
      QVERIFY(viewer.get_graphics_node_by_id("a"));
      QVERIFY(viewer.get_graphics_node_by_id("b"));
      QCOMPARE(viewer.get_links().size(), size_t(2));
      if (accept)
      {
        // Node removal cleans incident links; subsequent explicit link erasures
        // must be harmless, even when both the node and its link were selected.
        for (const auto &id : ids)
          QVERIFY(viewer.erase_node(id));
        for (const auto &link : links)
          QVERIFY(!viewer.erase_link(link));
      }
    };
    viewer.delete_selection();

    QCOMPARE(requests, 1);
    QCOMPARE(viewer.get_links().size(), accept ? size_t(0) : size_t(2));
    QCOMPARE(viewer.get_graphics_node_by_id("a") == nullptr, accept);
    QVERIFY(viewer.get_graphics_node_by_id("c"));
    QCOMPARE(viewer.get_graphics_node_by_id("c")->is_port_available(0), accept);
    QCOMPARE(deleted_nodes.count(), 0);
    QCOMPARE(deleted_links.count(), 0);
    QVERIFY(selection.count() > 0);
  }

  void control_click_requests_deletion_data()
  {
    QTest::addColumn<bool>("link");
    QTest::newRow("node") << false;
    QTest::newRow("link") << true;
  }

  void control_click_requests_deletion()
  {
    QFETCH(bool, link);
    TestViewer viewer;
    viewer.add_link("a", "out", "b", "in");
    viewer.resize(1000, 600);
    viewer.show();
    viewer.centerOn(200, 0);
    int requests = 0;
    viewer.delete_request =
        [&](const std::vector<std::string> &ids, const std::vector<LinkEndpoints> &links)
    {
      ++requests;
      QCOMPARE(ids.size(), link ? size_t(0) : size_t(1));
      QCOMPARE(links.size(), link ? size_t(1) : size_t(0));
    };
    const QPointF
        pos = link ? viewer.get_links().front()->path().pointAtPercent(0.5)
                   : viewer.get_graphics_node_by_id("a")->sceneBoundingRect().center();
    QTest::mouseClick(viewer.viewport(),
                      Qt::RightButton,
                      Qt::ControlModifier,
                      viewer.mapFromScene(pos));
    QCOMPARE(requests, 1);
    QVERIFY(viewer.get_graphics_node_by_id("a"));
    QCOMPARE(viewer.get_links().size(), size_t(1));
  }

  void legacy_connection_and_replacement()
  {
    TestViewer viewer;
    QSignalSpy completed(&viewer, &gngui::GraphViewer::connection_finished);
    QSignalSpy deleted(&viewer, &gngui::GraphViewer::connection_deleted);
    viewer.drag("b", 0, "a", 1);
    QCOMPARE(completed.count(), 1);
    QCOMPARE(deleted.count(), 0);
    viewer.drag("c", 1, "b", 0);
    QCOMPARE(completed.count(), 2);
    QCOMPARE(deleted.count(), 1);
    QVERIFY(deleted.front().at(4).toBool()); // replacement defers the legacy update
    QCOMPARE(viewer.get_links().size(), size_t(1));
    QVERIFY((link_endpoints(viewer.get_links().front()) ==
             LinkEndpoints{"c", "out", "b", "in"}));
  }

  void legacy_node_deletion_notifies_once()
  {
    TestViewer viewer;
    viewer.add_link("a", "out", "b", "in");
    viewer.add_link("b", "out", "c", "in");
    QSignalSpy nodes(&viewer, &gngui::GraphViewer::node_deleted);
    QSignalSpy links(&viewer, &gngui::GraphViewer::connection_deleted);
    viewer.set_node_as_selected("b");
    viewer.get_links().front()->setSelected(true);
    viewer.delete_selection();
    QCOMPARE(nodes.count(), 1);
    QCOMPARE(links.count(), 2);
    QVERIFY(viewer.get_links().empty());
    QVERIFY(!viewer.get_graphics_node_by_id("b"));
    QVERIFY(viewer.get_graphics_node_by_id("c")->is_port_available(0));
  }

  void missing_scene_items_are_harmless()
  {
    TestViewer viewer;
    QVERIFY(!viewer.erase_node("missing"));
    QVERIFY(!viewer.erase_link({"a", "out", "b", "in"}));
    QVERIFY(viewer.erase_node("a"));
    QVERIFY(!viewer.erase_node("a"));
  }
};

QTEST_MAIN(EditRequestsTest)
#include "main.moc"
