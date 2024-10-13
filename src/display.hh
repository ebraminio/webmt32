#pragma once

#include <cairo.h>

template<unsigned width, unsigned height>
class Painter {
    static_assert(width % 8 == 0, "width must be a multiple of 8");
    unsigned stride;
    cairo_surface_t *surface;
    uint8_t buffer[width * height / 8]{};
    cairo_t *cr;

public:
    explicit Painter() {
        stride = cairo_format_stride_for_width(CAIRO_FORMAT_A1, width);
        assert(stride == width / 8 && "Cairo not able to handle contiguous buffer for the width");
        surface = cairo_image_surface_create_for_data(buffer, CAIRO_FORMAT_A1, width, height, stride);
        cr = cairo_create(surface);
        cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 42);
        cairo_set_source_rgb(cr, 1, 1, 1);
    }

    uint8_t *frame() {
        memset(buffer, 0, stride * height);

        const time_t now = time(nullptr);
        const tm *tm = localtime(&now);
        char str[8] = {};
        snprintf(str, 6, "%02d%c%02d", tm->tm_hour, now % 2 ? ':' : ' ', tm->tm_min);

        cairo_set_font_size(cr, 42);
        cairo_move_to(cr, 0, 32);
        cairo_show_text(cr, str);

        cairo_set_font_size(cr, 10);
        cairo_move_to(cr, 0, 56);
        cairo_show_text(cr, currentMessage);

        return buffer;
    }
};

#ifndef __EMSCRIPTEN__
#include <MiniFB.h>

template<unsigned width, unsigned height>
class MiniFB {
    static_assert(width % 8 == 0, "width must be a multiple of 8");
    mfb_window *window;
    uint32_t buffer[width * height]{};

public:
    MiniFB() {
        window = mfb_open("WebMT32", width, height);
    }

    void update(const uint8_t *frame) {
        if (!window) return;
        for (unsigned i = 0; i < width * height; ++i) {
            const uint8_t value = frame[i / 8] & 1 << i % 8 ? 0xFF : 0;
            buffer[i] = MFB_ARGB(0xFF, value, value, value);
        }
        mfb_update(window, buffer);
    }
};
#endif
