/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdarg>
#include <cstring>
#include <cstdio>

#include <coreinit/screen.h>
#include <coreinit/cache.h>
#include <coreinit/memheap.h>
#include <coreinit/memfrmheap.h>
#include <proc_ui/procui.h>

#include "gfx.h"

#define CONSOLE_FRAME_HEAP_TAG (0x000DECAF) // copied from wut lmao

namespace Gfx {
    static void *tvBuffer, *drcBuffer;
    static uint32_t tvBufferSize, drcBufferSize;

    static bool hasForeground = true;

    uint32_t BackgroundColour;
    
    static uint32_t ProcCallbackAcquired(void *context) {
        MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
        MEMRecordStateForFrmHeap(heap, CONSOLE_FRAME_HEAP_TAG);

        if (tvBufferSize) {
            tvBuffer = MEMAllocFromFrmHeapEx(heap, tvBufferSize, 4);
        }

        if (drcBufferSize) {
            drcBuffer = MEMAllocFromFrmHeapEx(heap, drcBufferSize, 4);
        }

        hasForeground = true;
        OSScreenSetBufferEx(SCREEN_TV, tvBuffer);
        OSScreenSetBufferEx(SCREEN_DRC, drcBuffer);

        return 0;
    }

    static uint32_t ProcCallbackReleased(void *context) {
        MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
        MEMFreeByStateToFrmHeap(heap, CONSOLE_FRAME_HEAP_TAG);
        
        hasForeground = false;
        
        return 0;
    }
    
    bool Init() {
        OSScreenInit();
        tvBufferSize = OSScreenGetBufferSizeEx(SCREEN_TV);
        drcBufferSize = OSScreenGetBufferSizeEx(SCREEN_DRC);

        ProcCallbackAcquired(nullptr);
        OSScreenEnableEx(SCREEN_TV, TRUE);
        OSScreenEnableEx(SCREEN_DRC, TRUE);

        ProcUIRegisterCallback(PROCUI_CALLBACK_ACQUIRE, ProcCallbackAcquired, nullptr, 100);
        ProcUIRegisterCallback(PROCUI_CALLBACK_RELEASE, ProcCallbackReleased, nullptr, 100);

        return true;
    }

    void Shutdown() {
        if (hasForeground) {
            OSScreenShutdown();
            ProcCallbackReleased(nullptr);
        }
    }

    void Clear(uint32_t colour) {
        OSScreenClearBufferEx(SCREEN_TV, colour);
        OSScreenClearBufferEx(SCREEN_DRC, colour);
    }

    void Draw() {
        DCFlushRange(tvBuffer, tvBufferSize);
        DCFlushRange(drcBuffer, drcBufferSize);
        OSScreenFlipBuffersEx(SCREEN_TV);
        OSScreenFlipBuffersEx(SCREEN_DRC);
    }

    void PutFont(int x, int y, const char *str) {
        OSScreenPutFontEx(SCREEN_TV, x, y, str);
        OSScreenPutFontEx(SCREEN_DRC, x, y, str);
    }

    void Print(int x, int y, const char *fmt, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        int curX = x, curY = y;
        char *lineStart = buffer;

        for (char *s = buffer; *s; s++) {
            if (*s == '\n') {
                *s = '\0';
                PutFont(curX, curY, lineStart);
                *s = '\n';
                curY += 1;
                lineStart = s + 1;
            }
        }

        if (*lineStart) {
            PutFont(curX, curY, lineStart);
        }
    }

    void SetBackgroundColour(uint32_t colour) {
        BackgroundColour = colour;
    }

    uint32_t GetBackgroundColour() {
        return BackgroundColour;
    }
}