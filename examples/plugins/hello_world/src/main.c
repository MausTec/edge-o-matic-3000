typedef unsigned int uint32_t;

// Entry point for WASM module
int main(int argc, char** argv) {
    uint32_t a = 21;
    uint32_t b = 2;
    uint32_t result = a * b;

    return (int)result;
}