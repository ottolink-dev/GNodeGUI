find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(test_editor_interaction main.cpp)
target_link_libraries(test_editor_interaction PRIVATE gnodegui Qt6::Widgets Qt6::Test
                                                      nlohmann_json::nlohmann_json)
set_property(TARGET test_editor_interaction PROPERTY AUTOMOC ON)
add_test(NAME editor_interaction COMMAND test_editor_interaction)
set_tests_properties(editor_interaction PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
