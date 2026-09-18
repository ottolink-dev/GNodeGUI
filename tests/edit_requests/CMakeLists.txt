find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(test_edit_requests main.cpp)
target_link_libraries(test_edit_requests PRIVATE gnodegui Qt6::Widgets Qt6::Test
                                                nlohmann_json::nlohmann_json)
set_property(TARGET test_edit_requests PROPERTY AUTOMOC ON)
add_test(NAME graph_edit_requests COMMAND test_edit_requests)
set_tests_properties(graph_edit_requests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
