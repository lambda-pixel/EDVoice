find_package(Vulkan REQUIRED)

set(IMGUI_SRC
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp
)

if (USE_SDL)
    set(IMGUI_SRC ${IMGUI_SRC}
        imgui/backends/imgui_impl_sdl3.cpp
    )
else()
    set(IMGUI_SRC ${IMGUI_SRC}
        imgui/backends/imgui_impl_win32.cpp
    )
endif()

if (USE_VULKAN)
    set(IMGUI_SRC ${IMGUI_SRC}
        imgui/backends/imgui_impl_vulkan.cpp
    )
else()
    set(IMGUI_SRC ${IMGUI_SRC}
        imgui/backends/imgui_impl_dx11.cpp
    )
endif()


add_library(imgui ${IMGUI_SRC})
target_include_directories(imgui PUBLIC imgui)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan)

if (USE_SDL)
    target_link_libraries(imgui PUBLIC SDL3::SDL3)
endif()
