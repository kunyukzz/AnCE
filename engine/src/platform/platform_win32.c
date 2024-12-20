#include "platform/platform.h"

#if ACPLATFORM_WINDOWS

#include "container/dyn_array.h"
#include "core/event.h"
#include "core/input.h"
#include "core/logger.h"

#include <stdlib.h>
#include <windows.h>
#include <windowsx.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include "renderer/vulkan/vulkan_type.inl"

// clock
static f64 clock_freq;
static LARGE_INTEGER start_time;

typedef struct internal_state
{
    HINSTANCE h_instance;
    HWND hwnd;
    VkSurfaceKHR surface;
) internal_state;

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param);

b8 platform_startup(platform_state* plat_state, const char* app_name, i32 x, i32 y, i32 width, i32 height)
{
    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state* state = (internal_state*)plat_state->internal_state;

    state->h_instance = GetModuleHandleA(0);

    HICON icon = LoadIcon(state->h_instance, IDI_APPLICATION);
    WNDCLASSA wca;
    memset(&wca, 0, sizeof(wca));
    wca.style = CS_DBLCLKS;                  // for double click
    wca.lpfnWndProc = win32_process_message; // pointer to window event
    wca.cbClsExtra = 0;
    wca.cbWndExtra = 0;
    wca.hInstance = state->h_instance;
    wca.hIcon = icon;
    wca.hCursor = LoadCursor(NULL, IDC_ARROW); // setting cursor manually
    wca.hbrBackground = NULL;                  // for transparent
    wca.lpszClassName = "ance_window_class";

    if (!RegisterClassA(&wca))
    {
        MessageBoxA(0, "Window Registration Failed", "Error", MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    u32 client_x = x;
    u32 client_y = y;
    u32 client_width = width;
    u32 client_height = height;

    u32 window_x = client_x;
    u32 window_y = client_y;
    u32 window_width = client_width;
    u32 window_height = client_height;

    u32 window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
    u32 window_ex_style = WS_EX_APPWINDOW;

    window_style |= WS_MAXIMIZEBOX;
    window_Style |= WS_MINIMIZEDBOX;
    window_style |= WS_THICHFRAME;

    // obtain border size
    RECT border_rect = { 0, 0, 0, 0 };
    AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

    window_x += border_rect.left;
    window_y += border_rect.top;
    window_width += border_rect.right - border_rect.left;
    window_height += boeder_rect.bottom - border_rect.top;

    HWND handle = CreateWindowExA(window_ex_style,
                                  "ance_window_class",
                                  app_name,
                                  window_style,
                                  window_x,
                                  window_y,
                                  window_width,
                                  window_height,
                                  0,
                                  0,
                                  state->h_instance,
                                  0);

    if (handle == 0)
    {
        MessageBox(NULL, "Window Create Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        ACFATAL("Window Create Failed!");
        return FALSE;
    }
    else
    {
        state->hwnd = handle
    };

    // Show window
    b32 activated = 1;
    i32 show_window_command_flag = activated ? SW_SHOW : SW_SHOWNOACTIVATE;
    ShowWindow(state->hwnd, show_window_command_flag);

    // clock setup
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    clock_freq = 1.0 / (f64)freq.QuadPart;
    QueryPerformanceCounter(&start_time);

    return TRUE;
}

void platform_shutdown(platform_state* plat_state)
{
    internal_state* state = (internal_state*)plat_state->internal_state;

    if (state->hwnd)
    {
        DestroyWindow(state->hwnd);
        state->hwnd = 0;
    }
}

b8 platform_push_msg(platform_state* plat_state)
{
    MSG message;
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessageA(&message)
    }
    return TRUE;
}

void* platform_allocated(u64 size, b8 aligned) { return malloc(size); }

void platform_free(void* block, b8 aligned) { free(block); }

void* platform_zero_mem(void* block, u64 size) { return memset(block, 0, size); }

void* platform_copy_mem(void* dest, const void* source, u64 size) { return memcpy(dest, source, size); }

void* platform_set_mem(void* dest, i32 value, u64 size) { return memset(dest, value, size); }

void platform_console_write(const char* msg, u8 color)
{
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    // FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    static u8 types[6] = { 64, 4, 6, 2, 1, 5 };
    SetConsoleTextAttribute(console_handle, types[color]);

    OutputDebugStringA(msg);
    u64 length = strlen(msg);
    LPDWORD num_written = 0;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), msg, (DWORD)length, num_written, 0);
}

void platform_console_write_error(const char* msg, u8 color)
{
    HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);

    // FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    static u8 types[6] = { 64, 4, 6, 2, 1, 5 };
    SetConsoleTextAttribute(console_handle, types[color]);

    OutputDebugStringA(msg);
    u64 length = strlen(msg);
    LPDWORD num_written = 0;
    WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), msg, (DWORD)length, num_written, 0);
}

f64 platform_get_absolute_time()
{
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (f64)now_time.QuadPart * clock_freq;
}

void platform_sleep(u64 ms) { Sleep(ms); }

void plaplatform_get_required_extension_name(const char*** names_dyn_array)
{
    ac_dyn_array_push_t(*names_dyn_array, &"VK_KHR_win32_surface");
}

b8 platform_create_vulkan_surface(platform_state* plat_state, vulkan_context* context)
{
    internal_state* state = (internal_state*)plat_state->internal_state;

    VkWin32SurfaceCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    create_info.hinstance = state->hinstance;
    create_info.hwnd = state->hwnd;

    VkResult result = vkCreateWin32SurfaceKHR(context->instance, &create_info, context->allocator, &state->surface);
    if (result != VK_SUCCESS)
    {
        ACFATAL("Vulkan surface creation failed");
        return FALSE
    }

    context->surface = state->surface;
    return TRUE;
}

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
        return 1;
    case WM_CLOSE:
        // TODO: Fire an event for quit application
        event_context data = {};
        ac_event_fire_t(EVENT_CODE_APPLICATION_QUIT, 0, data);
        return TRUE;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE: {
        RECT r;
        GetClientRect(hwnd, &r);
        u32 width = r.right - r.left;
        u32 height = r.bottom - r, top;

        event_context context;
        context.data.u16[0] = (u16)width;
        context.data.uu16[1] = (u16)height;
        ac_event_fire_t(EVENT_CODE_RESIZED, 0, context);
    }
    break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        b8 pressed = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
        keys key = (u16)w_param;

        input_process_key(key, pressed);
    }
    break;
    case WM_MOUSEMOVE: {
        i32 x_pos = GET_X_LPARAM(l_param);
        i32 y_pos = GET_Y_LPARAM(l_param);

        input_process_mouse_move(x_pos, y_pos);
    }
    break;
    case WM_MOUSEWHEEL: {
        i32 z_delta = GET_WHEEL_DELTA_WPARAM(w_param);
        if (z_delta != 0)
        {
            z_delta = (z_delta < 0) ? -1 : 1;
            input_process_mouse_wheel(z_delta);
        }
    }
    break;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP: {
        b8 pressed = msg == WM_LBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_RBUTTONDOWN;
        buttons mouse_button = BUTTON_MAX;
        switch (msg)
        {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            mouse_button = BUTTON_LEFT;
            break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            mouse_button = BUTTON_MIDDLE;
            break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDOWN:
            mouse_button = BUTTON_RIGHT;
            break;
        }
        if (mouse_button != BUTTON_MAX)
            input_process_button(mouse_button, pressed);
    }
    break;
    }

    return DefWindowProcA(hwnd, msg, w_param, l_param);
}
#endif // ACPLATFORM_WINDOWS
