
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


// custom data
static SocketLogs* selected_socket = 0;
static SocketLogs* focused_socket = 0;

static bool focus_http = false; // used to toggle between global socket/http loggraph view
static vector<SocketLogs*> previewed_https = {};


static bool preview_socket_packets_sent = false;
static bool preview_socket_bytes_sent = false;
static bool preview_socket_packets_recieved = false;
static bool preview_socket_bytes_recieved = false;
static bool preview_socket_timestamp = false;
static bool preview_socket_status = false;
static bool preview_socket_event_count = false;
static bool preview_socket_label = true;

static bool focused_show_global_send = true;
static bool focused_show_global_recv = true;
static bool focused_show_send = false;
static bool focused_show_sendto = false;
static bool focused_show_wsasend = false;
static bool focused_show_wsasendto = false;
static bool focused_show_wsasendmsg = false;
static bool focused_show_recv = false;
static bool focused_show_recvfrom = false;
static bool focused_show_wsarecv = false;
static bool focused_show_wsarecvfrom = false;

static bool filter_by_has_recieve = false;
static bool filter_by_has_send = false;
static bool filter_by_has_lifetime_activity = false;
static bool filter_by_status_open = false;
static bool filter_by_status_unknown = false;
static bool filter_by_status_closed = false;
static bool filter_by_hidden = true;
static bool filter_by_is_socket = false;
static bool filter_by_is_http = false;
static bool filter_by_is_url = false;
static bool filter_by_has_custom_name = false;
static bool filter_by_is_ext = false;



void log_thing(const char* label, IOLog log, int id){   // show send stats
    ImGui::PushID(id);
    ImGui::Text(label, log.total);
    ImGui::SameLine();
    float recv_average = (log.data[io_history_count - 2] + log.data[io_history_count - 3] + log.data[io_history_count - 4]) / 3.0f;

    ImGui::SameLine();
    if (log.total < 1000)
        ImGui::Text("total: %db", log.total);
    else if (log.total < 1000000)
        ImGui::Text("total: %.1fkb", ((float)log.total) / 1000.0f);
    else ImGui::Text("total: %.1fmb", ((float)log.total) / 1000000.0f);

    ImGui::SameLine();
    if (log.cached_peak < 1000.0f)
        ImGui::Text("peak: %.1fb", log.cached_peak);
    else if (log.cached_peak < 1000000.0f)
        ImGui::Text("peak: %.1fkb", log.cached_peak / 1000.0f);
    else ImGui::Text("peak: %.1fmb", log.cached_peak / 1000000.0f);

    //ImGui::SameLine();
    //if (log.cached_average < 1000.0f)
    //    ImGui::Text("average: %.1fb", log.cached_average);
    //else if (log.cached_average < 1000000.0f)
    //    ImGui::Text("average: %.1fkb", log.cached_average / 1000.0f);
    //else ImGui::Text("average: %.1fmb", log.cached_average / 1000000.0f);

    ImGui::PopID();
}
static bool invert = false; // USED AS AN ADDITIONAL PARAM
bool socket_comp_timestamp(         SocketLogs* a, SocketLogs* b) { return (a->timestamp < b->timestamp) ^ invert; }
bool socket_comp_status(            SocketLogs* a, SocketLogs* b) { return (a->state < b->state) ^ invert; }
bool socket_comp_activity_total(    SocketLogs* a, SocketLogs* b) { return ((a->total_recv_log.total + a->total_send_log.total) < (b->total_recv_log.total + b->total_send_log.total)) ^ invert; }
bool socket_comp_activity_local(    SocketLogs* a, SocketLogs* b) { return ((a->total_recv_log.cached_total + a->total_send_log.cached_total) < (b->total_recv_log.cached_total + b->total_send_log.cached_total)) ^ invert; }
bool socket_comp_activity_timestamp(SocketLogs* a, SocketLogs* b) { 
    long long a_last_activity = a->total_recv_log.last_update_timestamp > a->total_send_log.last_update_timestamp ? a->total_recv_log.last_update_timestamp : a->total_send_log.last_update_timestamp;
    long long b_last_activity = b->total_recv_log.last_update_timestamp > b->total_send_log.last_update_timestamp ? b->total_recv_log.last_update_timestamp : b->total_send_log.last_update_timestamp;
    return (a_last_activity < b_last_activity) ^ invert;
}

std::string ErrorCodeToString(DWORD error_code){
    if (error_code == 0) return std::string("Success");

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    return message;
}

void display_iolog_details(const char* log_label, IOLog* log) {
    ImGui::Text(log_label);
    ImGui::SameLine();
    if (log->last_update_timestamp)
         ImGui::Text("%s |",HistoryToTimestamp(log->last_update_timestamp));
    else ImGui::Text("--:--:-- |");
    ImGui::SameLine();
    ImGui::Text("packets %d",log->total_logs);

    ImGui::SameLine(); 
    if (log->total < 1000)
         ImGui::Text("bytes %db",  log->total);
    else if (log->total < 1000000)
         ImGui::Text("bytes %dkb", log->total/1000);
    else ImGui::Text("bytes %dmb", log->total/1000000);

    ImGui::SameLine();
    ImGui::Text("  GRAPH:", log->total_logs);

    ImGui::SameLine(); 
    if (log->cached_peak < 1000)
         ImGui::Text("peak %db,", (int)(log->cached_peak));
    else if (log->cached_peak < 1000000)
         ImGui::Text("peak %dkb,", (int)(log->cached_peak / 1000));
    else ImGui::Text("peak %dmb,", (int)(log->cached_peak / 1000000));

    ImGui::SameLine();
    if (log->cached_average < 1000)
         ImGui::Text("average %db,", (int)(log->cached_average));
    else if (log->cached_average < 1000000)
         ImGui::Text("average %dkb,", (int)(log->cached_average / 1000));
    else ImGui::Text("average %dmb,", (int)(log->cached_average / 1000000));

    ImGui::SameLine();
    if (log->cached_total < 1000)
         ImGui::Text("total %db", (int)(log->cached_total));
    else if (log->cached_total < 1000000)
         ImGui::Text("total %dkb", (int)(log->cached_total / 1000));
    else ImGui::Text("total %dmb", (int)(log->cached_total / 1000000));
}

void displayLogFilter(const char* context, socket_event_type filter) {

    bool enabled = GetLogFilter(filter);
    if (ImGui::MenuItem(context, 0, &enabled)) {
        UpdateLogFilter(filter, enabled);
        if (enabled)
            LogEntry("log filter enabled", t_generic_log);
        else LogEntry("log filter disabled", t_generic_log);
    }
}

// gets string containing 16 characters, delimited if too long, or prints out the whole thing because why not
std::string BufferShortPreview(vector<char> preview_buffer, int maxbytes = 32) {
    //const int buffer_size = 48;
    //char buffer[buffer_size+1];
    //// if preview is too big to fit in buffer, put as many as we can and delimit
    //if (preview_buffer.size()*3 >= buffer_size) {
    //    for (int i = 0; i*3 < buffer_size-3; i++)
    //        sprintf_s(buffer + (i*3), 4, "%02X ", preview_buffer[i]);
    //    // add delimiting ...
    //    sprintf_s(buffer + buffer_size-3, 4, "...");

    //// else just put all the chars into the buffer
    //} else for (int i = 0; i < preview_buffer.size(); i++)
    //    sprintf_s(buffer + (i*3), 4, "%02X ", preview_buffer[i]);
    //return string(buffer);
    std::stringstream ss;
    for (int i = 0; i < preview_buffer.size(); i++) {
        ss << std::setfill('0') << std::setw(2) << std::hex << (0xff & (unsigned int)preview_buffer[i]) << " ";
        if (maxbytes && i >= maxbytes) { ss << "..."; break; };
    }
    return ss.str();
}

// Main code
int injected_window_main(){
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();

    // init window + imgui stuff
    HWND hwnd; 
    WNDCLASSEXW wc;
    {
        wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
        ::RegisterClassExW(&wc);
        hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
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
        ImPlotStyle& style = ImPlot::GetStyle();
        ImPlotStyle savedStyle = style;
        style.PlotPadding = ImVec2(0, 0);
        style.LabelPadding = ImVec2(0, 0);
        style.LegendPadding = ImVec2(0, 0);
        style.FitPadding = ImVec2(0, 0);
        style.PlotBorderSize = 0;
        style.Colors[ImPlotCol_PlotBg] = ImVec4(0, 0, 0, 0);

        // Setup Platform/Renderer backends
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    }
    ImGuiIO& io = ImGui::GetIO(); (void)io; 

    // Our state
    bool show_demo_window = false;
    bool show_plot_demo_window = false;
    bool show_console_window = false;
    bool show_hostnames_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done) {
        // update window
        {
            // Poll and handle messages (inputs, window resize, etc.)
            // See the WndProc() function below for our to dispatch events to the Win32 backend.
            MSG msg;
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done) break;

            // Handle window being minimized or screen locked
            if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
                ::Sleep(10);
                continue;
            }
            g_SwapChainOccluded = false;

            // Handle window resize (we don't resize directly in the WM_SIZE handler)
            if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
                g_ResizeWidth = g_ResizeHeight = 0;
                CreateRenderTarget();
            }

            // Start the Dear ImGui frame
            ImGui_ImplDX11_NewFrame(); 
            ImGui_ImplWin32_NewFrame(); 
            ImGui::NewFrame(); 
        }

        ImGui::SetNextWindowPos(ImVec2(0, 0)); // Top-left corner
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize); // Full screen size
        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("ImGui Demo", nullptr, &show_demo_window);
                ImGui::MenuItem("ImPlot Demo", nullptr, &show_plot_demo_window);
                if (ImGui::BeginMenu("Console")) {
                    ImGui::MenuItem("Open", nullptr, &show_console_window);
                    if (ImGui::MenuItem("Clear"))
                        ClearLogs();
                    if (ImGui::MenuItem("Export"))
                        DumpToNotepad(string(logs.buffer));
                    ImGui::EndMenu();
                }
                ImGui::MenuItem("Hostnames", nullptr, &show_hostnames_window);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::BeginMenu("Log Filters")) {
                    displayLogFilter("Exclude Send", t_send);
                    displayLogFilter("Exclude Sendto", t_send_to);
                    displayLogFilter("Exclude WSASend", t_wsa_send);
                    displayLogFilter("Exclude WSASendTo", t_wsa_send_to); 
                    displayLogFilter("Exclude WSASendMsg", t_wsa_send_msg); 
                    displayLogFilter("Exclude Recv", t_recv);
                    displayLogFilter("Exclude RecvFrom", t_recv_from);
                    displayLogFilter("Exclude WSARecv", t_wsa_recv);
                    displayLogFilter("Exclude WSARecvFrom", t_wsa_recv_from);
                    displayLogFilter("Exclude HTTP Open", t_http_open);
                    displayLogFilter("Exclude HTTP Close", t_http_close);
                    displayLogFilter("Exclude HTTP Connect", t_http_connect);
                    displayLogFilter("Exclude HTTP Open Request", t_http_open_request);
                    displayLogFilter("Exclude HTTP Add Request Headers", t_http_add_request_headers);
                    displayLogFilter("Exclude HTTP Send Request", t_http_send_request);
                    displayLogFilter("Exclude HTTP Write Data", t_http_write_data);
                    displayLogFilter("Exclude HTTP Read Data", t_http_read_data);
                    displayLogFilter("Exclude HTTP Query Headers", t_http_query_headers);
                    displayLogFilter("Exclude GetHostByName", t_get_hostname);
                    displayLogFilter("Exclude GetAddrInfo", t_get_addr_info);
                    displayLogFilter("Exclude GetAddrInfoEx", t_get_addr_info_ex);
                    displayLogFilter("Exclude LookupServiceBegin", t_wsa_lookup_service_begin);
                    displayLogFilter("Exclude LookupServiceNext", t_wsa_lookup_service_next);
                    displayLogFilter("Exclude Connect", t_connect);
                    displayLogFilter("Exclude NetworkSessionSend", t_ns_send);
                    displayLogFilter("Exclude WRTCDataChannelSend", t_wrtc_send);
                    displayLogFilter("Exclude SignalSend", t_signal_send);
                    displayLogFilter("Exclude SignalRecv", t_signal_recv);

                    displayLogFilter("Exclude Error Log", t_error_log);
                    displayLogFilter("Exclude Generic Log", t_generic_log);
                    displayLogFilter("Exclude Uncategorized Log", t_uncategorized);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Configs")) {
                    if (ImGui::MenuItem("Apply HTTP config", nullptr, nullptr)) {
                        focus_http = true;
                        filter_by_has_custom_name = true;
                        filter_by_is_socket = true;
                        filter_by_is_http = false;
                        filter_by_is_url = true;
                    }
                    if (ImGui::MenuItem("Apply SOCKET config", nullptr, nullptr)) {
                        focus_http = false;
                        filter_by_has_custom_name = false;
                        filter_by_is_socket = false;
                        filter_by_is_http = true;
                        filter_by_is_url = false;
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Save Configs"))
                        SaveConfigs();
                    if (ImGui::MenuItem("Revert To Saved"))
                        LoadConfigs();
                    ImGui::EndMenu();
                }
                ImGui::Separator();
                float graph_speed_adjust = time_factor;
                ImGui::Text("Graph Speed");
                // NOTE: this widget should be disabled when our process is paused, however theres no api for that? so just leave it enabled since it just gives more control to the user
                if (ImGui::SliderFloat("##Graph Speed slider", &graph_speed_adjust, 0.0f, 5.0f))
                    IO_UpdateTimeFactor(graph_speed_adjust);
                
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Tools")) {
                if (ImGui::MenuItem("Dump All HTTP", nullptr, nullptr))
                    DumpAllHTTPEvents();
                if (!is_process_suspended && ImGui::MenuItem("Freeze Process", nullptr, nullptr)) 
                    FreezeProcess();
                if (is_process_suspended && ImGui::MenuItem("Resume Process", nullptr, nullptr))
                    ResumeProcess();
                if (ImGui::MenuItem("Try map all socket addresses", nullptr, nullptr))
                    TryWriteSocketAddresses(false);
                if (ImGui::MenuItem("Try map all socket hostnames", nullptr, nullptr))
                    TryWriteSocketAddresses(true);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        if (show_plot_demo_window)
            ImPlot::ShowDemoWindow(&show_plot_demo_window);
        if (show_console_window) {
            if (ImGui::Begin("Console Output", &show_console_window, ImGuiWindowFlags_HorizontalScrollbar)) {
                ImGui::TextUnformatted(logs.buffer);
                
            }
            ImGui::End();
        }
        if (show_hostnames_window) {
            if (ImGui::Begin("Hostnames Output", &show_hostnames_window, ImGuiWindowFlags_HorizontalScrollbar)) {
                std::string output;
                for (const auto& [ip, host] : mapped_hosts)
                    output += ip + " -> " + host + "\n";
                if (ImGui::Button("Export"))
                    DumpToNotepad(output);
                ImGui::TextUnformatted(output.c_str());
            }
            ImGui::End();
        }


        {
            // performance display stuff
            {

                ImVec2 originalCursorPos = ImGui::GetCursorPos();
                int allocated = ram_allocated;
                const char* size_symbol = "b";
                if (ram_allocated >= 1000000) {
                    allocated /= 1000000;
                    size_symbol = "mb";
                } else if (ram_allocated >= 1000) {
                    allocated /= 1000;
                    size_symbol = "kb";
                }
                char final_string[256];
                if (is_process_suspended) 
                     snprintf(final_string, sizeof(final_string), "PROCESS PAUSED | RAM %d%s | %.1f FPS", allocated, size_symbol, io.Framerate);
                else snprintf(final_string, sizeof(final_string),                  "RAM %d%s | %.1f FPS", allocated, size_symbol, io.Framerate);
            
                // shift text to the right side
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(final_string).x));
                ImGui::Text(final_string);
                ImGui::SetCursorPos(originalCursorPos);
            }

            // refresh all the main display caches
            long long timestamp = IO_history_timestamp();

            // show all HTTP simplified view windows
            for (int i = 0; i < previewed_https.size(); i++) {
                ImGui::PushID(i);
                bool my_window_open = true;
                if (ImGui::Begin("HTTP View", &my_window_open, ImGuiWindowFlags_HorizontalScrollbar)){
                    string display = FormatHTTPEvents(previewed_https[i]);
                    if (ImGui::Button("Export"))
                        DumpToNotepad(display);
                    
                    ImGui::TextUnformatted(display.c_str());
                    ImGui::End();
                } 
                if (!my_window_open) {
                    previewed_https.erase(previewed_https.begin() + i);
                    i--;
                }
                ImGui::PopID();
            }

            // denote thingo
            if (focused_socket) {
                char label[256];
                if (preview_socket_label && focused_socket->custom_label[0])
                    memcpy(label, focused_socket->custom_label, 256);
                else sprintf_s(label, "Socket %llu", (unsigned long long)focused_socket->s);
                ImGui::Text(label);
                // revert focus to global button
                ImGui::SameLine();
                if (ImGui::Button("View global")) {
                    focused_socket = 0;
                }

                // then draw up all the toggles to change what data we show on our graph
                ImGui::SameLine(); ImGui::Checkbox("total send", &focused_show_global_send);
                ImGui::SameLine(); ImGui::Checkbox("total recv", &focused_show_global_recv);

                ImGui::SameLine(); ImGui::Checkbox("send", &focused_show_send);
                ImGui::SameLine(); ImGui::Checkbox("sendto", &focused_show_sendto);
                ImGui::SameLine(); ImGui::Checkbox("wsasend", &focused_show_wsasend);
                ImGui::SameLine(); ImGui::Checkbox("wsasendto", &focused_show_wsasendto);
                ImGui::SameLine(); ImGui::Checkbox("wsasendmsg", &focused_show_wsasendmsg);

                ImGui::SameLine(); ImGui::Checkbox("recv", &focused_show_recv);
                ImGui::SameLine(); ImGui::Checkbox("recvfrom", &focused_show_recvfrom);
                ImGui::SameLine(); ImGui::Checkbox("wsarecv", &focused_show_wsarecv);
                ImGui::SameLine(); ImGui::Checkbox("wsarecvfrom", &focused_show_wsarecvfrom);

            } else {
                ImGui::Text("Viewing global");
                ImGui::Checkbox("HTTP View", &focus_http);
                ImGui::SameLine(); 
                // ?????? what the hell
                bool focus_socket = !focus_http;
                ImGui::Checkbox("Socket View", &focus_socket);
                focus_http = !focus_socket;
            }
                


            ImVec2 ogCursorPos = ImGui::GetCursorPos();
            if (focused_socket) {
                if (focused_show_global_send) {
                    focused_socket->total_send_log.refresh_cache(timestamp);
                    log_thing("total send", focused_socket->total_send_log, 0);
                }
                if (focused_show_global_recv) {
                    focused_socket->total_recv_log.refresh_cache(timestamp);
                    log_thing("total recv", focused_socket->total_recv_log, 0);
                }

                if (focused_show_send) {
                    focused_socket->send_log.refresh_cache(timestamp);
                    log_thing("send", focused_socket->send_log, 0);
                }
                if (focused_show_sendto) {
                    focused_socket->sendto_log.refresh_cache(timestamp);
                    log_thing("sendto", focused_socket->sendto_log, 0);
                }
                if (focused_show_wsasend) {
                    focused_socket->wsasend_log.refresh_cache(timestamp);
                    log_thing("wsasend", focused_socket->wsasend_log, 0);
                }
                if (focused_show_wsasendto) {
                    focused_socket->wsasendto_log.refresh_cache(timestamp);
                    log_thing("wsasendto", focused_socket->wsasendto_log, 0);
                }
                if (focused_show_wsasendmsg) {
                    focused_socket->wsasendmsg_log.refresh_cache(timestamp);
                    log_thing("wsasendmsg", focused_socket->wsasendmsg_log, 0);
                }

                if (focused_show_recv) {
                    focused_socket->recv_log.refresh_cache(timestamp);
                    log_thing("recv", focused_socket->recv_log, 0);
                }
                if (focused_show_recvfrom) {
                    focused_socket->recvfrom_log.refresh_cache(timestamp);
                    log_thing("recvfrom", focused_socket->recvfrom_log, 0);
                }
                if (focused_show_wsarecv) {
                    focused_socket->wsarecv_log.refresh_cache(timestamp);
                    log_thing("wsarecv", focused_socket->wsarecv_log, 0);
                }
                if (focused_show_wsarecvfrom) {
                    focused_socket->wsarecvfrom_log.refresh_cache(timestamp);
                    log_thing("wsarecvfrom", focused_socket->wsarecvfrom_log, 0);
                }
                // otherwise no focused socket, just show global send/recv
            }
            else {
                if (focus_http) {
                    global_http_send_log.refresh_cache(timestamp);
                    global_http_recv_log.refresh_cache(timestamp);
                    log_thing("send", global_http_send_log, 0);
                    log_thing("recv", global_http_recv_log, 1);
                } else {
                    global_io_send_log.refresh_cache(timestamp);
                    global_io_recv_log.refresh_cache(timestamp);
                    log_thing("send", global_io_send_log, 0);
                    log_thing("recv", global_io_recv_log, 1);
                }
            }
            ImGui::SetCursorPos(ogCursorPos);



            if (ImPlot::BeginPlot("IO", ImVec2(-1, 0), ImPlotFlags_NoLegend | ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoInputs | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoFrame)) {
                ImPlot::SetupAxes(0, 0, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines);

                if (focused_socket) {
                    if (focused_show_global_send)   ImPlot::PlotLine("total send", focused_socket->total_send_log.cached_data, io_history_count);
                    if (focused_show_global_recv)   ImPlot::PlotLine("total recv", focused_socket->total_recv_log.cached_data, io_history_count);
                    if (focused_show_send)          ImPlot::PlotLine("send", focused_socket->send_log.cached_data, io_history_count);
                    if (focused_show_sendto)        ImPlot::PlotLine("sendto", focused_socket->sendto_log.cached_data, io_history_count);
                    if (focused_show_wsasend)       ImPlot::PlotLine("wsasend", focused_socket->wsasend_log.cached_data, io_history_count);
                    if (focused_show_wsasendto)     ImPlot::PlotLine("wsasendto", focused_socket->wsasendto_log.cached_data, io_history_count);
                    if (focused_show_wsasendmsg)    ImPlot::PlotLine("wsasendmsg", focused_socket->wsasendmsg_log.cached_data, io_history_count);
                    if (focused_show_recv)          ImPlot::PlotLine("recv", focused_socket->recv_log.cached_data, io_history_count);
                    if (focused_show_recvfrom)      ImPlot::PlotLine("recvfrom", focused_socket->recvfrom_log.cached_data, io_history_count);
                    if (focused_show_wsarecv)       ImPlot::PlotLine("wsarecv", focused_socket->wsarecv_log.cached_data, io_history_count);
                    if (focused_show_wsarecvfrom)   ImPlot::PlotLine("wsarecvfrom", focused_socket->wsarecvfrom_log.cached_data, io_history_count);
                } else if (focus_http){
                    ImPlot::PlotLine("HTTP send", global_http_send_log.cached_data, io_history_count);
                    ImPlot::PlotLine("HTTP read", global_http_recv_log.cached_data, io_history_count);
                } else {
                    ImPlot::PlotLine("send", global_io_send_log.cached_data, io_history_count);
                    ImPlot::PlotLine("recv", global_io_recv_log.cached_data, io_history_count);
                }
                ImPlot::EndPlot();
            }





            enum sort_type {
                sort_by_timestamp,
                sort_by_status,
                sort_by_activity_total,
                sort_by_activity_local,
                sort_by_activity_timestamp,
            };
            static sort_type socket_sort_type = sort_by_timestamp;

            if (ImGui::Button("Sockets.."))
                ImGui::OpenPopup("display_popup");
            if (ImGui::BeginPopup("display_popup")) {
                ImGui::MenuItem("require: recv activity", "", &filter_by_has_recieve);
                ImGui::MenuItem("require: send activity", "", &filter_by_has_send);
                ImGui::MenuItem("require: lifetime activity", "", &filter_by_has_lifetime_activity);
                ImGui::MenuItem("exclude: status open", "", &filter_by_status_open);
                ImGui::MenuItem("exclude: status unknown", "", &filter_by_status_unknown);
                ImGui::MenuItem("exclude: status closed", "", &filter_by_status_closed);
                ImGui::MenuItem("exclude: manually hidden", "", &filter_by_hidden);
                ImGui::MenuItem("exclude: un-named", "", &filter_by_has_custom_name);
                ImGui::MenuItem("exclude: socket", "", &filter_by_is_socket);
                ImGui::MenuItem("exclude: http", "", &filter_by_is_http);
                ImGui::MenuItem("exclude: url", "", &filter_by_is_url);
                ImGui::MenuItem("exclude: ext", "", &filter_by_is_ext);

                ImGui::Separator();
                if (ImGui::MenuItem("sort: creation timestamp", "", socket_sort_type == sort_by_timestamp)) socket_sort_type = sort_by_timestamp;
                if (ImGui::MenuItem("sort: status", "", socket_sort_type == sort_by_status)) socket_sort_type = sort_by_status;
                if (ImGui::MenuItem("sort: total activity", "", socket_sort_type == sort_by_activity_total)) socket_sort_type = sort_by_activity_total;
                if (ImGui::MenuItem("sort: recent activity", "", socket_sort_type == sort_by_activity_local)) socket_sort_type = sort_by_activity_local;
                if (ImGui::MenuItem("sort: last activity timestamp", "", socket_sort_type == sort_by_activity_timestamp)) socket_sort_type = sort_by_activity_timestamp;
                ImGui::Separator();
                ImGui::MenuItem("sort: invert", "", &invert);

                ImGui::Separator();
                ImGui::MenuItem("preview: packets sent", "", &preview_socket_packets_sent);
                ImGui::MenuItem("preview: packets recieved", "", &preview_socket_packets_recieved);
                ImGui::MenuItem("preview: bytes sent", "", &preview_socket_bytes_sent);
                ImGui::MenuItem("preview: bytes recieved", "", &preview_socket_bytes_recieved);
                ImGui::MenuItem("preview: timestamp", "", &preview_socket_timestamp);
                ImGui::MenuItem("preview: status", "", &preview_socket_status);
                ImGui::MenuItem("preview: events", "", &preview_socket_event_count);
                ImGui::MenuItem("preview: label", "", &preview_socket_label);
                ImGui::End();
            }

            // count up sockets & HTTP elements
            int total_sockets = 0;
            int total_shown_sockets = 0;
            int total_http = 0;
            int total_shown_http = 0;
            int total_other = 0;

            // filter out bad sockets
            vector<SocketLogs*> filtered_sockets = {};
            for (const auto& pair : logged_sockets) {
                if (pair.second->source_type == st_Socket)
                    total_sockets++;
                else if (pair.second->source_type == st_WinHttp)
                    total_http++;
                else total_other++;

                // we need to refresh cache to revalidate all values
                pair.second->total_send_log.refresh_cache(timestamp);
                pair.second->total_recv_log.refresh_cache(timestamp);

                // if both IO are required, then we actually require either, NOT both
                if (filter_by_has_recieve && filter_by_has_send) {
                    if (pair.second->total_recv_log.cached_total <= 0.0f 
                    &&  pair.second->total_send_log.cached_total <= 0.0f) continue;
                }else {
                    if (filter_by_has_recieve && pair.second->total_recv_log.cached_total <= 0.0f) continue;
                    if (filter_by_has_send && pair.second->total_send_log.cached_total <= 0.0f) continue;
                }
                if (filter_by_has_lifetime_activity) {
                    if (pair.second->total_recv_log.total <= 0.0f
                    &&  pair.second->total_send_log.total <= 0.0f) continue;
                }

                if (filter_by_status_open && pair.second->state == s_open)    continue;
                if (filter_by_status_unknown && pair.second->state == s_unknown) continue;
                if (filter_by_status_closed && pair.second->state == s_closed)  continue;
                if (filter_by_has_custom_name && pair.second->custom_label[0] == 0) continue;

                if (filter_by_hidden && pair.second->hidden)  continue;

                if (filter_by_is_socket && pair.second->source_type == st_Socket)  continue;
                if (filter_by_is_http && pair.second->source_type == st_WinHttp)   continue;
                if (filter_by_is_url && pair.second->source_type == st_URL)        continue;
                if (filter_by_is_ext && pair.second->source_type == st_ext)        continue;

                filtered_sockets.push_back(pair.second);
                if (pair.second->source_type == st_Socket)
                     total_shown_sockets++;
                else if (pair.second->source_type == st_WinHttp) total_shown_http++;
            }
            // then sort sockets
            switch (socket_sort_type) {
            case sort_by_timestamp:
                std::sort(filtered_sockets.begin(), filtered_sockets.end(), socket_comp_timestamp);
                break;
            case sort_by_status:
                std::sort(filtered_sockets.begin(), filtered_sockets.end(), socket_comp_status);
                break;
            case sort_by_activity_total:
                std::sort(filtered_sockets.begin(), filtered_sockets.end(), socket_comp_activity_total);
                break;
            case sort_by_activity_local:
                std::sort(filtered_sockets.begin(), filtered_sockets.end(), socket_comp_activity_local);
                break;
            case sort_by_activity_timestamp:
                std::sort(filtered_sockets.begin(), filtered_sockets.end(), socket_comp_activity_timestamp);
                break;
            }

            ImGui::SameLine();
            ImGui::Text("Sockets: %d/%d HTTP: %d/%d", total_shown_sockets, total_sockets - total_other, total_shown_http, total_http);
            ImGui::SameLine();
            ImGui::Text("Packets sent: %d, recieved: %d", global_io_send_log.total_logs, global_io_recv_log.total_logs);


            // socket preview panel (Left side) 
            {
                ImGui::BeginChild("left pane", ImVec2(150, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
                for (int i = 0; i < filtered_sockets.size(); i++) {
                    auto soc_logs = filtered_sockets[i];
                    long long current_socket = (long long)soc_logs->s;
                    // refresh cache for our graph widget
                    soc_logs->total_send_log.refresh_cache(timestamp);
                    soc_logs->total_recv_log.refresh_cache(timestamp);

                    // create a widget that shows the name of the socket ID
                    ImGui::PushID((int)current_socket);
                    // we could simplify this code to check whether the two pointers match, and not whether their sockets are the same number
                    // as there should never be a scenario where the pointers match or when the pointers dont match and their socket numbers do
                    if (ImGui::Selectable("##selec", selected_socket && (long long)selected_socket->s == current_socket, 0, ImVec2(0, 40)))
                        selected_socket = soc_logs;

                    ImGui::SameLine();
                    if (ImPlot::BeginPlot("IO", ImVec2(-1, 40), ImPlotFlags_NoLegend | ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoInputs | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoFrame)) {
                        ImPlot::SetupAxes(0, 0, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines);

                        ImVec2 plotPos = ImPlot::GetPlotPos();
                        ImVec2 plotSize = ImPlot::GetPlotSize();
                        ImPlot::PlotLine("Send", soc_logs->total_send_log.cached_data, io_history_count);
                        ImPlot::PlotLine("Recv", soc_logs->total_recv_log.cached_data, io_history_count);
                        ImPlot::EndPlot();

                        // Get the plot position and size
                        ImVec2 originalCursorPos = ImGui::GetCursorPos();
                        ImGui::SetCursorScreenPos(ImVec2(plotPos.x + 5, plotPos.y + 5));

                        char label[256];
                        if (preview_socket_label && soc_logs->custom_label[0])
                            memcpy(label, soc_logs->custom_label, 256);
                        else if (soc_logs->source_type == st_Socket) sprintf_s(label, "Socket %lld", current_socket);
                        else if (soc_logs->source_type == st_WinHttp) sprintf_s(label, "HTTP %lld", current_socket);
                        else sprintf_s(label, "add source here? %lld", current_socket);
                        ImGui::Text(label);

                        if (preview_socket_status) {
                            ImGui::SameLine();
                            if (soc_logs->state == s_closed) ImGui::Text("| CLOSED");
                            else if (soc_logs->state == s_open) ImGui::Text("| OPEN");
                            else                                  ImGui::Text("| UNKNOWN");
                        }

                        if (preview_socket_timestamp) {
                            ImGui::SameLine(); ImGui::Text("| %s", nanosecondsToTimestamp(soc_logs->timestamp).c_str());
                        }

                        if (preview_socket_event_count) {
                            ImGui::SameLine(); ImGui::Text("| Events: %d", soc_logs->events.size());
                        }

                        if (preview_socket_packets_sent) {
                            ImGui::SameLine(); ImGui::Text("| Pkt sent: %d", soc_logs->total_send_log.total_logs);
                        }
                        if (preview_socket_bytes_sent) {
                            ImGui::SameLine(); 
                            if (soc_logs->total_send_log.total < 1000)
                                ImGui::Text("| Dat sent: %db", soc_logs->total_send_log.total);
                            else if (soc_logs->total_send_log.total < 1000000)
                                ImGui::Text("| Dat sent: %dkb", soc_logs->total_send_log.total / 1000);
                            else
                                ImGui::Text("| Dat sent: %dmb", soc_logs->total_send_log.total / 1000000);
                        }
                        if (preview_socket_packets_recieved) {
                            ImGui::SameLine(); ImGui::Text("| Pkt recv: %d", soc_logs->total_recv_log.total_logs);
                        }
                        if (preview_socket_bytes_recieved) {
                            ImGui::SameLine();
                            if (soc_logs->total_recv_log.total < 1000)
                                ImGui::Text("| Dat recv: %db", soc_logs->total_recv_log.total);
                            else if (soc_logs->total_recv_log.total < 1000000)
                                ImGui::Text("| Dat recv: %dkb", soc_logs->total_recv_log.total / 1000);
                            else
                                ImGui::Text("| Dat recv: %dmb", soc_logs->total_recv_log.total / 1000000);
                        }

                        // Restore the original cursor position
                        ImGui::SetCursorPos(originalCursorPos);
                    }
                    ImGui::PopID();
                }
                ImGui::EndChild();
            }
            ImGui::SameLine();

            // socket details panel (Right side)
            {
                ImGui::BeginGroup();
                if (!selected_socket) ImGui::Text("Select a socket...");
                else {
                    ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below
                    ImGui::Text("Socket: %llu", (unsigned long long)selected_socket->s);
                    ImGui::SameLine();
                    ImGui::InputText("name", selected_socket->custom_label, socket_custom_label_len);
                    if (selected_socket->source_type == st_WinHttp) {
                        ImGui::SameLine(); 
                        if (ImGui::Button("HTTP View")) {
                            previewed_https.push_back(selected_socket);
                        }
                    }
                    ImGui::Separator();
                    if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None)) {
                        if (ImGui::BeginTabItem("Event Log")) {

                            bool is_focused = focused_socket == selected_socket;
                            if (ImGui::Checkbox("Focus Graph", &is_focused))
                                if (is_focused) focused_socket = selected_socket;
                                else focused_socket = 0;

                            ImGui::SameLine();
                            ImGui::Checkbox("Hide Socket", &selected_socket->hidden);


                            if (selected_socket->state == s_open)
                                ImGui::Text("OPEN |");
                            else if (selected_socket->state == s_closed)
                                ImGui::Text("CLOSED |");
                            else ImGui::Text("UNKNOWN |");
                            ImGui::SameLine();
                            ImGui::Text(nanosecondsToTimestamp(selected_socket->timestamp).c_str());
                            ImGui::SameLine();
                            ImGui::Text("| Events: %d", selected_socket->events.size());
                            ImGui::SameLine(); 
                            if (selected_socket->log_event_index < 0)
                                 ImGui::TextColored({ 0.7,0.7,0.7,1 }, "(scroll index: %d)", selected_socket->log_event_index);
                            else ImGui::TextColored({ 0.25,0.8,1,1 }, "(scroll index: %d)", selected_socket->log_event_index);

                            // preview 8 events
                            // -1 means view from bottom most

                            if (!selected_socket->events.size())
                                ImGui::Text("No Events...");

                            else{
                                ImGui::BeginChild("logs pane", ImVec2(80, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);

                                const int max_loggy_elements = 8;
                                float totalHeight = ImGui::GetContentRegionAvail().y; // Available height in the panel 
                                float elementHeight = totalHeight / max_loggy_elements; // Height for each element
                                int last_valid_index = selected_socket->events.size() - 1;

                                // TAKE SCROLL INPUT TO NAVIGATE THE LIST

                                if (ImGui::IsWindowHovered()) {
                                    float scrollAmount = ImGui::GetIO().MouseWheel;
                                    if (selected_socket->log_event_index < 0) {
                                        if (scrollAmount > 0)
                                            if (last_valid_index >= max_loggy_elements-1)
                                                selected_socket->log_event_index = last_valid_index;
                                    }else {
                                        // Handle scroll up
                                        if (scrollAmount > 0) {
                                            if  (selected_socket->log_event_index >= max_loggy_elements) // we actaully want the minimum scroll to be
                                                 selected_socket->log_event_index--;

                                        // Handle scroll down
                                        } else if (scrollAmount < 0) {
                                            if  (selected_socket->log_event_index < last_valid_index)
                                                 selected_socket->log_event_index++;
                                            else selected_socket->log_event_index = -1;
                                }}}
                                // auto move scroll to bottom index if too few elements
                                if (selected_socket->log_event_index >= 0 && selected_socket->log_event_index < max_loggy_elements - 1) {
                                    if (last_valid_index > max_loggy_elements - 1)
                                         selected_socket->log_event_index = max_loggy_elements - 1;
                                    else selected_socket->log_event_index = last_valid_index;
                                }

                                // move to max if auto scroll to bottom is enabled
                                int element_index = selected_socket->log_event_index;
                                if (element_index < 0) element_index = last_valid_index;

                                element_index -= max_loggy_elements-1;
                                if (element_index < 0) element_index = 0;

                                // validate the amount of thingos to show
                                int last_index = element_index + max_loggy_elements - 1;
                                if (last_index > last_valid_index) last_index = last_valid_index;


                                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(-1, 0));
                                for (int i = element_index; i <= last_index; i++) {
                                    // get element index

                                    ImGui::PushID(i); // Push a unique ID for each element
                                    socket_log_entry* curr_loggy = selected_socket->events[i];
                                    if (ImGui::Selectable("##selec", i == selected_socket->log_event_index_focused, 0, ImVec2(0, elementHeight)))
                                        selected_socket->log_event_index_focused = i;
                                    ImGui::SameLine();
                                    ImGui::BeginGroup();

                                    // event_name | 00:00:00 | send (wsasendto) | success
                                    // param1: 123 | param2: 234 | param3: 345
                                    ImGui::Text("%d - %s | %s | ", i, nanosecondsToTimestamp(curr_loggy->timestamp).c_str(), curr_loggy->name);
                                    ImGui::SameLine();
                                    if (curr_loggy->category == c_create) ImGui::Text("create  ");
                                    else if (curr_loggy->category == c_send) ImGui::Text("send    ");
                                    else if (curr_loggy->category == c_recieve) ImGui::Text("receive ");
                                    else if (curr_loggy->category == c_info) ImGui::Text("info    ");
                                    else if (curr_loggy->category == c_http) ImGui::Text("http    ");
                                    else if (curr_loggy->category == c_url)  ImGui::Text("url     ");
                                    ImGui::SameLine();
                                    switch (curr_loggy->type) {
                                    case t_send:                     ImGui::Text("(Send)              "); break;
                                    case t_send_to:                  ImGui::Text("(SendTo)            "); break;
                                    case t_wsa_send:                 ImGui::Text("(WSASend)           "); break;
                                    case t_wsa_send_to:              ImGui::Text("(WSASendTo)         "); break;
                                    case t_wsa_send_msg:             ImGui::Text("(WSASendMsg)        "); break;
                                    case t_recv:                     ImGui::Text("(Recv)              "); break;
                                    case t_recv_from:                ImGui::Text("(RecvFrom)          "); break;
                                    case t_wsa_recv:                 ImGui::Text("(WSARecv)           "); break;
                                    case t_wsa_recv_from:            ImGui::Text("(WSARecvFrom)       "); break;
                                    case t_http_open:                ImGui::Text("(HTTPOpen)          "); break;
                                    case t_http_close:               ImGui::Text("(HTTPClose)         "); break;
                                    case t_http_connect:             ImGui::Text("(HTTPConnect)       "); break;
                                    case t_http_open_request:        ImGui::Text("(HTTPOpenRequest)   "); break;
                                    case t_http_add_request_headers: ImGui::Text("(HTTPRequestHeaders)"); break;
                                    case t_http_send_request:        ImGui::Text("(HTTPSendRequest)   "); break;
                                    case t_http_write_data:          ImGui::Text("(HTTPWriteData)     "); break;
                                    case t_http_read_data:           ImGui::Text("(HTTPReadData)      "); break;
                                    case t_http_query_headers:       ImGui::Text("(HTTPQueryHeaders)  "); break;
                                    case t_get_hostname:             ImGui::Text("(GetHostByName)     "); break;
                                    case t_get_addr_info:            ImGui::Text("(GetAddrInfo)       "); break;
                                    case t_get_addr_info_ex:         ImGui::Text("(GetAddrInfoEx)     "); break;
                                    case t_wsa_lookup_service_begin: ImGui::Text("(LookupServiceBegin)"); break;
                                    case t_wsa_lookup_service_next:  ImGui::Text("(LookupServiceNext) "); break;
                                    case t_connect:                  ImGui::Text("(Connect)           "); break;
                                    case t_ns_send:                  ImGui::Text("(NetworkSessionSend)"); break;
                                    case t_wrtc_send:                ImGui::Text("(RTCDataChannelSend)"); break;
                                    case t_signal_send:              ImGui::Text("(SignallingSend)    "); break;
                                    case t_signal_recv:              ImGui::Text("(SignallingRecieve) "); break;
                                    }
                                    ImGui::SameLine();
                                    if (curr_loggy->error_code)
                                        ImGui::TextColored({ 255,0,0,255 }, " %s", ErrorCodeToString(curr_loggy->error_code).c_str());

                                    ImGui::EndGroup();
                                    ImGui::PopID();
                                }
                                ImGui::PopStyleVar();
                                ImGui::EndChild();
                                ImGui::SameLine();

                                // selected event view
                                ImGui::BeginChild("##123");
                                if (selected_socket->log_event_index_focused < 0)
                                    ImGui::Text("No selected event..");
                                else if (selected_socket->log_event_index_focused >= selected_socket->events.size())
                                    ImGui::Text("BAD selected event.. (range overflow)");

                                else {
                                    socket_log_entry* curr_loggy = selected_socket->events[selected_socket->log_event_index_focused];
                                    ImGui::Text("Event %d | %s", selected_socket->log_event_index_focused, nanosecondsToTimestamp(curr_loggy->timestamp).c_str());

                                    ImGui::Text("%s - ", curr_loggy->name);
                                    ImGui::SameLine();
                                    if (curr_loggy->category == c_create) ImGui::Text("create");
                                    else if (curr_loggy->category == c_send) ImGui::Text("send");
                                    else if (curr_loggy->category == c_recieve) ImGui::Text("receive");
                                    else if (curr_loggy->category == c_info) ImGui::Text("info");
                                    else if (curr_loggy->category == c_http) ImGui::Text("http");
                                    ImGui::SameLine();
                                    switch (curr_loggy->type) {
                                    case t_send:                     ImGui::Text("(Send)");               break;
                                    case t_send_to:                  ImGui::Text("(SendTo)");             break;
                                    case t_wsa_send:                 ImGui::Text("(WSASend)");            break;
                                    case t_wsa_send_to:              ImGui::Text("(WSASendTo)");          break;
                                    case t_wsa_send_msg:             ImGui::Text("(WSASendMsg)");         break;
                                    case t_recv:                     ImGui::Text("(Recv)");               break;
                                    case t_recv_from:                ImGui::Text("(RecvFrom)");           break;
                                    case t_wsa_recv:                 ImGui::Text("(WSARecv)");            break;
                                    case t_wsa_recv_from:            ImGui::Text("(WSARecvFrom)");        break;
                                    case t_http_open:                ImGui::Text("(HTTPOpen)");           break;
                                    case t_http_close:               ImGui::Text("(HTTPClose)");          break;
                                    case t_http_connect:             ImGui::Text("(HTTPConnect)");        break;
                                    case t_http_open_request:        ImGui::Text("(HTTPOpenRequest)");    break;
                                    case t_http_add_request_headers: ImGui::Text("(HTTPRequestHeaders)"); break;
                                    case t_http_send_request:        ImGui::Text("(HTTPSendRequest)");    break;
                                    case t_http_write_data:          ImGui::Text("(HTTPWriteData)");      break;
                                    case t_http_read_data:           ImGui::Text("(HTTPReadData)");       break;
                                    case t_http_query_headers:       ImGui::Text("(HTTPQueryHeaders)");   break;
                                    case t_get_hostname:             ImGui::Text("(GetHostByName)");      break;
                                    case t_get_addr_info:            ImGui::Text("(GetAddrInfo)");        break;
                                    case t_get_addr_info_ex:         ImGui::Text("(GetAddrInfoEx)");      break;
                                    case t_wsa_lookup_service_begin: ImGui::Text("(LookupServiceBegin)"); break;
                                    case t_wsa_lookup_service_next:  ImGui::Text("(LookupServiceNext)");  break;
                                    case t_connect:                  ImGui::Text("(Connect)");            break;
                                    case t_ns_send:                  ImGui::Text("(NetworkSessionSend)"); break;
                                    case t_wrtc_send:                ImGui::Text("(RTCDataChannelSend)"); break;
                                    case t_signal_send:              ImGui::Text("(SignallingSend)");     break;
                                    case t_signal_recv:              ImGui::Text("(SignallingRecieve)");  break;
                                    }
                                    ImGui::Text("Status: %s", ErrorCodeToString(curr_loggy->error_code).c_str());
                                    ImGui::NewLine();

                                    // fill in variable details stuff here!!
                                    switch (curr_loggy->type) {
                                        // SEND CLASS UI //
                                    case t_send: {
                                        ImGui::Text("flags: 0x%x", curr_loggy->data.send.flags);
                                        ImGui::Text("data: 0x%llx [%s]", curr_loggy->data.send.buffer.size(), BufferShortPreview(curr_loggy->data.send.buffer).c_str());
                                        if (ImGui::Button("Copy#1")){ CopyToClipboard(BufferShortPreview(curr_loggy->data.send.buffer, 0)); }
                                        break;}
                                    case t_send_to: {
                                        ImGui::Text("flags: 0x%x", curr_loggy->data.sendto.flags);
                                        ImGui::Text("data: 0x%llx [%s]", curr_loggy->data.sendto.buffer.size(), BufferShortPreview(curr_loggy->data.sendto.buffer).c_str());
                                        if (ImGui::Button("Copy#1")) { CopyToClipboard(BufferShortPreview(curr_loggy->data.sendto.buffer, 0)); }
                                        ImGui::Text("port: 0x%x", curr_loggy->data.sendto.address.sin_port);
                                        ImGui::Text("address: %s", curr_loggy->data.sendto.address.IP.c_str());
                                        break;}
                                    case t_wsa_send: {
                                        ImGui::Text("flags: 0x%x", curr_loggy->data.wsasend.flags);
                                        ImGui::Text("sent: 0x%x", curr_loggy->data.wsasend.bytes_sent);
                                        ImGui::Text("callback: 0x%llx", curr_loggy->data.wsasend.completion_routine);
                                        for (int i = 0; i < curr_loggy->data.wsasend.buffer.size(); i++) {
                                            ImGui::Text("data[%d]: 0x%llx [%s]", i, curr_loggy->data.wsasend.buffer[i].size(), BufferShortPreview(curr_loggy->data.wsasend.buffer[i]).c_str());
                                            ImGui::PushID(i);
                                            if (ImGui::Button("Copy")) { CopyToClipboard(BufferShortPreview(curr_loggy->data.wsasend.buffer[i], 0)); }
                                            ImGui::PopID();
                                        }
                                        break;}
                                    case t_wsa_send_to: {
                                        ImGui::Text("flags: 0x%x", curr_loggy->data.wsasendto.flags);
                                        ImGui::Text("sent: 0x%x", curr_loggy->data.wsasendto.bytes_sent);
                                        ImGui::Text("callback: 0x%llx", curr_loggy->data.wsasendto.completion_routine);
                                        for (int i = 0; i < curr_loggy->data.wsasendto.buffer.size(); i++) {
                                            ImGui::Text("data[%d]: 0x%llx [%s]", i, curr_loggy->data.wsasendto.buffer[i].size(), BufferShortPreview(curr_loggy->data.wsasendto.buffer[i]).c_str());
                                            ImGui::PushID(i);
                                            if (ImGui::Button("Copy")) { CopyToClipboard(BufferShortPreview(curr_loggy->data.wsasendto.buffer[i], 0)); }
                                            ImGui::PopID();
                                        }
                                        ImGui::Text("port: 0x%x", curr_loggy->data.wsasendto.address.sin_port);
                                        ImGui::Text("address: %s", curr_loggy->data.wsasendto.address.IP.c_str());
                                        break;}
                                    case t_wsa_send_msg: {
                                        ImGui::Text("flags: 0x%x", curr_loggy->data.wsasendmsg.flags);
                                        ImGui::Text("msg flags: 0x%x", curr_loggy->data.wsasendmsg.msg_flags);
                                        ImGui::Text("sent: 0x%x", curr_loggy->data.wsasendmsg.bytes_sent);
                                        ImGui::Text("callback: 0x%llx", curr_loggy->data.wsasendmsg.completion_routine);
                                        for (int i = 0; i < curr_loggy->data.wsasendmsg.buffer.size(); i++) {
                                            ImGui::Text("data[%d]: 0x%llx [%s]", i, curr_loggy->data.wsasendmsg.buffer[i].size(), BufferShortPreview(curr_loggy->data.wsasendmsg.buffer[i]).c_str());
                                            ImGui::PushID(i);
                                            if (ImGui::Button("Copy")) { CopyToClipboard(BufferShortPreview(curr_loggy->data.wsasendmsg.buffer[i], 0)); }
                                            ImGui::PopID();
                                        }
                                        ImGui::Text("port: 0x%x", curr_loggy->data.wsasendmsg.address.sin_port);
                                        ImGui::Text("address: %s", curr_loggy->data.wsasendmsg.address.IP.c_str());
                                        ImGui::Text("control: 0x%llx [%s]", curr_loggy->data.wsasendmsg.control_buffer.size(), BufferShortPreview(curr_loggy->data.wsasendmsg.control_buffer).c_str());
                                        if (ImGui::Button("Copy#2")) { CopyToClipboard(BufferShortPreview(curr_loggy->data.wsasendmsg.control_buffer, 0)); }
                                        break;}
                                    // RECIEVE CLASS UI //
                                    case t_recv: {
                                        ImGui::Text("flags: 0x%x", curr_loggy->data.recv.flags);
                                        ImGui::Text("data: 0x%llx [%s]", curr_loggy->data.recv.buffer.size(), BufferShortPreview(curr_loggy->data.recv.buffer).c_str());
                                        if (ImGui::Button("Copy")) { CopyToClipboard(BufferShortPreview(curr_loggy->data.recv.buffer, 0)); }
                                        break;}
                                    case t_recv_from: {
                                        ImGui::Text("flags: 0x%x", curr_loggy->data.recvfrom.flags);
                                        ImGui::Text("data: 0x%llx [%s]", curr_loggy->data.recvfrom.buffer.size(), BufferShortPreview(curr_loggy->data.recvfrom.buffer).c_str());
                                        if (ImGui::Button("Copy#1")) { CopyToClipboard(BufferShortPreview(curr_loggy->data.recvfrom.buffer, 0)); }
                                        ImGui::Text("port: 0x%x", curr_loggy->data.recvfrom.address.sin_port);
                                        ImGui::Text("address: %s", curr_loggy->data.recvfrom.address.IP.c_str());
                                        break;}
                                    case t_wsa_recv: {
                                        ImGui::Text("flags: 0x%x", curr_loggy->data.wsarecv.flags);
                                        ImGui::Text("out flags: 0x%x", curr_loggy->data.wsarecv.out_flags);
                                        ImGui::Text("sent: 0x%x", curr_loggy->data.wsarecv.bytes_recived);
                                        ImGui::Text("callback: 0x%llx", curr_loggy->data.wsarecv.completion_routine);
                                        for (int i = 0; i < curr_loggy->data.wsarecv.buffer.size(); i++){
                                            ImGui::Text("data[%d]: 0x%llx [%s]", i, curr_loggy->data.wsarecv.buffer[i].size(), BufferShortPreview(curr_loggy->data.wsarecv.buffer[i]).c_str());
                                            ImGui::PushID(i);
                                            if (ImGui::Button("Copy")) { CopyToClipboard(BufferShortPreview(curr_loggy->data.wsarecv.buffer[i], 0)); }
                                            ImGui::PopID();
                                        }
                                        break;}
                                    case t_wsa_recv_from: {
                                        ImGui::Text("flags: 0x%x", curr_loggy->data.wsarecvfrom.flags);
                                        ImGui::Text("out flags: 0x%x", curr_loggy->data.wsarecvfrom.out_flags);
                                        ImGui::Text("sent: 0x%x", curr_loggy->data.wsarecvfrom.bytes_recived);
                                        ImGui::Text("callback: 0x%llx", curr_loggy->data.wsarecvfrom.completion_routine);
                                        for (int i = 0; i < curr_loggy->data.wsarecvfrom.buffer.size(); i++){
                                            ImGui::Text("data[%d]: 0x%llx [%s]", i, curr_loggy->data.wsarecvfrom.buffer[i].size(), BufferShortPreview(curr_loggy->data.wsarecvfrom.buffer[i]).c_str());
                                            ImGui::PushID(i);
                                            if (ImGui::Button("Copy")) { CopyToClipboard(BufferShortPreview(curr_loggy->data.wsarecvfrom.buffer[i], 0)); }
                                            ImGui::PopID();
                                        }
                                        ImGui::Text("port: 0x%x", curr_loggy->data.wsarecvfrom.address.sin_port);
                                        ImGui::Text("address: %s", curr_loggy->data.wsarecvfrom.address.IP.c_str());
                                        break;}
                                    // HTTP CLASS UI //
                                    case t_http_open: {
                                        ImGui::Text("agent: %s", curr_loggy->data.http_open.agent.c_str());
                                        ImGui::Text("access type: 0x%x", curr_loggy->data.http_open.access_type);
                                        ImGui::Text("proxy: %s", curr_loggy->data.http_open.proxy.c_str());
                                        ImGui::Text("proxy bypass: %s", curr_loggy->data.http_open.proxy_bypass.c_str());
                                        ImGui::Text("flags: 0x%x", curr_loggy->data.http_open.flags);
                                        break;}
                                    case t_http_close: {
                                        break;}
                                    case t_http_connect: {
                                        ImGui::Text("server name: %s", curr_loggy->data.http_connect.server_name.c_str());
                                        ImGui::Text("server port: 0x%x", curr_loggy->data.http_connect.server_port);
                                        ImGui::Text("reserved: 0x%x", curr_loggy->data.http_connect.reserved);
                                        break;}
                                    case t_http_open_request: {
                                        ImGui::Text("verb: %s", curr_loggy->data.http_open_request.verb.c_str());
                                        ImGui::Text("object name: %s", curr_loggy->data.http_open_request.object_name.c_str());
                                        ImGui::Text("version: %s", curr_loggy->data.http_open_request.version.c_str());
                                        ImGui::Text("referrer: %s", curr_loggy->data.http_open_request.referrer.c_str());
                                        ImGui::Text("accepted types: ");
                                        for (int i = 0; i < curr_loggy->data.http_open_request.accept_types.size(); i++)
                                            ImGui::Text("%d: %s", i, curr_loggy->data.http_open_request.accept_types[i].c_str());
                                        ImGui::Text("flags: 0x%x", curr_loggy->data.http_open_request.flags);
                                        break;}
                                    case t_http_add_request_headers: {
                                        ImGui::Text("headers: %s", curr_loggy->data.http_add_request_headers.headers.c_str());
                                        ImGui::Text("modifiers: 0x%x", curr_loggy->data.http_add_request_headers.modifiers);
                                        break;}
                                    case t_http_send_request: {
                                        ImGui::Text("headers: %s", curr_loggy->data.http_send_request.headers.c_str());
                                        ImGui::Text("data: 0x%llx [%s]", curr_loggy->data.http_send_request.optional_data.size(), curr_loggy->data.http_send_request.optional_data.c_str());
                                        ImGui::Text("total length: 0x%x", curr_loggy->data.http_send_request.total_length);
                                        ImGui::Text("context: 0x%llx", curr_loggy->data.http_send_request.context);
                                        break;}
                                    case t_http_write_data: {
                                        ImGui::Text("data: 0x%llx [%s]", curr_loggy->data.http_write_data.data.size(), curr_loggy->data.http_write_data.data.c_str());
                                        ImGui::Text("bytes written: 0x%x", curr_loggy->data.http_write_data.bytes_written);
                                        break;}
                                    case t_http_read_data: {
                                        ImGui::Text("data: 0x%llx [%s]", curr_loggy->data.http_read_data.data.size(), curr_loggy->data.http_read_data.data.c_str());
                                        ImGui::Text("bytes read: 0x%x", curr_loggy->data.http_read_data.bytes_to_read);
                                        break;}
                                    case t_http_query_headers: {
                                        ImGui::Text("info level: 0x%x", curr_loggy->data.http_query_headers.info_level);
                                        ImGui::Text("name: %s", curr_loggy->data.http_query_headers.name.c_str());
                                        ImGui::Text("buffer: 0x%llx [%s]", curr_loggy->data.http_query_headers.buffer.size(), curr_loggy->data.http_query_headers.buffer.c_str());
                                        ImGui::Text("index in: 0x%x", curr_loggy->data.http_query_headers.index_in);
                                        ImGui::Text("index out: 0x%x", curr_loggy->data.http_query_headers.index_out);
                                        break;}
                                    case t_get_hostname: {
                                        ImGui::Text("input name: %s", curr_loggy->data.get_hostbyname.in_name.c_str());
                                        ImGui::Text("name: %s", curr_loggy->data.get_hostbyname.name.c_str());
                                        ImGui::Text("address type: 0x%x", curr_loggy->data.get_hostbyname.addrtype);
                                        ImGui::Text("length: 0x%x", curr_loggy->data.get_hostbyname.length);
                                        ImGui::Text("aliases: ");
                                        for (int i = 0; i < curr_loggy->data.get_hostbyname.aliases.size(); i++)
                                            ImGui::Text("%d: %s", i, curr_loggy->data.get_hostbyname.aliases[i].c_str());
                                        ImGui::Text("addresses: ");
                                        for (int i = 0; i < curr_loggy->data.get_hostbyname.addr_list.size(); i++)
                                            ImGui::Text("%d: %s##2", i, curr_loggy->data.get_hostbyname.addr_list[i].c_str());
                                        break;}
                                    case t_get_addr_info: {
                                        ImGui::Text("node name: %s", curr_loggy->data.get_addr_info.node_name.c_str());
                                        ImGui::Text("service name: %s", curr_loggy->data.get_addr_info.service_name.c_str());
                                        ImGui::Text("results: ");
                                        for (int i = 0; i < curr_loggy->data.get_addr_info.results.size(); i++) {
                                            ImGui::PushID(i);
                                            ImGui::Text("%d - flags: 0x%x", i, curr_loggy->data.get_addr_info.results[i].ai_flags);
                                            ImGui::Text("%d - family: 0x%x", i, curr_loggy->data.get_addr_info.results[i].ai_family);
                                            ImGui::Text("%d - socket type: 0x%x", i, curr_loggy->data.get_addr_info.results[i].ai_socktype);
                                            ImGui::Text("%d - protocol: 0x%x", i, curr_loggy->data.get_addr_info.results[i].ai_protocol);
                                            ImGui::Text("%d - name: %s", i, curr_loggy->data.get_addr_info.results[i].name);
                                            ImGui::Text("%d - port: 0x%x", i, curr_loggy->data.get_addr_info.results[i].address.sin_port);
                                            ImGui::Text("%d - address: %s", i, curr_loggy->data.get_addr_info.results[i].address.IP.c_str());
                                            ImGui::PopID();
                                        }
                                        break;}
                                    case t_get_addr_info_ex: {
                                        ImGui::Text("node name: %s", curr_loggy->data.get_addr_info_ex.node_name.c_str());
                                        ImGui::Text("service name: %s", curr_loggy->data.get_addr_info_ex.service_name.c_str());
                                        ImGui::Text("namespace: 0x%x", curr_loggy->data.get_addr_info_ex.dwNameSpace);
                                        // dont bother showing guid
                                        ImGui::Text("handle: 0x%x", curr_loggy->data.get_addr_info_ex.lpHandle);
                                        ImGui::Text("results: ");
                                        for (int i = 0; i < curr_loggy->data.get_addr_info_ex.results.size(); i++) {
                                            ImGui::PushID(i);
                                            ImGui::Text("%d - flags: 0x%x", i, curr_loggy->data.get_addr_info_ex.results[i].ai_flags);
                                            ImGui::Text("%d - family: 0x%x", i, curr_loggy->data.get_addr_info_ex.results[i].ai_family);
                                            ImGui::Text("%d - socket type: 0x%x", i, curr_loggy->data.get_addr_info_ex.results[i].ai_socktype);
                                            ImGui::Text("%d - protocol: 0x%x", i, curr_loggy->data.get_addr_info_ex.results[i].ai_protocol);
                                            ImGui::Text("%d - name: %s", i, curr_loggy->data.get_addr_info_ex.results[i].name.c_str());
                                            ImGui::Text("%d - port: 0x%x", i, curr_loggy->data.get_addr_info_ex.results[i].address.sin_port);
                                            ImGui::Text("%d - address: %s", i, curr_loggy->data.get_addr_info_ex.results[i].address.IP.c_str());
                                            ImGui::Text("%d - blob: 0x%llx [%s]", i, curr_loggy->data.get_addr_info_ex.results[i].ai_blob.size(), BufferShortPreview(curr_loggy->data.get_addr_info_ex.results[i].ai_blob).c_str());
                                            ImGui::PopID();
                                        }
                                        break;}
                                    case t_wsa_lookup_service_begin: {
                                        ImGui::Text("handle: 0x%x", curr_loggy->data.wsa_lookup_service_begin.lphLookup);
                                        ImGui::Text("control flags: 0x%x", curr_loggy->data.wsa_lookup_service_begin.dwControlFlags);
                                        ImGui::Text("size: 0x%x", curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.dwSize);
                                        ImGui::Text("service instance name: %s", curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.lpszServiceInstanceName.c_str());
                                        // skip guid
                                        ImGui::Text("version: 0x%x", curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.lpVersion.dwVersion);
                                        ImGui::Text("version comparator: 0x%x", (int)curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.lpVersion.ecHow);
                                        ImGui::Text("comment: %s", curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.lpszComment.c_str());
                                        ImGui::Text("namespace: 0x%x", curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.dwNameSpace);
                                        // skip guid
                                        ImGui::Text("context: %s", curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.lpszContext.c_str());
                                        ImGui::Text("query: %s", curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.lpszQueryString.c_str());
                                        ImGui::Text("output flags: 0x%x", curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.dwOutputFlags);
                                        ImGui::Text("blob: 0x%llx [%s]", curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.lpBlob.size(), BufferShortPreview(curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.lpBlob).c_str());
                                        ImGui::Text("protocols: ");
                                        for (int i = 0; i < curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.afpProtocols.size(); i++) {
                                            ImGui::PushID(i);
                                            ImGui::Text("%d - family: 0x%x", i, curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.afpProtocols[i].iAddressFamily);
                                            ImGui::Text("%d - protocol: 0x%x", i, curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.afpProtocols[i].iProtocol);
                                            ImGui::PopID();
                                        }
                                        ImGui::Text("addresses: ");
                                        for (int i = 0; i < curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.csaBuffer.size(); i++) {
                                            ImGui::PushID(i);
                                            ImGui::Text("%d - local IP: %s", i, curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.csaBuffer[i].LocalAddr.IP.c_str());
                                            ImGui::Text("%d - local port: 0x%x", i, curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.csaBuffer[i].LocalAddr.sin_port);
                                            ImGui::Text("%d - remote IP: %s", i, curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.csaBuffer[i].RemoteAddr.IP.c_str());
                                            ImGui::Text("%d - remote port: 0x%x", i, curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.csaBuffer[i].RemoteAddr.sin_port);
                                            ImGui::Text("%d - socket type: 0x%x", i, curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.csaBuffer[i].iSocketType);
                                            ImGui::Text("%d - protocol: 0x%x", i, curr_loggy->data.wsa_lookup_service_begin.lpqsRestrictions.csaBuffer[i].iProtocol);
                                            ImGui::PopID();
                                        }
                                        break;}
                                    case t_wsa_lookup_service_next: {
                                        ImGui::Text("handle: 0x%x", curr_loggy->data.wsa_lookup_service_next.hLookup);
                                        ImGui::Text("control flags: 0x%x", curr_loggy->data.wsa_lookup_service_next.dwControlFlags);
                                        ImGui::Text("buffer size: 0x%x", curr_loggy->data.wsa_lookup_service_next.lpdwBufferLength);
                                        ImGui::Text("size: 0x%x", curr_loggy->data.wsa_lookup_service_next.lpqsResults.dwSize);
                                        ImGui::Text("service instance name: %s", curr_loggy->data.wsa_lookup_service_next.lpqsResults.lpszServiceInstanceName.c_str());
                                        // skip guid
                                        ImGui::Text("version: 0x%x", curr_loggy->data.wsa_lookup_service_next.lpqsResults.lpVersion.dwVersion);
                                        ImGui::Text("version comparator: 0x%x", (int)curr_loggy->data.wsa_lookup_service_next.lpqsResults.lpVersion.ecHow);
                                        ImGui::Text("comment: %s", curr_loggy->data.wsa_lookup_service_next.lpqsResults.lpszComment.c_str());
                                        ImGui::Text("namespace: 0x%x", curr_loggy->data.wsa_lookup_service_next.lpqsResults.dwNameSpace);
                                        // skip guid
                                        ImGui::Text("context: %s", curr_loggy->data.wsa_lookup_service_next.lpqsResults.lpszContext.c_str());
                                        ImGui::Text("query: %s", curr_loggy->data.wsa_lookup_service_next.lpqsResults.lpszQueryString.c_str());
                                        ImGui::Text("output flags: 0x%x", curr_loggy->data.wsa_lookup_service_next.lpqsResults.dwOutputFlags);
                                        ImGui::Text("blob: 0x%llx [%s]", curr_loggy->data.wsa_lookup_service_next.lpqsResults.lpBlob.size(), BufferShortPreview(curr_loggy->data.wsa_lookup_service_next.lpqsResults.lpBlob).c_str());
                                        ImGui::Text("protocols: ");
                                        for (int i = 0; i < curr_loggy->data.wsa_lookup_service_next.lpqsResults.afpProtocols.size(); i++) {
                                            ImGui::PushID(i);
                                            ImGui::Text("%d - family: 0x%x", i, curr_loggy->data.wsa_lookup_service_next.lpqsResults.afpProtocols[i].iAddressFamily);
                                            ImGui::Text("%d - protocol: 0x%x", i, curr_loggy->data.wsa_lookup_service_next.lpqsResults.afpProtocols[i].iProtocol);
                                            ImGui::PopID();
                                        }
                                        ImGui::Text("addresses: ");
                                        for (int i = 0; i < curr_loggy->data.wsa_lookup_service_next.lpqsResults.csaBuffer.size(); i++) {
                                            ImGui::PushID(i);
                                            ImGui::Text("%d - local IP: %s", i, curr_loggy->data.wsa_lookup_service_next.lpqsResults.csaBuffer[i].LocalAddr.IP.c_str());
                                            ImGui::Text("%d - local port: 0x%x", i, curr_loggy->data.wsa_lookup_service_next.lpqsResults.csaBuffer[i].LocalAddr.sin_port);
                                            ImGui::Text("%d - remote IP: %s", i, curr_loggy->data.wsa_lookup_service_next.lpqsResults.csaBuffer[i].RemoteAddr.IP.c_str());
                                            ImGui::Text("%d - remote port: 0x%x", i, curr_loggy->data.wsa_lookup_service_next.lpqsResults.csaBuffer[i].RemoteAddr.sin_port);
                                            ImGui::Text("%d - socket type: 0x%x", i, curr_loggy->data.wsa_lookup_service_next.lpqsResults.csaBuffer[i].iSocketType);
                                            ImGui::Text("%d - protocol: 0x%x", i, curr_loggy->data.wsa_lookup_service_next.lpqsResults.csaBuffer[i].iProtocol);
                                            ImGui::PopID();
                                        }
                                        break;}
                                    case t_connect: {
                                        ImGui::Text("address: %s", curr_loggy->data.connect.address.IP.c_str());
                                        ImGui::Text("port: 0x%x", curr_loggy->data.connect.address.sin_port);
                                        ImGui::Text("length: 0x%x", curr_loggy->data.connect.namelen);
                                        break;}


                                    case t_ns_send: {
                                        ImGui::Text("username: %s", curr_loggy->data.ns_send.username.c_str());
                                        ImGui::Text("password: %s", curr_loggy->data.ns_send.password.c_str());
                                        ImGui::Text("reliable: 0x%x", curr_loggy->data.ns_send.reliable);
                                        ImGui::Text("param5: 0x%x", curr_loggy->data.ns_send.param5);
                                        ImGui::Text("data: 0x%llx [%s]", curr_loggy->data.ns_send.buffer.size(), BufferShortPreview(curr_loggy->data.ns_send.buffer).c_str());
                                        if (ImGui::Button("Copy")) { CopyToClipboard(BufferShortPreview(curr_loggy->data.ns_send.buffer, 0)); }

                                        ImGui::Text("addresses: ");
                                        for (int i = 0; i < curr_loggy->data.ns_send.servers.size(); i++) {
                                            ImGui::PushID(i);
                                            ImGui::Text("%d - url: %s", i, curr_loggy->data.ns_send.servers[i].url.c_str());
                                            ImGui::Text("%d - username: %s", i, curr_loggy->data.ns_send.servers[i].username.c_str());
                                            ImGui::Text("%d - password: %s", i, curr_loggy->data.ns_send.servers[i].password.c_str());
                                            ImGui::PopID();
                                        }
                                        break;}
                                    case t_wrtc_send: {
                                        ImGui::Text("data: 0x%llx [%s]", curr_loggy->data.wrtc_send.buffer.size(), BufferShortPreview(curr_loggy->data.wrtc_send.buffer).c_str());
                                        if (ImGui::Button("Copy")) { CopyToClipboard(BufferShortPreview(curr_loggy->data.wrtc_send.buffer, 0)); }
                                        break;}
                                    case t_signal_send: {
                                        ImGui::Text("code: 0x%x", (int)curr_loggy->data.signal_send.code);
                                        ImGui::Text("GUID: %s", GuidToString(curr_loggy->data.signal_send.guid).c_str());
                                        ImGui::Text("data: 0x%llx [%s]", curr_loggy->data.signal_send.buffer.size(), BufferShortPreview(curr_loggy->data.signal_send.buffer).c_str());
                                        if (ImGui::Button("Copy")) { CopyToClipboard(BufferShortPreview(curr_loggy->data.signal_send.buffer, 0)); }
                                        break;}
                                    case t_signal_recv: {
                                        ImGui::Text("label: %s", curr_loggy->data.signal_recv.label_str.c_str());
                                        ImGui::Text("response: %s", curr_loggy->data.signal_recv.response_str.c_str());
                                        if (ImGui::Button("Copy")) { CopyToClipboard(curr_loggy->data.signal_recv.response_str); }
                                        break;
                                    }





                                    }

                                    if (curr_loggy->callstack)
                                        ImGui::Text(curr_loggy->callstack);
                                    else ImGui::Text("Bad callstack!");

                                }
                                ImGui::EndChild();


                            }
                            ImGui::EndTabItem();
                        }
                        if (ImGui::BeginTabItem("IO")) {
                            ImGui::Text("placeholder");
                            ImGui::EndTabItem();
                        }
                        if (ImGui::BeginTabItem("IO Details")) {
                            if (selected_socket->source_type == st_Socket) {
                                display_iolog_details("total send    |", &selected_socket->total_send_log);
                                display_iolog_details("total recv    |", &selected_socket->total_recv_log);
                                ImGui::NewLine();
                                display_iolog_details("send          |", &selected_socket->send_log);
                                display_iolog_details("send to       |", &selected_socket->sendto_log);
                                display_iolog_details("wsa send      |", &selected_socket->wsasend_log);
                                display_iolog_details("wsa send to   |", &selected_socket->wsasendto_log);
                                display_iolog_details("wsa send msg  |", &selected_socket->wsasendmsg_log);
                                ImGui::NewLine();
                                display_iolog_details("recv          |", &selected_socket->recv_log);
                                display_iolog_details("recv from     |", &selected_socket->recvfrom_log);
                                display_iolog_details("wsa recv      |", &selected_socket->wsarecv_log);
                                display_iolog_details("wsa recv from |", &selected_socket->wsarecvfrom_log);
                            } else if (selected_socket->source_type == st_WinHttp) {
                                display_iolog_details("total sent         |", &selected_socket->total_send_log);
                                display_iolog_details("total read         |", &selected_socket->total_recv_log);
                                ImGui::NewLine();
                                display_iolog_details("HTTP write         |", &selected_socket->send_log);
                                display_iolog_details("HTTP send request  |", &selected_socket->sendto_log);
                                ImGui::NewLine();
                                display_iolog_details("HTTP read          |", &selected_socket->recv_log);
                                display_iolog_details("HTTP query headers |", &selected_socket->recvfrom_log);
                            }
                            else if (selected_socket->source_type == st_URL) {
                                display_iolog_details("total operations |", &selected_socket->total_recv_log);
                                ImGui::NewLine();
                                display_iolog_details("operations       |", &selected_socket->recv_log);
                            }
                            else if (selected_socket->source_type == st_ext) {
                                display_iolog_details("total sent           |", &selected_socket->total_send_log);
                                display_iolog_details("total read           |", &selected_socket->total_recv_log);
                                ImGui::NewLine();
                                display_iolog_details("NetworkSession Sent  |", &selected_socket->send_log);
                                display_iolog_details("WRTCDataChannel Sent |", &selected_socket->sendto_log);
                                display_iolog_details("Signalling Sent      |", &selected_socket->wsasend_log);
                                display_iolog_details("Signalling Recieved  |", &selected_socket->recv_log);
                            }
                            ImGui::EndTabItem();
                        }
                        ImGui::EndTabBar();
                    }
                    ImGui::EndChild();
                    if (ImGui::Button("Revert")) {}
                    ImGui::SameLine();
                    if (ImGui::Button("Save")) {}
                }
                ImGui::EndGroup();
            }



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
