// Spinlock infrastructure implementation

volatile int lock = 0;

void acquire_lock() {
    while(__sync_lock_test_and_set(&lock, 1)) { }
}

void release_lock() {
    __sync_lock_release(&lock);
}

// Enhanced baro capture validation

bool validateBaroCapture(float value) {
    return value > 0 && value < MAX_BARO_VALUE;
}

// Constant definition
const char* FIRMWARE_VERSION = "V2.0";
