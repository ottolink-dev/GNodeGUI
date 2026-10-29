find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(test_node_widget main.cpp)
target_link_libraries(test_node_widget PRIVATE gnodegui Qt6::Widgets Qt6::Test
                                              nlohmann_json::nlohmann_json)
set_property(TARGET test_node_widget PROPERTY AUTOMOC ON)
add_test(NAME node_widget COMMAND test_node_widget)
set_tests_properties(node_widget PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
