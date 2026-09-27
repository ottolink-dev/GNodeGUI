#include "gnodegui/graph_viewer.hpp"
#include "gnodegui/graphics_group.hpp"
#include "gnodegui/style.hpp"

#include <QSignalSpy>
#include <QtTest>

// windows.h, pulled in by QtTest, defines IN and OUT as empty macros
#undef IN
#undef OUT

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

// three nodes side by side, shown so that mouse events go through the view
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
      x += 300;
    }
    this->resize(800, 600);
    this->show();
  }

  ~TestViewer() override
  {
    for (const auto &id : {"a", "b", "c"})
      this->erase_node(id);
  }

  gngui::GraphicsNode *node(const std::string &id)
  {
    return this->get_graphics_node_by_id(id);
  }

  QPoint port_pos(const std::string &id, int port)
  {
    auto *p_node = this->node(id);
    return this->mapFromScene(p_node->scenePos() +
                              p_node->get_geometry().port_rects[port].center());
  }

  QPoint body_pos(const std::string &id)
  {
    auto *p_node = this->node(id);
    return this->mapFromScene(p_node->scenePos() +
                              p_node->get_geometry().header_rect.center());
  }

  void wheel(int delta, int times)
  {
    for (int k = 0; k < times; k++)
    {
      const QPointF pos = this->viewport()->rect().center();
      QWheelEvent   event(pos,
                        this->viewport()->mapToGlobal(pos),
                        QPoint(),
                        QPoint(0, delta),
                        Qt::NoButton,
                        Qt::NoModifier,
                        Qt::NoScrollPhase,
                        false);
      QApplication::sendEvent(this->viewport(), &event);
    }
  }
};

void click(QWidget *viewport, QPoint pos, Qt::KeyboardModifiers modifiers = {})
{
  QTest::mouseMove(viewport, pos);
  QTest::mousePress(viewport, Qt::LeftButton, modifiers, pos);
  QTest::mouseRelease(viewport, Qt::LeftButton, modifiers, pos);
}
} // namespace

class EditorInteractionTest : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void initTestCase()
  {
    GN_STYLE->viewer.add_toolbar = false;
    qRegisterMetaType<std::string>();
  }

  void zoom_is_limited()
  {
    TestViewer viewer;

    viewer.wheel(120, 40);
    QVERIFY(viewer.transform().m11() <= GN_STYLE->viewer.zoom_max + 1e-6);

    // three nodes fit well before zoom_min, which is then the limit
    viewer.wheel(-120, 80);
    QVERIFY(viewer.transform().m11() >= GN_STYLE->viewer.zoom_min - 1e-6);
  }

  void port_click_does_not_select_node()
  {
    TestViewer viewer;

    click(viewer.viewport(), viewer.port_pos("a", 1));
    QVERIFY(!viewer.node("a")->isSelected());

    click(viewer.viewport(), viewer.body_pos("a"));
    QVERIFY(viewer.node("a")->isSelected());
  }

  void ctrl_click_output_selects_its_links()
  {
    TestViewer viewer;
    viewer.add_link("a", "out", "b", "in");
    viewer.add_link("a", "out", "c", "in");
    viewer.node("b")->setSelected(true); // cleared by the click

    click(viewer.viewport(), viewer.port_pos("a", 1), Qt::ControlModifier);

    int selected = 0;
    for (auto *p_link : viewer.get_links())
      if (p_link->isSelected())
      {
        QCOMPARE(p_link->get_node_out(), viewer.node("a"));
        selected++;
      }
    QCOMPARE(selected, 2);
    QVERIFY(!viewer.node("a")->isSelected());
    QVERIFY(!viewer.node("b")->isSelected());
  }

  void select_all_notifies_once()
  {
    TestViewer viewer;
    QSignalSpy node_selected(&viewer, &gngui::GraphViewer::node_selected);
    QSignalSpy selection_changed(&viewer, &gngui::GraphViewer::selection_has_changed);

    viewer.select_all();

    QCOMPARE(node_selected.count(), 0);
    QCOMPARE(selection_changed.count(), 1);
    QVERIFY(viewer.node("b")->isSelected());
  }

  void right_click_releases_mouse_grab()
  {
    TestViewer viewer;
    QTest::mousePress(viewer.viewport(), Qt::RightButton, {}, viewer.body_pos("a"));
    QCOMPARE(viewer.scene()->mouseGrabberItem(), viewer.node("a"));

    // the release goes to the node menu in the application: never sent here
    QCoreApplication::processEvents();
    QCOMPARE(viewer.scene()->mouseGrabberItem(), nullptr);
  }

  void group_wraps_selection_and_ungroups()
  {
    TestViewer viewer;
    viewer.node("a")->setSelected(true);
    viewer.node("b")->setSelected(true);
    viewer.add_group();

    gngui::GraphicsGroup *p_group = nullptr;
    for (auto *item : viewer.scene()->items())
      if (auto *group = dynamic_cast<gngui::GraphicsGroup *>(item))
        p_group = group;
    QVERIFY(p_group);

    const QRectF area = p_group->sceneBoundingRect();
    QVERIFY(area.contains(viewer.node("a")->sceneBoundingRect()));
    QVERIFY(area.contains(viewer.node("b")->sceneBoundingRect()));
    QVERIFY(!area.contains(viewer.node("c")->sceneBoundingRect()));
    QVERIFY(p_group->zValue() < viewer.node("a")->zValue());

    // "Ungroup" is the menu's second entry
    viewer.scene()->clearSelection();
    QTimer::singleShot(50,
                       []()
                       {
                         QWidget *menu = QApplication::activePopupWidget();
                         QTest::keyClick(menu, Qt::Key_Down);
                         QTest::keyClick(menu, Qt::Key_Down);
                         QTest::keyClick(menu, Qt::Key_Return);
                       });
    const QPoint      pos = viewer.mapFromScene(area.bottomLeft() + QPointF(10, -10));
    QContextMenuEvent event(QContextMenuEvent::Mouse,
                            pos,
                            viewer.viewport()->mapToGlobal(pos));
    QApplication::sendEvent(viewer.viewport(), &event);
    QCoreApplication::processEvents();

    for (auto *item : viewer.scene()->items())
      QVERIFY(!dynamic_cast<gngui::GraphicsGroup *>(item));
    QVERIFY(viewer.node("a")->isSelected());
    QVERIFY(viewer.node("b")->isSelected());
    QVERIFY(!viewer.node("c")->isSelected());
  }
};

QTEST_MAIN(EditorInteractionTest)
#include "main.moc"
