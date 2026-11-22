#include "gnodegui/graph_viewer.hpp"
#include "gnodegui/graphics_group.hpp"
#include "gnodegui/style.hpp"

#include <QLineEdit>
#include <QMenu>
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

  void new_node_menu_filtering_caps_restores_and_reanchors()
  {
    TestViewer viewer;

    std::map<std::string, std::string> inventory;
    for (int k = 0; k < 100; ++k)
    {
      inventory["Alpha_" + std::to_string(k)] = "CatA/Sub1";
      inventory["Beta_" + std::to_string(k)] = "CatB/Sub2";
      inventory["Flow_" + std::to_string(k)] = "CatC";
    }
    viewer.set_node_inventory(inventory);

    GN_STYLE->viewer.max_filtered_nodes = 15;
    QSignalSpy new_node_spy(&viewer, &gngui::GraphViewer::new_node_request);

    QCursor::setPos(100, 100);

    QTimer::singleShot(
        50,
        []()
        {
          QMenu *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
          QVERIFY(menu);

          // Ensure the menu Always closes when this lambda returns (even if a QCOMPARE
          // fails)
          const auto guard = qScopeGuard([menu]() { menu->close(); });

          QLineEdit *filter_box = menu->findChild<QLineEdit *>();
          QVERIFY(filter_box);

          const QPoint initial_pos = menu->pos();

          // --- Broad filter ('e' matches Beta_0..99 = 100 nodes)

          filter_box->setText("e");
          QCoreApplication::processEvents();

          std::vector<QAction *> visible_actions;
          for (QAction *action : menu->actions())
            if (action->isVisible())
              visible_actions.push_back(action);

          // 1 QWidgetAction (filter_box) + 15 capped items + 1 "...and 85 more"
          QCOMPARE(visible_actions.size(), size_t(17));
          QVERIFY(!visible_actions.back()->isEnabled());
          QCOMPARE(visible_actions.back()->text(), QStringLiteral("…and 85 more"));
          QCOMPARE(menu->pos(), initial_pos);

          // --- Clear filter restores original top-level categories (CatA, CatB, CatC)

          filter_box->clear();
          QCoreApplication::processEvents();

          visible_actions.clear();
          for (QAction *action : menu->actions())
            if (action->isVisible())
              visible_actions.push_back(action);

          // 1 QWidgetAction + 3 top-level category submenus
          QCOMPARE(visible_actions.size(), size_t(4));
          QVERIFY(visible_actions[1]->menu() != nullptr);
          QCOMPARE(menu->pos(), initial_pos);

          // --- Typing while a category submenu is open forwards to filter_box

          QAction *cat_c_action = visible_actions[3];
          QMenu   *cat_c_menu = cat_c_action->menu();
          QVERIFY(cat_c_menu);
          menu->setActiveAction(cat_c_action);
          cat_c_menu->popup(menu->pos() + QPoint(100, 50));
          QCoreApplication::processEvents();
          QVERIFY(cat_c_menu->isVisible());

          QTest::keyClick(cat_c_menu, Qt::Key_F);
          QTest::keyClick(cat_c_menu, Qt::Key_L);
          QCoreApplication::processEvents();
          QVERIFY(!cat_c_menu->isVisible());
          QCOMPARE(filter_box->text(), QStringLiteral("fl"));

          filter_box->clear();
          QCoreApplication::processEvents();

          // --- Narrow filter + Return key selects first matching node

          filter_box->setText("Flow_42");
          QCoreApplication::processEvents();

          visible_actions.clear();
          for (QAction *action : menu->actions())
            if (action->isVisible())
              visible_actions.push_back(action);

          // 1 QWidgetAction + 1 exact match ("Flow_42")
          QCOMPARE(visible_actions.size(), size_t(2));
          QCOMPARE(visible_actions[1]->text(), QStringLiteral("Flow_42"));
          QCOMPARE(menu->pos(), initial_pos);

          QTest::keyClick(filter_box, Qt::Key_Return);
        });

    const bool created = viewer.execute_new_node_context_menu();
    QVERIFY(created);
    QCOMPARE(new_node_spy.count(), 1);
    QCOMPARE(new_node_spy.takeFirst().at(0).value<std::string>(), std::string("Flow_42"));
  }
};

QTEST_MAIN(EditorInteractionTest)
#include "main.moc"
