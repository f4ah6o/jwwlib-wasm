#ifndef RS_DEBUG_H
#define RS_DEBUG_H

#include <iostream>

// rs_debug.h stub for WASM build
// Replaces LibreCAD's debug system with simple console output

class RS_Debug {
public:
    enum RS_DebugLevel {
        D_NOTHING,
        D_CRITICAL,
        D_ERROR,
        D_WARNING,
        D_NOTICE,
        D_INFORMATIONAL,
        D_DEBUGGING
    };

    void setLevel(RS_DebugLevel level) {
        debugLevel = level;
    }

    void print(const char* format, ...) {
        if (debugLevel >= D_DEBUGGING) {
            std::cout << "[DEBUG] " << format << std::endl;
        }
    }

    void print(RS_DebugLevel level, const char* format, ...) {
        if (level <= debugLevel) {
            const char* prefix = "";
            switch(level) {
                case D_CRITICAL: prefix = "[CRITICAL] "; break;
                case D_ERROR: prefix = "[ERROR] "; break;
                case D_WARNING: prefix = "[WARNING] "; break;
                case D_NOTICE: prefix = "[NOTICE] "; break;
                case D_INFORMATIONAL: prefix = "[INFO] "; break;
                case D_DEBUGGING: prefix = "[DEBUG] "; break;
                default: break;
            }
            std::cout << prefix << format << std::endl;
        }
    }

private:
    RS_DebugLevel debugLevel = D_WARNING;
};

// Global debug instance
inline RS_Debug* RS_DEBUG = new RS_Debug();

#endif // RS_DEBUG_H