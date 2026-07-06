#define COBJMACROS
#define DIRECTINPUT_VERSION 0x0500
#include <windows.h>
#include <commdlg.h>
#include <dinput.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define APP_TITLE "MVP 2005 Controller Config"
#define MAX_BINDINGS 64
#define MAX_DEVICES 8
#define PREVIEW_SIZE 16384
#define SAVE_PATH_SIZE 1024

typedef enum { BIND_NONE, BIND_BUTTON, BIND_AXIS_LOW, BIND_AXIS_HIGH, BIND_POV } BindingType;
typedef enum { EXPECT_BUTTON, EXPECT_AXIS, EXPECT_POV } ExpectType;

typedef struct {
    BindingType type;
    int value;
} Binding;

typedef struct {
    const char *key;
    const char *prompt;
    ExpectType expect;
} Step;

static const Step STEPS[] = {
    {"triangle", "Press Triangle / north face button", EXPECT_BUTTON},
    {"cross", "Press X / Cross / south face button", EXPECT_BUTTON},
    {"circle", "Press Circle / east face button", EXPECT_BUTTON},
    {"square", "Press Square / west face button", EXPECT_BUTTON},
    {"l1", "Press L1 / left bumper", EXPECT_BUTTON},
    {"r1", "Press R1 / right bumper", EXPECT_BUTTON},
    {"l2", "Press L2 / left trigger", EXPECT_BUTTON},
    {"r2", "Press R2 / right trigger", EXPECT_BUTTON},
    {"select", "Press Select / Create / Back", EXPECT_BUTTON},
    {"start", "Press Start / Options", EXPECT_BUTTON},
    {"l3", "Press left stick click / L3", EXPECT_BUTTON},
    {"r3", "Press right stick click / R3", EXPECT_BUTTON},
    {"dpad_up", "Press D-pad up", EXPECT_POV},
    {"dpad_down", "Press D-pad down", EXPECT_POV},
    {"dpad_left", "Press D-pad left", EXPECT_POV},
    {"dpad_right", "Press D-pad right", EXPECT_POV},
    {"left_stick_up", "Move left stick fully up, then release", EXPECT_AXIS},
    {"left_stick_down", "Move left stick fully down, then release", EXPECT_AXIS},
    {"left_stick_left", "Move left stick fully left, then release", EXPECT_AXIS},
    {"left_stick_right", "Move left stick fully right, then release", EXPECT_AXIS},
    {"right_stick_up", "Move right stick fully up, then release", EXPECT_AXIS},
    {"right_stick_down", "Move right stick fully down, then release", EXPECT_AXIS},
    {"right_stick_left", "Move right stick fully left, then release", EXPECT_AXIS},
    {"right_stick_right", "Move right stick fully right, then release", EXPECT_AXIS},
};

enum { ID_STATUS = 100, ID_PROMPT, ID_PREVIEW, ID_RETRY_LAST, ID_START_OVER, ID_SAVE, ID_TIMER };

typedef struct {
    LPDIRECTINPUTDEVICEA dev;
    DIDEVCAPS caps;
    DIJOYSTATE prev;
    int initialized;
    char instance_name[MAX_PATH];
    char product_name[MAX_PATH];
    char profile_name[MAX_PATH];
} Controller;

static HWND g_hwnd, g_status, g_prompt, g_preview, g_retry_last, g_start_over, g_save;
static LPDIRECTINPUTA g_di;
static Controller g_devices[MAX_DEVICES];
static int g_device_count;
static int g_active_device = -1;
static char g_product_name[MAX_PATH], g_profile_name[MAX_PATH];
static char g_file_name[MAX_PATH];
static DIDEVCAPS g_caps;
static Binding g_bindings[MAX_BINDINGS];
static int g_step;
static char g_generated[PREVIEW_SIZE];

static int step_count(void) { return (int)(sizeof(STEPS) / sizeof(STEPS[0])); }

static Binding binding_none(void)
{
    Binding b = {BIND_NONE, -1};
    return b;
}

static void copy_string(char *dst, size_t dst_size, const char *src)
{
    if (dst_size == 0) return;
    snprintf(dst, dst_size, "%s", src ? src : "");
}

static void append_text(char **p, size_t *remain, const char *fmt, ...)
{
    if (*remain == 0) return;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(*p, *remain, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if ((size_t)n >= *remain) {
        *p += *remain;
        *remain = 0;
    } else {
        *p += n;
        *remain -= (size_t)n;
    }
}

static void binding_to_text(Binding b, char *out, size_t out_size)
{
    switch (b.type) {
    case BIND_BUTTON: snprintf(out, out_size, "button %d", b.value); break;
    case BIND_AXIS_HIGH: snprintf(out, out_size, "axis- %d", b.value); break;
    case BIND_AXIS_LOW: snprintf(out, out_size, "axis+ %d", b.value); break;
    case BIND_POV: snprintf(out, out_size, "pov%d 1", b.value); break;
    case BIND_NONE:
    default: snprintf(out, out_size, "not_assigned -1"); break;
    }
}

static void sanitize_name(const char *input, char *out, size_t out_size)
{
    size_t j = 0;
    int last_us = 0;
    for (size_t i = 0; input[i] && j + 1 < out_size; i++) {
        unsigned char c = (unsigned char)input[i];
        int ws = c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
        if (ws) {
            if (!last_us && j > 0) {
                out[j++] = '_';
                last_us = 1;
            }
        } else {
            out[j++] = (char)c;
            last_us = 0;
        }
    }
    while (j > 0 && out[j - 1] == '_') j--;
    if (j == 0) {
        strncpy(out, "Controller", out_size);
        out[out_size - 1] = 0;
    } else {
        out[j] = 0;
    }
}

static void sanitize_filename_part(const char *input, char *out, size_t out_size)
{
    size_t j = 0;
    int last_us = 0;
    for (size_t i = 0; input[i] && j + 1 < out_size; i++) {
        unsigned char c = (unsigned char)input[i];
        int ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        if (ok) {
            out[j++] = (char)c;
            last_us = 0;
        } else if (!last_us && j > 0) {
            out[j++] = '_';
            last_us = 1;
        }
    }
    while (j > 0 && out[j - 1] == '_') j--;
    if (j == 0) {
        strncpy(out, "Controller", out_size);
        out[out_size - 1] = 0;
    } else {
        out[j] = 0;
    }
}

static int step_index(const char *key)
{
    for (int i = 0; i < step_count(); i++) {
        if (strcmp(STEPS[i].key, key) == 0) return i;
    }
    return -1;
}

static Binding bind_key(const char *key)
{
    int i = step_index(key);
    return i >= 0 ? g_bindings[i] : binding_none();
}

static void append_binding(char **p, size_t *remain, const char *name, Binding b)
{
    char text[64];
    binding_to_text(b, text, sizeof(text));
    append_text(p, remain, "%s= %s\r\n", name, text);
}

static void generate_config(void)
{
    char *p = g_generated;
    size_t remain = sizeof(g_generated);
    Binding cross = bind_key("cross"), circle = bind_key("circle"), square = bind_key("square"), triangle = bind_key("triangle");
    Binding l1 = bind_key("l1"), r1 = bind_key("r1"), l2 = bind_key("l2"), r2 = bind_key("r2");
    Binding select = bind_key("select"), start = bind_key("start"), l3 = bind_key("l3"), r3 = bind_key("r3");
    Binding dpad_up = bind_key("dpad_up"), dpad_down = bind_key("dpad_down"), dpad_left = bind_key("dpad_left"), dpad_right = bind_key("dpad_right");
    Binding ls_right = bind_key("left_stick_right"), ls_left = bind_key("left_stick_left"), ls_up = bind_key("left_stick_up"), ls_down = bind_key("left_stick_down");
    Binding rs_right = bind_key("right_stick_right"), rs_left = bind_key("right_stick_left"), rs_up = bind_key("right_stick_up"), rs_down = bind_key("right_stick_down");

    g_generated[0] = 0;
    append_text(&p, &remain, "\r\nprofile= %s\r\n", g_profile_name);
    append_text(&p, &remain, "device= %s\r\nplayer= 1\r\n", g_profile_name);
    append_text(&p, &remain, "number_of_buttons= %lu\r\nnumber_of_povs= %lu\r\nnumber_of_axis= %lu\r\n",
                (unsigned long)g_caps.dwButtons, (unsigned long)g_caps.dwPOVs, (unsigned long)g_caps.dwAxes);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_START", start);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_SELECT", select);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_CROSS", cross);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_CIRCLE", circle);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_SQUARE", square);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_TRIANGLE", triangle);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_L1", l1);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_R1", r1);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_L2", l2);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_R2", r2);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_L3", l3);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_R3", r3);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_DPAD_UP", dpad_up);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_DPAD_DOWN", dpad_down);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_DPAD_LEFT", dpad_left);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_DPAD_RIGHT", dpad_right);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_L_STICK_RIGHT", ls_right);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_L_STICK_LEFT", ls_left);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_L_STICK_UP", ls_up);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_L_STICK_DOWN", ls_down);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_R_STICK_RIGHT", rs_right);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_R_STICK_LEFT", rs_left);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_R_STICK_UP", rs_up);
    append_binding(&p, &remain, "VPAD_VIRTUAL_BUTTON_R_STICK_DOWN", rs_down);
    append_binding(&p, &remain, "VPAD_PITCH_1", cross);
    append_binding(&p, &remain, "VPAD_PITCH_2", circle);
    append_binding(&p, &remain, "VPAD_PITCH_3", square);
    append_binding(&p, &remain, "VPAD_PITCH_4", triangle);
    append_binding(&p, &remain, "VPAD_PITCH_5", r1);
    append_binding(&p, &remain, "VPAD_FIELD_PICK_OFF_THROW_FIRST", circle);
    append_binding(&p, &remain, "VPAD_FIELD_PICK_OFF_THROW_SECOND", triangle);
    append_binding(&p, &remain, "VPAD_FIELD_PICK_OFF_THROW_THIRD", square);
    append_binding(&p, &remain, "VPAD_PITCH_OUT", cross);
    append_binding(&p, &remain, "VPAD_THROW_BALL", binding_none());
    append_binding(&p, &remain, "VPAD_INTENTIONAL_WALK", r3);
    append_binding(&p, &remain, "VPAD_INTENTIONAL_HITBATTER", l3);
    append_binding(&p, &remain, "VPAD_PITCH_HISTORY_OPEN", l3);
    append_binding(&p, &remain, "VPAD_SWING_NORMAL", cross);
    append_binding(&p, &remain, "VPAD_SWING_BUNT", r3);
    append_binding(&p, &remain, "VPAD_CHARGE_MOUND", triangle);
    append_binding(&p, &remain, "VPAD_FIELD_THROW_FIRST", circle);
    append_binding(&p, &remain, "VPAD_FIELD_THROW_SECOND", triangle);
    append_binding(&p, &remain, "VPAD_FIELD_THROW_THIRD", square);
    append_binding(&p, &remain, "VPAD_FIELD_THROW_HOME", cross);
    append_binding(&p, &remain, "VPAD_FIELD_SWITCH", l1);
    append_binding(&p, &remain, "VPAD_FIELD_RELAY_THROW", r2);
    append_binding(&p, &remain, "VPAD_FIELD_CUTOFF_THROW", r2);
    append_binding(&p, &remain, "VPAD_FIELD_FAKE_RUNDOWN_THROW", r1);
    append_binding(&p, &remain, "VPAD_RUNNER_FIRST_SELECT", circle);
    append_binding(&p, &remain, "VPAD_RUNNER_SECOND_SELECT", triangle);
    append_binding(&p, &remain, "VPAD_RUNNER_THIRD_SELECT", square);
    append_binding(&p, &remain, "VPAD_RUNNER_RUNFIRST", dpad_right);
    append_binding(&p, &remain, "VPAD_RUNNER_RUNSECOND", dpad_up);
    append_binding(&p, &remain, "VPAD_RUNNER_RUNTHIRD", dpad_left);
    append_binding(&p, &remain, "VPAD_RUNNER_RUNHOME2SCORE", dpad_down);
    append_binding(&p, &remain, "VPAD_BASERUNNER_ADVANCEALL", l1);
    append_binding(&p, &remain, "VPAD_BASERUNNER_RETREATALL", r1);
}

static void update_preview(void)
{
    char text[PREVIEW_SIZE];
    char *p = text;
    size_t remain = sizeof(text);
    text[0] = 0;
    for (int i = 0; i < step_count(); i++) {
        char bind[64] = "";
        if (g_bindings[i].type == BIND_NONE && i < g_step) {
            snprintf(bind, sizeof(bind), "(no mapping)");
        } else if (g_bindings[i].type != BIND_NONE) {
            binding_to_text(g_bindings[i], bind, sizeof(bind));
        }
        append_text(&p, &remain, "%c %-48s %s\r\n", i == g_step ? '>' : ' ', STEPS[i].prompt, bind);
    }
    SetWindowTextA(g_preview, text);
}

static void set_prompt(void)
{
    char status[1024];
    if (g_active_device >= 0) {
        snprintf(status, sizeof(status), "Device: %s  (%lu buttons, %lu axes, %lu POVs)\r\nProfile: %s",
                 g_product_name[0] ? g_product_name : "(none)",
                 (unsigned long)g_caps.dwButtons, (unsigned long)g_caps.dwAxes,
                 (unsigned long)g_caps.dwPOVs, g_profile_name[0] ? g_profile_name : "(none)");
    } else {
        snprintf(status, sizeof(status), "Detected controllers: %d\r\nMapping will lock to the first controller that sends input.", g_device_count);
    }
    SetWindowTextA(g_status, status);
    if (g_device_count == 0) {
        SetWindowTextA(g_prompt, "No DirectInput controller found.");
    } else if (g_step >= step_count()) {
        SetWindowTextA(g_prompt, "Mapping complete. Save the generated config when ready.");
    } else {
        char prompt[256];
        snprintf(prompt, sizeof(prompt), "Step %d of %d: %s    (Esc skips)", g_step + 1, step_count(), STEPS[g_step].prompt);
        SetWindowTextA(g_prompt, prompt);
    }
    EnableWindow(g_retry_last, g_step > 0 && g_step < step_count());
    EnableWindow(g_start_over, g_step > 0);
    update_preview();
}

static BOOL CALLBACK enum_devices(const DIDEVICEINSTANCEA *inst, void *ctx)
{
    HWND hwnd = (HWND)ctx;
    if (g_device_count >= MAX_DEVICES) return DIENUM_STOP;
    Controller *c = &g_devices[g_device_count];
    HRESULT hr = IDirectInput_CreateDevice(g_di, &inst->guidInstance, &c->dev, NULL);
    if (FAILED(hr)) return DIENUM_CONTINUE;
    strncpy(c->instance_name, inst->tszInstanceName, sizeof(c->instance_name));
    c->instance_name[sizeof(c->instance_name) - 1] = 0;
    strncpy(c->product_name, inst->tszProductName, sizeof(c->product_name));
    c->product_name[sizeof(c->product_name) - 1] = 0;
    sanitize_name(c->product_name[0] ? c->product_name : c->instance_name, c->profile_name, sizeof(c->profile_name));
    IDirectInputDevice_SetDataFormat(c->dev, &c_dfDIJoystick);
    IDirectInputDevice_SetCooperativeLevel(c->dev, hwnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
    ZeroMemory(&c->caps, sizeof(c->caps));
    c->caps.dwSize = sizeof(c->caps);
    IDirectInputDevice_GetCapabilities(c->dev, &c->caps);
    IDirectInputDevice_Acquire(c->dev);
    g_device_count++;
    return DIENUM_CONTINUE;
}

static int init_dinput(HWND hwnd)
{
    HRESULT hr = DirectInputCreateA(GetModuleHandleA(NULL), DIRECTINPUT_VERSION, &g_di, NULL);
    if (FAILED(hr)) return 0;
    IDirectInput_EnumDevices(g_di, DIDEVTYPE_JOYSTICK, enum_devices, hwnd, DIEDFL_ATTACHEDONLY);
    return g_device_count > 0;
}

static void finish_mapping(void)
{
    generate_config();
    SetWindowTextA(g_preview, g_generated);
    EnableWindow(g_save, TRUE);
    EnableWindow(g_retry_last, FALSE);
    KillTimer(g_hwnd, ID_TIMER);
    set_prompt();
    SetWindowTextA(g_preview, g_generated);
}

static void accept_binding(Binding b)
{
    if (g_step >= step_count()) return;
    g_bindings[g_step] = b;
    g_step++;
    if (g_step >= step_count()) finish_mapping();
    else set_prompt();
}

static void skip_current_step(void)
{
    if (g_step >= step_count()) return;
    accept_binding(binding_none());
}

static void poll_input(void)
{
    if (g_device_count == 0 || g_step >= step_count()) return;
    int start = g_active_device >= 0 ? g_active_device : 0;
    int end = g_active_device >= 0 ? g_active_device + 1 : g_device_count;
    ExpectType expect = STEPS[g_step].expect;

    for (int d = start; d < end; d++) {
        Controller *c = &g_devices[d];
        DIJOYSTATE st;
        HRESULT hr = IDirectInputDevice_GetDeviceState(c->dev, sizeof(st), &st);
        if (FAILED(hr)) {
            IDirectInputDevice_Acquire(c->dev);
            continue;
        }
        if (!c->initialized) {
            c->prev = st;
            c->initialized = 1;
            continue;
        }

        if (expect == EXPECT_BUTTON) {
            for (int i = 0; i < 32; i++) {
                int was = !!(c->prev.rgbButtons[i] & 0x80);
                int now = !!(st.rgbButtons[i] & 0x80);
                if (!was && now) {
                    Binding b = {BIND_BUTTON, i + 1};
                    c->prev = st;
                    if (g_active_device < 0) {
                        g_active_device = d;
                        g_caps = c->caps;
                        copy_string(g_product_name, sizeof(g_product_name), c->product_name);
                        copy_string(g_profile_name, sizeof(g_profile_name), c->profile_name);
                        sanitize_filename_part(g_profile_name, g_file_name, sizeof(g_file_name));
                    }
                    accept_binding(b);
                    return;
                }
            }
        } else if (expect == EXPECT_AXIS) {
            LONG prev_axes[6] = {c->prev.lX, c->prev.lY, c->prev.lZ, c->prev.lRx, c->prev.lRy, c->prev.lRz};
            LONG axes[6] = {st.lX, st.lY, st.lZ, st.lRx, st.lRy, st.lRz};
            for (int i = 0; i < 6; i++) {
                if (axes[i] < 12000 && prev_axes[i] >= 12000) {
                    Binding b = {BIND_AXIS_LOW, i + 1};
                    c->prev = st;
                    accept_binding(b);
                    return;
                }
                if (axes[i] > 53000 && prev_axes[i] <= 53000) {
                    Binding b = {BIND_AXIS_HIGH, i + 1};
                    c->prev = st;
                    accept_binding(b);
                    return;
                }
            }
        } else if (expect == EXPECT_POV && st.rgdwPOV[0] != c->prev.rgdwPOV[0]) {
            DWORD pov = st.rgdwPOV[0];
            if (pov == 0 || pov == 9000 || pov == 18000 || pov == 27000) {
                Binding b = {BIND_POV, (int)(pov / 100)};
                c->prev = st;
                accept_binding(b);
                return;
            }
        }
        c->prev = st;
    }
}

static void start_mapping(void)
{
    for (int i = 0; i < MAX_BINDINGS; i++) g_bindings[i] = binding_none();
    g_step = 0;
    g_active_device = -1;
    g_caps.dwAxes = g_caps.dwButtons = g_caps.dwPOVs = 0;
    g_product_name[0] = g_profile_name[0] = g_file_name[0] = 0;
    for (int i = 0; i < g_device_count; i++) g_devices[i].initialized = 0;
    g_generated[0] = 0;
    EnableWindow(g_save, FALSE);
    SetTimer(g_hwnd, ID_TIMER, 25, NULL);
    set_prompt();
    SetFocus(g_preview);
}

static void retry_step(void)
{
    if (g_step > 0 && g_step < step_count()) {
        g_step--;
        g_bindings[g_step] = binding_none();
        if (g_active_device >= 0) g_devices[g_active_device].initialized = 0;
        set_prompt();
        SetFocus(g_preview);
    }
}

static void default_save_path(char *out, size_t out_size)
{
    char user[MAX_PATH];
    DWORD n = GetEnvironmentVariableA("USERPROFILE", user, sizeof(user));
    const char *name = g_file_name[0] ? g_file_name : "Controller";
    if (n > 0 && n < sizeof(user)) snprintf(out, out_size, "%s\\Documents\\MVP Baseball 2005\\controller.%s.cfg", user, name);
    else snprintf(out, out_size, "controller.%s.cfg", name);
}

static void save_config(void)
{
    char path[SAVE_PATH_SIZE];
    default_save_path(path, sizeof(path));
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = "Config files (*.cfg)\0*.cfg\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof(path);
    ofn.lpstrDefExt = "cfg";
    ofn.Flags = OFN_OVERWRITEPROMPT;
    if (!GetSaveFileNameA(&ofn)) return;
    FILE *f = fopen(path, "wb");
    if (!f) {
        MessageBoxA(g_hwnd, "Could not write the selected file.", APP_TITLE, MB_ICONERROR);
        return;
    }
    fwrite(g_generated, 1, strlen(g_generated), f);
    fclose(f);
    MessageBoxA(g_hwnd, "Saved generated config.\n\nBack up your existing controller.cfg, then copy or rename this file to controller.cfg.", APP_TITLE, MB_ICONINFORMATION);
}

static void focus_next_button(int reverse)
{
    HWND buttons[] = {g_retry_last, g_start_over, g_save};
    int count = (int)(sizeof(buttons) / sizeof(buttons[0]));
    HWND current = GetFocus();
    int current_index = -1;

    for (int i = 0; i < count; i++) {
        if (buttons[i] == current) {
            current_index = i;
            break;
        }
    }

    for (int step = 1; step <= count; step++) {
        int index;
        if (reverse) {
            index = current_index < 0 ? count - step : (current_index - step + count) % count;
        } else {
            index = current_index < 0 ? step - 1 : (current_index + step) % count;
        }
        if (buttons[index] && IsWindowEnabled(buttons[index])) {
            SetFocus(buttons[index]);
            return;
        }
    }
}

static HWND make_child(const char *class_name, const char *text, DWORD style, int id)
{
    return CreateWindowExA(0, class_name, text, style | WS_CHILD | WS_VISIBLE,
                           0, 0, 0, 0, g_hwnd, (HMENU)(INT_PTR)id, GetModuleHandleA(NULL), NULL);
}

static void layout_children(int width, int height)
{
    int margin = 12;
    MoveWindow(g_status, margin, margin, width - margin * 2, 42, TRUE);
    MoveWindow(g_prompt, margin, 62, width - margin * 2, 48, TRUE);
    MoveWindow(g_preview, margin, 118, width - margin * 2, height - 170, TRUE);
    MoveWindow(g_retry_last, margin, height - 42, 90, 30, TRUE);
    MoveWindow(g_start_over, margin + 102, height - 42, 90, 30, TRUE);
    MoveWindow(g_save, width - margin - 170, height - 42, 170, 30, TRUE);
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        g_hwnd = hwnd;
        g_status = make_child("STATIC", "Device: detecting...", 0, ID_STATUS);
        g_prompt = make_child("STATIC", "Preparing controller mapping...", SS_CENTER, ID_PROMPT);
        g_preview = make_child("EDIT", "", WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, ID_PREVIEW);
        SendMessageA(g_preview, WM_SETFONT, (WPARAM)GetStockObject(ANSI_FIXED_FONT), TRUE);
        g_retry_last = make_child("BUTTON", "Retry Last", WS_TABSTOP, ID_RETRY_LAST);
        g_start_over = make_child("BUTTON", "Start Over", WS_TABSTOP, ID_START_OVER);
        g_save = make_child("BUTTON", "Save Generated Config", WS_TABSTOP, ID_SAVE);
        EnableWindow(g_retry_last, FALSE);
        EnableWindow(g_start_over, FALSE);
        EnableWindow(g_save, FALSE);
        for (int i = 0; i < MAX_BINDINGS; i++) g_bindings[i] = binding_none();
        init_dinput(hwnd);
        start_mapping();
        return 0;
    case WM_SIZE:
        layout_children(LOWORD(lp), HIWORD(lp));
        return 0;
    case WM_COMMAND:
        if (LOWORD(wp) == ID_RETRY_LAST) {
            retry_step();
            return 0;
        }
        if (LOWORD(wp) == ID_START_OVER) {
            start_mapping();
            return 0;
        }
        if (LOWORD(wp) == ID_SAVE) {
            save_config();
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) {
            skip_current_step();
            return 0;
        }
        break;
    case WM_TIMER:
        if (wp == ID_TIMER) poll_input();
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER);
        for (int i = 0; i < g_device_count; i++) {
            if (g_devices[i].dev) {
                IDirectInputDevice_Unacquire(g_devices[i].dev);
                IDirectInputDevice_Release(g_devices[i].dev);
            }
        }
        if (g_di) IDirectInput_Release(g_di);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    (void)prev;
    (void)cmd;
    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = inst;
    wc.lpszClassName = "MVP2005CtrlCfgWnd";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(0, wc.lpszClassName, APP_TITLE, WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, 840, 640, NULL, NULL, inst, NULL);
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            skip_current_step();
            continue;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_TAB) {
            focus_next_button(GetKeyState(VK_SHIFT) < 0);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}
