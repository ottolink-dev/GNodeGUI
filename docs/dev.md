# Development & Refactoring Log

This document tracks codebase investigation, architectural reasoning, and modifications implemented on branch `refactor/codebase-consolidation`.

---

## 1. Codebase Investigation Summary

### 1.1 Lifetime & Ownership Consolidation
- **Embedded `QWidget` & `QGraphicsProxyWidget`**:
  - `GraphicsNode::get_widget()` was added in commit `19597b8` with WIP note about broken behavior.
  - In `GraphicsNode::paint()`, `current_widget_size` was compared to `get_widget_size()` to trigger `update_geometry()`, but `current_widget_size` was never assigned the new size. This caused `update_geometry()` to run on every paint frame whenever an embedded widget was present.
  - `set_widget_visibility` only toggled the internal `QWidget` without updating the `QGraphicsProxyWidget`'s visibility or accounting for widget visibility in `get_widget_size()`.
- **Port Bounds Safety & Link Connection**:
  - `set_is_port_connected` and `is_port_available` lacked boundary verification against `connected_link_ref` size and `get_nports()`. Negative or out-of-range port indices could lead to buffer overruns or invalid vector indexing.
- **Scene Cleanup Safety**:
  - `clean_delete_graphics_item(item)` had an aggressive check `if (!item || !item->scene()) return;` which prematurely returned when deleting unparented graphics items or items that had already been removed from the scene via `scene()->removeItem()`. This caused memory leaks during operations such as `GraphViewer::clear()`.

### 1.2 Performance & Redundant Allocations
- **ContextMenu Item Sorting & Ownership**:
  - `GraphViewer::execute_new_node_context_menu()` allocated `QMenu *menu = new QMenu(this)` directly on the heap and relied on non-scoped manual creation.
  - Category sorting copied pairs iteratively using a non-standard lambda capturing by value `[=]`. We modernized it with `std::sort`, move semantics, and `std::unique_ptr<QMenu>`.
- **Const Accessors**:
  - `GraphicsLink` exposed `get_node_out()` and `get_node_in()` only as mutable pointers, preventing usage in const contexts (such as const inspection of links in testing and graph inspection). Added `const GraphicsNode*` overloads and initialized port indices to `-1`.

---

## 2. Commit Log & Changes

| Commit | Description | File(s) Changed |
| :--- | :--- | :--- |
| `eb86767` | `fix(graphics_node): fix widget size caching and visibility handling` | [graphics_node.cpp](file:///home/glop/Documents/dev/GNodeGUI/GNodeGUI/src/graphics_node.cpp) |
| `cd2e0a8` | `fix(utils): allow clean_delete_graphics_item on items already unparented from scene` | [utils.cpp](file:///home/glop/Documents/dev/GNodeGUI/GNodeGUI/src/utils.cpp) |
| `eb758e6` | `fix(graphics_node): add bounds checks for port availability and connected links` | [graphics_node.cpp](file:///home/glop/Documents/dev/GNodeGUI/GNodeGUI/src/graphics_node.cpp) |
| `da6fce3` | `refactor(graph_viewer): modernize context menu lifecycle and sorting` | [graph_viewer.cpp](file:///home/glop/Documents/dev/GNodeGUI/GNodeGUI/src/graph_viewer.cpp) |
| `9f3ea34` | `refactor(graphics_link): add const accessors and member default initialization` | [graphics_link.hpp](file:///home/glop/Documents/dev/GNodeGUI/GNodeGUI/include/gnodegui/graphics_link.hpp) |
| `8e1a31b` | `test: add unit tests for node widget lifecycle and port safety` | [tests/node_widget/CMakeLists.txt](file:///home/glop/Documents/dev/GNodeGUI/tests/node_widget/CMakeLists.txt), [tests/node_widget/main.cpp](file:///home/glop/Documents/dev/GNodeGUI/tests/node_widget/main.cpp) |

---

## 3. Test Coverage & Verification

All automated tests build and pass:
1. `graph_edit_requests`: Link and node edit request lifecycle.
2. `editor_interaction`: Zoom limits, selection, and viewport interactions.
3. `node_widget`: Embedded widget lifecycle, visibility toggles, port availability bounds checks, and unparented graphics item cleanup.

---

## 4. API Stability & Guidelines for Consumers

- **API Compatibility**: All public APIs and signals remain backwards-compatible.
- **Const Correctness**: Consumers can now query `GraphicsLink::get_node_out()` and `GraphicsLink::get_node_in()` on `const GraphicsLink *`.
- **Embedded Widget Guidelines**:
  - `GraphicsNode::get_widget()` returns `nullptr` when no widget is set or when the widget has been destroyed.
  - Setting widget visibility via `GraphicsNode::set_widget_visibility(bool)` now automatically synchronizes the `QGraphicsProxyWidget` and recomputes the node layout seamlessly.
