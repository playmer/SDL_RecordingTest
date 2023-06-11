cmake_minimum_required(VERSION 3.15)

project(RecordingExample)

add_executable(RecordingExample)

target_sources(RecordingExample
PRIVATE
    main.cpp
    imgui/imgui.cpp
    imgui/imgui.h
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_internal.h
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imstb_rectpack.h
    imgui/imstb_textedit.h
    imgui/imstb_truetype.h
)

target_sources(RecordingExample
PRIVATE

    imgui/imgui_impl_sdl2.cpp
    imgui/imgui_impl_sdl2.h
    imgui/imgui_impl_sdlrenderer2.cpp
    imgui/imgui_impl_sdlrenderer2.h

    #imgui/imgui_impl_sdl3.cpp
    #imgui/imgui_impl_sdl3.h
    #imgui/imgui_impl_sdlrenderer3.cpp
    #imgui/imgui_impl_sdlrenderer3.h
)

target_include_directories(RecordingExample PRIVATE imgui ../SDL-1/include)

add_subdirectory(SDL-1)

set_target_properties(
    SDL2 
    RecordingExample 
PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY C:/repos/SDLRecording/SDL_RecordingTest/out/build/x64-Debug/bin
    LIBRARY_OUTPUT_DIRECTORY C:/repos/SDLRecording/SDL_RecordingTest/out/build/x64-Debug/bin
    RUNTIME_OUTPUT_DIRECTORY C:/repos/SDLRecording/SDL_RecordingTest/out/build/x64-Debug/bin
)

target_link_libraries(RecordingExample 
PRIVATE 
    SDL2::SDL2
    SDL2::SDL2main
)
