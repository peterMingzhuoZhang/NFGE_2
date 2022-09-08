#include <Core\Inc\Core.h>
#include <Grphics/Inc/Graphics.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	NFGE::Core::Window myWindow;
	myWindow.Initialize(hInstance, "Hello Window", 1280, 720);
    auto handle = myWindow.GetWindowHandle();
    NFGE::Graphics::GraphicsSystem::StaticInitialize(myWindow, true, false);

	bool done = false;

	while (!done)
	{
		done = myWindow.ProcessMessage();

		// run your game logic ...
        static uint64_t frameCounter = 0;
        static double elapsedSeconds = 0.0;
        static std::chrono::high_resolution_clock clock;
        static auto t0 = clock.now();

		{// time and delta time block
            frameCounter++;
            auto t1 = clock.now();
            auto deltaTime = t1 - t0;
            t0 = t1;

            elapsedSeconds += deltaTime.count() * 1e-9;
            if (elapsedSeconds > 1.0)
            {
                char buffer[500];
                auto fps = frameCounter / elapsedSeconds;
                sprintf_s(buffer, 500, "FPS: %f\n", fps);
                LOG("FPS: %s", buffer);

                frameCounter = 0;
                elapsedSeconds = 0.0;
            }
		}
        
        NFGE::Graphics::GraphicsSystem::Get()->BeginRender();
        NFGE::Graphics::GraphicsSystem::Get()->EndRender();

	}

    NFGE::Graphics::GraphicsSystem::Get()->StaticTerminate();

	myWindow.Terminate();
	return 0;
}