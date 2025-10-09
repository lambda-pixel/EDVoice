find_package(Vulkan REQUIRED)

if (USE_SDL)
    add_library(imgui
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_sdl3.cpp
        imgui/backends/imgui_impl_vulkan.cpp
        #imgui/imgui_demo.cpp
    )
else()
    add_library(imgui
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_win32.cpp
        imgui/backends/imgui_impl_vulkan.cpp
        imgui/backends/imgui_impl_dx11.cpp
        imgui/imgui_demo.cpp
    )
endif()

target_include_directories(imgui PUBLIC imgui)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan)

if (USE_SDL)
    target_link_libraries(imgui PUBLIC SDL3::SDL3)
endif()
