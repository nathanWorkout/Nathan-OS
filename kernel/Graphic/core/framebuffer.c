#include <limine.h>
#include "gfx.h"
#include "framebuffer.h"

static volatile struct limine_framebuffer_request fb_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

Canvas fb_get_canvas(void) {
    struct limine_framebuffer *fb = fb_request.response->framebuffers[0];
    Canvas cv = {
        .address = (uint32_t *)fb->address,
        .width   = fb->width,
        .height  = fb->height,
        .pitch   = fb->pitch,
    };
    return cv;
}
