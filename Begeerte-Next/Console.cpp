#include "Console.h"
#include <Windows.h>
#include <stdio.h>

namespace Console {
    void Initialize() {
        AllocConsole();
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

        printf("[Begeerte] Console initialized.\n");
    }

    void Cleanup() {
        printf("[Begeerte] Console cleaning up...\n");
        FreeConsole();
        fclose(stdout);
    }
}