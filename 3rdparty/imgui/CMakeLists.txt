find_package(Vulkan REQUIRED)

if (WIN32)
    add_library(imgui
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_win32.cpp
        imgui/backends/imgui_impl_vulkan.cpp
        imgui/imgui_demo.cpp
    )
else()
    add_library(imgui
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_sdl3.cpp
        imgui/backends/imgui_impl_vulkan.cpp
        imgui/imgui_demo.cpp
    )
endif()

target_include_directories(imgui PUBLIC imgui)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan)