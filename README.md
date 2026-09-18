# GNodeGUI

GNodeGUI is a C++ graphical node editor library aimed at providing an interface for building and manipulating nodes in a graphical context.
This library is currently used in the [Hesiod](https://github.com/otto-link/Hesiod) GUI, a node-based system for heightmap generation.

![Screenshot_2024-10-09_19-13-17](https://github.com/user-attachments/assets/3362ae46-47ee-4add-b7fd-9f143d8d887c)



## Features

- Node-based graphical interface
- Supports custom node definitions

## Build Instructions

1. Clone the repository:
   ```bash
   git clone --recurse-submodules https://github.com/otto-link/GNodeGUI.git
   ```
2. Build using CMake:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

## Application-owned graph editing

`GraphViewer` can delegate connection and deletion gestures to an application
editor. Override these protected methods:

```cpp
void request_connection(const gngui::LinkEndpoints &link) override;
void request_deletion(const std::vector<std::string> &node_ids,
                      const std::vector<gngui::LinkEndpoints> &links) override;
```

Requests arrive before established graphics nodes or links are changed. Connection
endpoints are ordered output to input, regardless of drag direction. The temporary
drag line is removed before the request. An occupied input retains its existing
link until the editor accepts a replacement. Delete and Ctrl + right-click use the
same deletion request; deleting a selection produces one batch of IDs and explicitly
selected links. Incident links need not be individually selected.

The override owns validation, model mutation, error handling and synchronization.
It may reject a request by leaving the graph and scene unchanged. For accepted
edits, use `add_node` / `add_link` and `erase_node` / `erase_link` to synchronize the
scene. Erasing a node also erases its incident graphics links. The erase methods
return whether an item existed and emit no `node_deleted` or `connection_deleted`
signals. Selection notifications still work. Keep the model proxies valid until
their graphics items have been removed. Request arguments are owned values, but an
override must copy them if it retains a request after returning.

Do not call the base request implementation when the application owns the edit.
The defaults preserve standalone editing and the legacy after-edit signals.
Existing `remove_node` / `remove_link` methods also retain those signals for
compatibility. These legacy signals are not a second command path for a controller
using the request overrides.

## Regression tests

```sh
cmake -S . -B build -DGNODEGUI_ENABLE_TESTS=ON
cmake --build build --target test_edit_requests
ctest --test-dir build --output-on-failure
```

Tests require Qt Test and run with the offscreen platform plugin. They cover
accepted and rejected requests, input replacement, both drag directions, batched
deletion, Ctrl + right-click and legacy notifications. The interactive example
is built as `graph_viewer_demo` (`test` is reserved for CTest).

## License

This project is licensed under the GPL-3.0 license.
