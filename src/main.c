#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <syscall.h>
#include <unistd.h>
#include <sys/mman.h>

#include <wayland-client.h>
#include <wayland-client-protocol.h>

#include "../xdg-shell-client-protocol.h"

struct wl_compositor *compositor;
struct wl_shm *shm;
struct xdg_wm_base *xdg;

bool configured = false;
bool running = true;

struct wl_surface *surface;
struct xdg_surface *xdg_surface;
struct xdg_toplevel *xdg_toplevel;

void registry_global_handler
(
    void *data,
    struct wl_registry *registry,
    uint32_t name,
    const char *interface,
    uint32_t version
) {
    if (strcmp(interface, "wl_compositor") == 0) {
        compositor = wl_registry_bind(registry, name,
            &wl_compositor_interface, 3);
    } else if (strcmp(interface, "wl_shm") == 0) {
        shm = wl_registry_bind(registry, name,
            &wl_shm_interface, 1);
    } else if (strcmp(interface, "xdg_wm_base") == 0) {
        xdg = wl_registry_bind(registry, name,
            &xdg_wm_base_interface, 1);
    }
}

void registry_global_remove_handler
(
    void *data,
    struct wl_registry *registry,
    uint32_t name
) {}

const struct wl_registry_listener registry_listener = {
    .global = registry_global_handler,
    .global_remove = registry_global_remove_handler
};

void xdg_wm_base_handle_ping(void *data,
		struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
	// The compositor will send us a ping event to check that we're responsive.
	// We need to send back a pong request immediately.
	xdg_wm_base_pong(xdg_wm_base, serial);
}

const struct xdg_wm_base_listener xdg_wm_base_listener = {
	.ping = xdg_wm_base_handle_ping,
};

void xdg_surface_handle_configure(void *data,
		struct xdg_surface *xdg_surface, uint32_t serial) {
	// The compositor configures our surface, acknowledge the configure event
	xdg_surface_ack_configure(xdg_surface, serial);

	if (configured) {
		// If this isn't the first configure event we've received, we already
		// have a buffer attached, so no need to do anything. Commit the
		// surface to apply the configure acknowledgement.
		wl_surface_commit(surface);
	}

	configured = true;
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_handle_configure,
};

void noop() {}

static void xdg_toplevel_handle_close(void *data,
		struct xdg_toplevel *xdg_toplevel) {
	// Stop running if the user requests to close the toplevel
	running = false;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = noop,
	.close = xdg_toplevel_handle_close,
};

int main(void) {
    struct wl_display *display = wl_display_connect(NULL);
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    // wait for the "initial" set of globals to appear
    wl_display_roundtrip(display);

    surface = wl_compositor_create_surface(compositor);
    xdg_surface = xdg_wm_base_get_xdg_surface(xdg, surface);
    xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
    xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);

		wl_surface_commit(surface);
    while (!configured) {
        wl_display_dispatch(display);
    }
    
    int width = 200;
    int height = 200;
    int stride = width * 4;
    int size = stride * height;  // bytes

    // open an anonymous file and write some zero bytes to it
    int fd = syscall(SYS_memfd_create, "buffer", 0);
    ftruncate(fd, size);

    // map it to the memory
    unsigned char *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    // turn it into a shared memory pool
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);

    // allocate the buffer in that pool
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool,
        0, width, height, stride, WL_SHM_FORMAT_XRGB8888);

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_commit(surface);

    while (running) {
        wl_display_dispatch(display);
    }

	  xdg_toplevel_destroy(xdg_toplevel);
	  xdg_surface_destroy(xdg_surface);
	  wl_surface_destroy(surface);
	  wl_buffer_destroy(buffer);

    return 0;
}
