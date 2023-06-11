// Dear ImGui: standalone example application for SDL2 + SDL_Renderer
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// Important to understand: SDL_Renderer is an _optional_ component of SDL2.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and SDL+OpenGL on Linux/OSX.

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <stdio.h>
#include <SDL.h>
#include <string>
#include <vector>

#include <mutex>

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

struct AudioDevice
{
  AudioDevice(int deviceIndex, bool isRecordingDevice)
    : mName{ SDL_GetAudioDeviceName(deviceIndex, isRecordingDevice)}
    , mIsRecordingDevice{isRecordingDevice}
  {
    if (isRecordingDevice)
      mFriendlyName = std::string("Capture Device: ") + mName;
    else
      mFriendlyName = std::string("Playback Device: ") + mName;
  }

  std::string mName;
  std::string mFriendlyName;
  bool mIsRecordingDevice;
};

std::vector<AudioDevice> GetAllDevices()
{
  std::vector<AudioDevice> devices;
  int nDevices = SDL_GetNumAudioDevices(SDL_FALSE);
  for (int i = 0; i < nDevices; i++)
  {
    devices.emplace_back(i, false);
  }

  nDevices = SDL_GetNumAudioDevices(SDL_TRUE);
  for (int i = 0; i < nDevices; i++)
  {
    devices.emplace_back(i, true);
  }

  return devices;
}

struct DeviceCapture
{
  DeviceCapture(const AudioDevice& deviceToCapture)
  {
    SDL_AudioSpec desiredRecordingSpec;
    SDL_zero(desiredRecordingSpec);
    desiredRecordingSpec.freq = 44100;
    desiredRecordingSpec.format = AUDIO_S32;
    desiredRecordingSpec.channels = 2;
    desiredRecordingSpec.samples = 4096;
    desiredRecordingSpec.userdata = this;
    desiredRecordingSpec.callback = audioRecordingCallback;

    mDeviceId = SDL_OpenAudioDevice(deviceToCapture.mName.c_str(), SDL_TRUE, &desiredRecordingSpec, &mReceivedRecordingSpec, 0);

    if (mDeviceId == 0)
    {
      //Report error
      printf("Failed to open recording device! SDL Error: %s\n", SDL_GetError());
    }

    SDL_PauseAudioDevice(mDeviceId, SDL_FALSE);
  }

  ~DeviceCapture()
  {
    SDL_CloseAudioDevice(mDeviceId);
  }

  //Recording/playback callbacks
  static void audioRecordingCallback(void* userdata, Uint8* stream, int len)
  {
    DeviceCapture* self = (DeviceCapture*)userdata;
    std::lock_guard<std::mutex> lock(self->mMutex);
    self->mBytes.insert(self->mBytes.end(), stream, stream + len);

    printf("Saving %d bytes\n", len);
  }

  void write_wave_file(const char* name)
  {
    std::vector<Uint8> fileBytes; 

    const auto BitsPerSample = SDL_AUDIO_BITSIZE(mReceivedRecordingSpec.format);
    const auto Channels = mReceivedRecordingSpec.channels;

    auto file = SDL_RWFromFile(name, "w+b");

    {
      std::lock_guard<std::mutex> lock(mMutex);

      SDL_RWwrite(file, "RIFF", 4, 1); // Marks the file as a riff file.
      SDL_WriteLE32(file, (mBytes.size() + 36) /* size of the WAV header */); // size of file
      SDL_RWwrite(file, "WAVE", 4, 1); // File type header
      SDL_RWwrite(file, "fmt ", 4, 1); // Format chunk marker
      SDL_WriteLE32(file, 16); // Length of format data
      SDL_WriteLE16(file, 1); // Type of format
      SDL_WriteLE16(file, Channels); // Number of channels
      SDL_WriteLE32(file, mReceivedRecordingSpec.freq); // Sample rate
      SDL_WriteLE32(file, ((mReceivedRecordingSpec.freq * BitsPerSample * 2) / 8)); // (Sample Rate * BitsPerSample * Channels) / 8
      SDL_WriteLE16(file, ((BitsPerSample * Channels) / 8)); // (BitsPerSample * Channels) / 8
      SDL_WriteLE16(file, BitsPerSample); //Bits per sample
      SDL_RWwrite(file, "data", 4, 1); // “data” chunk header. Marks the beginning of the data section.
      SDL_WriteLE32(file, mBytes.size()); //Size of the data section.
      SDL_RWwrite(file, mBytes.data(), mBytes.size(), 1);
    }


    //Create file for writing

    if (file == NULL)
    {
      printf("Failed to open file! SDL Error: %s\n", SDL_GetError());
      return;
    }

    auto test = SDL_RWwrite(file, fileBytes.data(), fileBytes.size(), 1);

    //Close file handler
    SDL_RWclose(file);
  }


  SDL_AudioDeviceID mDeviceId;
  SDL_AudioSpec mReceivedRecordingSpec;
  std::mutex mMutex;
  std::vector<Uint8> mBytes;
};

// Main code
int main(int, char**)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with SDL_Renderer graphics context
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+SDL_Renderer example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        SDL_Log("Error creating SDL_Renderer!");
        return 0;
    }
    //SDL_RendererInfo info;
    //SDL_GetRendererInfo(renderer, &info);
    //SDL_Log("Current SDL_Renderer: %s", info.name);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    auto audioDevices = GetAllDevices();

    unsigned int i = 1;
    char* c = (char*)&i;
    if (*c)
      printf("Little endian");
    else
      printf("Big endian");

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    auto currentAudioDriver = SDL_GetCurrentAudioDriver();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    std::unique_ptr<DeviceCapture> capture;

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("Another Window"))
        {
          ImGui::LabelText("Audio Driver:", "Audio Driver: %s", currentAudioDriver);

          static size_t audioDeviceIndex = 0;
          if (ImGui::BeginCombo("Select Audio Device", audioDevices[audioDeviceIndex].mFriendlyName.c_str()))
          {
            for (int n = 0; n < audioDevices.size(); n++)
            {
              const bool is_selected = (audioDeviceIndex == n);
              if (ImGui::Selectable(audioDevices[n].mFriendlyName.c_str(), is_selected))
                audioDeviceIndex = n;

              // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
              //if (is_selected)
              //  ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
          }

          if (ImGui::Button("StartCapture"))
          {
            capture = std::make_unique<DeviceCapture>(audioDevices[audioDeviceIndex]);
          }

          if (capture != nullptr)
          {
            if (ImGui::Button("SaveCapture"))
            {
              capture->write_wave_file("test.wav");
              capture.reset();
            }
          }

          ImGui::End();
        }

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
