idf_component_register(SRCS "gui_task.cpp" "network_task.cpp"
                        INCLUDE_DIRS "."
                        REQUIRES arduino-esp32 coffee-driver lvgl ui)
