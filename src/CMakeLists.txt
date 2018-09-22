
list(APPEND MAIN_APP_SOURCES
        main.cpp
        imgui_impl_opengl3.h
        imgui_impl_opengl3.cpp
        glad/glad.h
        glad/glad.c
        KHR/khrplatform.h
        )

set(MAIN_APP_LIBRARIES
        imgui
        glfw
        )

list(APPEND MAIN_APP_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
        )

set(NAME fcsim)
add_executable(${NAME} ${MAIN_APP_SOURCES})
target_include_directories(${NAME} PUBLIC ${MAIN_APP_INCLUDE_DIRS})
target_link_libraries(${NAME} ${MAIN_APP_LIBRARIES})

