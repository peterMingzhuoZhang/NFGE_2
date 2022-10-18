#include <Core/Inc/Core.h>
#include <Grphics/Inc/Graphics.h>
#include "Grphics/Src/D3DUtil.h" // hack
#include <NFGE_2/Inc/NFGE_2.h>

NFGE::Graphics::DirectionalLight myLight;
NFGE::Graphics::Camera myCamera;
NFGE::Graphics::GeometryPX myBall;

void Load()
{
    myLight.direction = NFGE::Math::Normalize({ 1.0f,-1.0f,-1.0f });
    myLight.ambient = { 0.3f, 0.3f, 0.3f, 1.0f };
    myLight.diffuse = { 0.6f, 0.6f, 0.6f, 1.0f };
    myLight.specular = { 1.0f, 1.0f, 1.0f, 1.0f };

    myCamera.SetDirection({ 0.0f,0.0f,1.0f });
    myCamera.SetPosition(0.0f);

    myBall.Load(NFGE::Graphics::MeshBuilder::CreateSpherePX(100, 100, 10), &myLight);
    myBall.mMeshRenderStrcuture.mTexture_0 = NFGE::Graphics::TextureManager::Get()->LoadTexture("texcoord.png", NFGE::Graphics::TextureUsage::Albedo, true);
    myBall.mMeshContext.position = { 0.0f,0.0f, 40.0f };

    auto commandQueue = NFGE::Graphics::GetCommandQueue(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE);
    auto commandList = NFGE::Graphics::GraphicsSystem::Get()->GetCurrentCommandList();
    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);
}

void Render()
{
    myBall.Render(myCamera);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    NFGE::Core::Window myWindow;
    myWindow.Initialize(hInstance, "Hello Window", 1280, 720, false, NULL);
    NFGE::Graphics::GraphicsSystem::StaticInitialize(myWindow, true, false, 0);
    std::filesystem::path assetsDirectory = L"../../Assets/Images";
    NFGE::Graphics::TextureManager::StaticInitialize(assetsDirectory);

    // Load
    Load();
    
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

        // Render
        {
            auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
            graphicSystem->BeginRender(NFGE::Graphics::RenderType::Direct);
            Render();
            //NFGE::Graphics::Texture* texture = NFGE::Graphics::TextureManager::Get()->GetTexture(myBall.mMeshRenderStrcuture.mTexture_0);
            //auto rtv = texture->GetRenderTargetView();
            //auto dsv = texture->GetDepthStencilView();
            //graphicSystem->GetCurrentCommandList()->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
            graphicSystem->EndRender(NFGE::Graphics::RenderType::Direct);
            graphicSystem->Reset();
        }
        
    }

    NFGE::Graphics::TextureManager::StaticTerminate();
    NFGE::Graphics::GraphicsSystem::StaticTerminate();

    myWindow.Terminate();
    return 0;
}