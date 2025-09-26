find_package(Vulkan REQUIRED)

add_library(imgui    
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_vulkan.cpp
    imgui/imgui_demo.cpp
)

target_include_directories(imgui PUBLIC imgui)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan)