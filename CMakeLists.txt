cmake_minimum_required(VERSION 3.20)
project(3DRender)

set(CMAKE_CXX_STANDARD 17)

# 尋找依賴
find_package(GLEW CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

# 定義 ImGui 源文件
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)

# 添加可執行文件
add_executable(3DRender
    main.cpp
    Shader.cpp
    Camera.cpp
    AABB.cpp
    OBB.cpp
    object.cpp
    physicManager.cpp
    ${IMGUI_SOURCES}
)

# 鏈接庫
target_link_libraries(3DRender PRIVATE
    glfw
    GLEW::GLEW
    glm::glm
    OpenGL::GL
)

# 添加 ImGui 頭文件路徑
target_include_directories(3DRender PRIVATE
    ${IMGUI_DIR}
)

# 複製資源文件
set(RESOURCE_FILES
    picSource/grid.jpg
    picSource/container.jpg
    vertexShaderSource.vert
    fragmentShaderSource.frag
)
foreach(RESOURCE ${RESOURCE_FILES})
    configure_file(${CMAKE_SOURCE_DIR}/${RESOURCE} ${CMAKE_BINARY_DIR}/Release/${RESOURCE} COPYONLY)
    configure_file(${CMAKE_SOURCE_DIR}/${RESOURCE} ${CMAKE_BINARY_DIR}/Debug/${RESOURCE} COPYONLY)
endforeach()

add_custom_target(run
    COMMAND ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}
    DEPENDS ${CMAKE_PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running ${CMAKE_PROJECT_NAME}..."
)