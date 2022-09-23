#include <Core/Inc/Core.h>
#include <Grphics/Inc/Graphics.h>
#include <NFGE_2/Inc/NFGE_2.h>

NFGE::Graphics::Geometry myBall;
NFGE::Graphics::DirectionalLight myLight;

void Load()
{
    myLight.direction = NFGE::Math::Normalize({ 1.0f,-1.0f,-1.0f });
    myLight.ambient = { 0.3f, 0.3f, 0.3f, 1.0f };
    myLight.diffuse = { 0.6f, 0.6f, 0.6f, 1.0f };
    myLight.specular = { 1.0f, 1.0f, 1.0f, 1.0f };

    
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    NFGE::Core::Window myWindow;
    myWindow.Initialize(hInstance, "Hello Window", 1280, 720, false, NULL);
    NFGE::Graphics::GraphicsSystem::StaticInitialize(myWindow, true, false, 0);

    
    bool done = false;

    while (!done)
    {
        done = myWindow.ProcessMessage();

        // run your game logic ...
        static uint64_t frameCounter = 0;
        static double totalSeconds = 0.0;
        static double elapsedSeconds = 0.0;
        static std::chrono::high_resolution_clock clock;
        static auto t0 = clock.now();

        {// time and delta time block
            frameCounter++;
            auto t1 = clock.now();
            auto deltaTime = t1 - t0;
            t0 = t1;

            elapsedSeconds += deltaTime.count() * 1e-9;
            totalSeconds += deltaTime.count() * 1e-9;
            if (elapsedSeconds > 1.0)
            {
                char buffer[500];
                auto fps = frameCounter / elapsedSeconds;
                sprintf_s(buffer, 500, "FPS: %f\n", fps);
                LOG("FPS: %f", fps);

                frameCounter = 0;
                elapsedSeconds = 0.0;
            }
        }
    }

    NFGE::Graphics::GraphicsSystem::StaticTerminate();

    myWindow.Terminate();
    return 0;
}