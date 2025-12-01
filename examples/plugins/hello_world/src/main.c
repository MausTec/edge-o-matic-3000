// Minimal freestanding WASM plugin - no libc dependencies
// This keeps the binary size very small (< 1KB)

typedef unsigned int uint32_t;

// Entry point for WASM module
// Note: We don't use main() since we're freestanding
// The runtime will call this exported function
__attribute__((export_name("run"))) int run() {
    // Simple math operation
    uint32_t a = 21;
    uint32_t b = 2;
    uint32_t result = a * b;

    // Return the result (42)
    // The runtime can check this value to verify execution
    return (int)result;
}

// Optional: Add an initialization function if needed
__attribute__((export_name("init"))) void init() {
    // Plugin initialization code here
}

// Optional: Event handler for WAMR runtime callbacks
__attribute__((export_name("on_event"))) void
on_event(uint32_t topic_id, const void* data, uint32_t len) {
    // Handle events from the runtime
    // For now, this is just a stub
    (void)topic_id;
    (void)data;
    (void)len;
}
