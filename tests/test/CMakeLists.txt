add_executable(graph_viewer_demo main.cpp signal_signatures.cpp)
target_link_libraries(graph_viewer_demo gnodegui Qt6::Core Qt6::Widgets
                                      nlohmann_json::nlohmann_json)
set_property(TARGET graph_viewer_demo PROPERTY AUTOMOC ON)
