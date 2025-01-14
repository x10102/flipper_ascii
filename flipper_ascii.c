#include <furi.h>
#include <gui/gui.h>

/* generated by fbt from .png files in images folder */
#include <flipper_ascii_icons.h>

#define TAG "flipAscii"
#define MSG_QUEUE_SIZE 8

static const char* CODE_NAMES[] = {"NUL", "SOH", "STX", "ETX", "EOT", "ENQ",
"ACK", "BEL", "BS", "TAB", "LF", "VT", "FF", "CR", "SO", "SI", "DLE", "DC1",
"DC2", "DC3", "DC4", "NAK", "SYN", "ETB", "CAN", "EM", "SUB", "ESC", "FS",
"GS", "RS", "US", "SPACE"};

typedef struct app_state_t {
    FuriMessageQueue *evt_queue;
    Gui *gui;
    ViewPort *view_port;
    uint8_t index;
    uint16_t repeat_count;
} AppState;

// Buffer must fit at least 7 chars
// Expects a value between 0 and 127
void ascii_to_str(uint16_t value, char* buffer) {
    if(value < 33) {
        strcpy(buffer, CODE_NAMES[value]);
        return;
    } else if(value == 127) {
        strcpy(buffer, "DEL");
    } else {
        buffer[0] = (char)value;
        buffer[1]= '\0';
    }
}

void draw_callback(Canvas *canvas, void *ctx) {
    furi_assert(ctx);
    AppState *state = ctx;

    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);

    char buffer[10];
    for(uint8_t i = 0; i < 6; i++) {
        // Draw the decimal number
        snprintf(buffer, 10, "%d", i+state->index);
        canvas_draw_str_aligned(canvas, 2, 10*(i+1), AlignLeft, AlignBottom, buffer);
        // Draw the hex number
        snprintf(buffer, 10, "0x%02X", i+state->index);
        canvas_draw_str_aligned(canvas, 26, 10*(i+1), AlignLeft, AlignBottom, buffer);
        // Draw the string
        ascii_to_str(i+state->index, buffer);
        canvas_draw_str_aligned(canvas, 76, 10*(i+1), AlignCenter, AlignBottom, buffer);
    }
    
}

// Just place the event in the queue to be handled by the main loop
void input_callback(InputEvent *evt, void *ctx) {
    furi_assert(ctx);
    FuriMessageQueue *q = ctx;

    furi_message_queue_put(q, evt, FuriWaitForever);
}

AppState* flipper_ascii_app_alloc() {
    AppState *s = malloc(sizeof(AppState));
    s->evt_queue = furi_message_queue_alloc(MSG_QUEUE_SIZE, sizeof(InputEvent));
    s->view_port = view_port_alloc();
    s->gui = furi_record_open(RECORD_GUI);
    s->index = 0;
    s->repeat_count = 0;
    return s;
}

void flipper_ascii_app_dealloc(AppState* state) {
    furi_check(state);
    furi_check(state->evt_queue);
    furi_message_queue_free(state->evt_queue);
    furi_check(state->view_port);
    view_port_free(state->view_port);
    furi_record_close(RECORD_GUI);
    free(state);
}

int32_t flipper_ascii_app(void* p) {
    UNUSED(p);
    UNUSED(CODE_NAMES);
    bool exit_flag = false;
    InputEvent event;
    AppState *state = flipper_ascii_app_alloc();

    view_port_draw_callback_set(state->view_port, draw_callback, state);
    view_port_input_callback_set(state->view_port, input_callback, state->evt_queue);

    gui_add_view_port(state->gui, state->view_port, GuiLayerFullscreen);

    while(!exit_flag) {
        furi_check(furi_message_queue_get(state->evt_queue, &event, FuriWaitForever) == FuriStatusOk);
        FURI_LOG_I(TAG, "Event received");
        if(event.type != InputTypePress)
            continue;
        switch (event.key)
        {
        case InputKeyBack:
            exit_flag = true;
            break;
        
        case InputKeyUp:
            if(state->index > 0)
                state->index--;
            break;

        case InputKeyDown:
            if(state->index < 122)
                state->index++;
            break;

        default:
            break;
        }
    }

    gui_remove_view_port(state->gui, state->view_port);

    flipper_ascii_app_dealloc(state);

    return 0;
}