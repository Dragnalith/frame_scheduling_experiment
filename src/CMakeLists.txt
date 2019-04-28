
list(APPEND MAIN_APP_SOURCES
        main.cpp
        imgui_impl_opengl3.h
        imgui_impl_opengl3.cpp
        glad/glad.h
        glad/glad.c
        KHR/khrplatform.h
        visualizer.h
        visualizer.cpp
        node_editor.cpp
        node_editor.h
        simulator.h
        simulator.cpp
        app.h
        )

set(MAIN_APP_LIBRARIES
        imgui
        glfw
        NodeEditor
        )

list(APPEND MAIN_APP_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
        )

set(NAME fcsim)
add_executable(${NAME} ${MAIN_APP_SOURCES})
target_compile_definitions(${NAME} PUBLIC IMGUI_DEFINE_MATH_OPERATORS)
target_include_directories(${NAME} PUBLIC ${MAIN_APP_INCLUDE_DIRS})
target_link_libraries(${NAME} ${MAIN_APP_LIBRARIES})

