
// Dear ImGui: standalone example application for DirectX 11

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include <d3d11_1.h>
#pragma comment(lib, "d3d11.lib")
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#include "imgui/imgui.h"
#include "imgui/implot.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include <tchar.h>

// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int injected_window_main()
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls


    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // implot styles
    //ImPlotStyle& style = ImPlot::GetStyle();
    //ImPlotStyle savedStyle = style;
    //style.PlotPadding = ImVec2(0, 0);
    //style.LabelPadding = ImVec2(0, 0);
    //style.LegendPadding = ImVec2(0, 0);
    //style.FitPadding = ImVec2(0, 0);
    //style.PlotBorderSize = 0;
    //style.Colors[ImPlotCol_PlotBg] = ImVec4(0, 0, 0, 0);



    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Our state
    bool show_demo_window = true;
    bool show_plot_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done){
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)){
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED){
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0){
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("ImGui Demo", nullptr, &show_demo_window);
                ImGui::MenuItem("ImPlot Demo", nullptr, &show_plot_demo_window);
                ImGui::MenuItem("Perf Window", nullptr, &show_another_window);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        if (show_plot_demo_window)
            ImPlot::ShowDemoWindow(&show_plot_demo_window);


        {
            // refresh all the main display caches
            long long timestamp = IO_history_timestamp();
            global_io_send_log.refresh_cache(timestamp);
            global_io_recv_log.refresh_cache(timestamp);
            float* send_data = global_io_send_log.cached_data;
            float* recv_data = global_io_recv_log.cached_data;

            {   // show send stats
                float send_average = (send_data[io_history_count-2] + send_data[io_history_count-3] + send_data[io_history_count-4]) / 3.0f;
                if (send_average < 1000)
                     ImGui::Text("Send: %.1fb/s", send_average);
                else if (send_average < 1000000)
                     ImGui::Text("Send: %.1fkb/s", send_average / 1000.0f);
                else ImGui::Text("Send: %.1fmb/s", send_average / 1000000.0f);
                ImGui::SameLine();
                if (global_io_send_log.total < 1000)
                     ImGui::Text("total: %db", global_io_send_log.total);
                else if (global_io_send_log.total < 1000000)
                     ImGui::Text("total: %.1fkb", ((float)global_io_send_log.total) / 1000.0f);
                else ImGui::Text("total: %.1fmb", ((float)global_io_send_log.total) / 1000000.0f);
            }

            {   // show send stats
                float recv_average = (recv_data[io_history_count-2] + recv_data[io_history_count-3] + recv_data[io_history_count-4]) / 3.0f;
                if (recv_average < 1000)
                     ImGui::Text("Recv: %.1fb/s", recv_average);
                else if (recv_average < 1000000)
                     ImGui::Text("Recv: %.1fkb/s", recv_average / 1000.0f);
                else ImGui::Text("Recv: %.1fmb/s", recv_average / 1000000.0f);
                ImGui::SameLine();
                if (global_io_recv_log.total < 1000)
                     ImGui::Text("total: %db", global_io_recv_log.total);
                else if (global_io_recv_log.total < 1000000)
                     ImGui::Text("total: %.1fkb", ((float)global_io_recv_log.total) / 1000.0f);
                else ImGui::Text("total: %.1fmb", ((float)global_io_recv_log.total) / 1000000.0f);
            }



            if (ImPlot::BeginPlot("IO", ImVec2(-1, 0), ImPlotFlags_NoLegend | ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoInputs | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoFrame)) {
                ImPlot::SetupAxes(0, 0, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines);
                ImPlot::PlotLine("Send", send_data, io_history_count);
                ImPlot::PlotLine("Recv", recv_data, io_history_count);
                ImPlot::EndPlot();
            }

            ImGui::Text("Sockets: %d/%d", logged_sockets.size(), logged_sockets.size());
            ImGui::SameLine();
            ImGui::Text("Packets sent: %d, recieved: %d", global_io_send_log.total_logs, global_io_recv_log.total_logs);

            // Left
            static long long selected_socket = 0;
            {
                ImGui::BeginChild("left pane", ImVec2(150, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
                for (const auto& pair : logged_sockets) {
                    long long current_socket = (long long)pair.first;
                    // refresh cache for our graph widget
                    pair.second->total_send_log.refresh_cache(timestamp);
                    pair.second->total_recv_log.refresh_cache(timestamp);

                    // create a widget that shows the name of the socket ID
                    ImGui::PushID(current_socket);
                    if (ImGui::Selectable("##selec", selected_socket == current_socket, 0, ImVec2(0, 40)))
                        selected_socket = current_socket;

                    ImGui::SameLine();
                    if (ImPlot::BeginPlot("IO", ImVec2(-1, 40), ImPlotFlags_NoLegend | ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoInputs | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoFrame)) {
                        ImPlot::SetupAxes(0, 0, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines);

                        ImVec2 plotPos = ImPlot::GetPlotPos();
                        ImVec2 plotSize = ImPlot::GetPlotSize();
                        ImPlot::PlotLine("Send", pair.second->total_send_log.cached_data, io_history_count);
                        ImPlot::PlotLine("Recv", pair.second->total_recv_log.cached_data, io_history_count);
                        ImPlot::EndPlot();

                        // Get the plot position and size
                        ImVec2 originalCursorPos = ImGui::GetCursorPos();
                        ImGui::SetCursorScreenPos(ImVec2(plotPos.x + 5, plotPos.y + 5));

                        char label[128];
                        sprintf_s(label, "Socket %d", current_socket);
                        ImGui::Text(label);
                        // Restore the original cursor position
                        ImGui::SetCursorPos(originalCursorPos);
                    }
                    ImGui::PopID();
                }
                ImGui::EndChild();
            }
            ImGui::SameLine();

            // Right
            {
                ImGui::BeginGroup();
                ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
                ImGui::Text("Socket: %d", selected_socket);
                ImGui::Separator();
                if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
                {
                    if (ImGui::BeginTabItem("Description"))
                    {
                        ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. ");
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Details"))
                    {
                        ImGui::Text("ID: 0123456789");
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                ImGui::EndChild();
                if (ImGui::Button("Revert")) {} 
                ImGui::SameLine(); 
                if (ImGui::Button("Save")) {} 
                ImGui::EndGroup(); 
            }

            

            ImGui::End();
        }

        if (show_another_window){
            ImGui::Begin("Performance data", &show_another_window);
            ImGui::Text("Hello from another window!");
            int ram_rounded = ram_allocated;
            if (ram_rounded < 1000) 
                 ImGui::Text("Allocated Mem: %db", ram_rounded);
            else if (ram_rounded < 1000000) 
                 ImGui::Text("Allocated Mem: %dkb", ram_rounded/1000);
            else ImGui::Text("Allocated Mem: %dmb", ram_rounded/1000000);
            
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
