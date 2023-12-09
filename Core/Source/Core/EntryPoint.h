#pragma once

#if _WIN32
#define PLATFORM_WINDOWS 
#else 
#undef PLATFORM_WINDOWS
#endif

#if _DEBUG
#undef BUILD_DIST
#else
#define BUILD_DIST 
#endif

#ifdef PLATFORM_WINDOWS

extern Core::Application* Core::CreateApplication(int, char**);

bool g_appRunning = true;

namespace Runtime {
	int Main(int argc, char** argv) {
		while (g_appRunning) {
			auto app = Core::CreateApplication(argc, argv);
			app->Run();
			delete app;
		}

		return 0;
	}
}

#if BUILD_DIST
#include "Windows.h"
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR cmdline, int cmdshow) {
	return Runtime::Main(__argc, __argv);
}
#else
int main(int argc, char** argv) {
	return Runtime::Main(argc, argv);
}
#endif

#endif  // PLATFORM WINDOWS