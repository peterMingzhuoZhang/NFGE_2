#include <Core\Inc\Core.h>
#include <Grphics/Inc/Graphics.h>
#include <Grphics/Src/D3DUtil.h>

using namespace Microsoft::WRL;
using namespace DirectX;
// Render object resources -------------------------------------------------------------------------|
// -------------------------------------------------------------------------------------------------v
// Vertex buffer for the cube.
Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
// Index buffer for the cube.
Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
D3D12_INDEX_BUFFER_VIEW indexBufferView;

// Depth buffer.
Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer;
// Descriptor heap for depth buffer.
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DSVHeap;
// Root signature
Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

// Pipeline state object.
Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

D3D12_VIEWPORT viewport;
void InitializeViewPort(int w, int h) 
{
    viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));
}

D3D12_RECT scissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX));

struct VertexPC
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Color;
};

static VertexPC cubeVertices[8] = {
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
    { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
    { XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
    { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
    { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
    { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
    { XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
    { XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

static WORD cubeIndicies[36] =
{
    0, 1, 2, 0, 2, 3,
    4, 6, 5, 4, 7, 6,
    4, 5, 1, 4, 1, 0,
    3, 2, 6, 3, 6, 7,
    1, 5, 6, 1, 6, 2,
    4, 0, 3, 4, 3, 7
};

void ResizeDepthBuffer(int width, int height)
{
    // Flush any GPU commands that might be referencing the depth buffer.
    NFGE::Graphics::Flush();

    width = std::max(1, width);
    height = std::max(1, height);

    auto device = NFGE::Graphics::GetDevice();

    // Resize screen dependent resources.
    // Create a depth buffer.
    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil = { 1.0f, 0 };

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
        1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optimizedClearValue,
        IID_PPV_ARGS(&depthBuffer)
    ));

    // Update the depth-stencil view.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    dsv.Flags = D3D12_DSV_FLAG_NONE;

    device->CreateDepthStencilView(depthBuffer.Get(), &dsv,
        DSVHeap->GetCPUDescriptorHandleForHeapStart());
}

void UpdateBufferResource(
    ComPtr<ID3D12GraphicsCommandList2> commandList,
    ID3D12Resource** pDestinationResource,
    ID3D12Resource** pIntermediateResource,
    size_t numElements, size_t elementSize, const void* bufferData,
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
{
    auto device = NFGE::Graphics::GetDevice();

    size_t bufferSize = numElements * elementSize;
    // Create a committed resource for the GPU resource in a default heap.
    {
        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
        ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(pDestinationResource)));
    }
    

    // Create an committed resource for the upload.
    if (bufferData)
    {
        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(pIntermediateResource)));
        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = bufferData;
        subresourceData.RowPitch = bufferSize;
        subresourceData.SlicePitch = subresourceData.RowPitch;

        UpdateSubresources(commandList.Get(),
            *pDestinationResource, *pIntermediateResource,
            0, 0, 1, &subresourceData);
    }
}

// -------------------------------------------------------------------------------------------------^
// Render object resources -------------------------------------------------------------------------|

// Camera resources --------------------------------------------------------------------------------|
// -------------------------------------------------------------------------------------------------v
float m_FoV{45.0};

XMMATRIX modelMatrix;
XMMATRIX viewMatrix;
XMMATRIX projectionMatrix;
// -------------------------------------------------------------------------------------------------^
// Camera resources --------------------------------------------------------------------------------|

bool Load(int width, int height)
{
    auto device = NFGE::Graphics::GetDevice();
    auto commandQueue = NFGE::Graphics::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->GetCommandList();

    // Upload vertex buffer data.
    ComPtr<ID3D12Resource> intermediateVertexBuffer;
    UpdateBufferResource(commandList.Get(),
        &vertexBuffer, &intermediateVertexBuffer,
        _countof(cubeVertices), sizeof(VertexPC), cubeVertices);
    // Create the vertex buffer view.
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = sizeof(cubeVertices);
    vertexBufferView.StrideInBytes = sizeof(VertexPC);
    // Upload index buffer data.
    ComPtr<ID3D12Resource> intermediateIndexBuffer;
    UpdateBufferResource(commandList.Get(),
        &indexBuffer, &intermediateIndexBuffer,
        _countof(cubeIndicies), sizeof(WORD), cubeIndicies);
    // Create index buffer view.
    indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    indexBufferView.SizeInBytes = sizeof(cubeIndicies);

    // Create the descriptor heap for the depth-stencil view.
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&DSVHeap)));

    // Load the vertex shader.
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));

    // Load the pixel shader.
    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderBlob));

    // Create the vertex input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // Create a root signature.
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    // A single 32-bit constant root parameter that is used by the vertex shader.
    CD3DX12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    // Serialize the root signature.
    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
        featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
    // Create the root signature.
    ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
        rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    } pipelineStateStream;

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    pipelineStateStream.pRootSignature = rootSignature.Get();
    pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };
    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipelineState)));

    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);

    // Resize/Create the depth buffer.
    ResizeDepthBuffer(width, height);

    return true;

}

int GetClientWidth(const NFGE::Core::Window& window) 
{
    RECT windowRect = window.GetWindowRECT();
    return (uint32_t)(windowRect.right - windowRect.left);
}
int GetClientHeight(const NFGE::Core::Window& window)
{
    RECT windowRect = window.GetWindowRECT();
    return (uint32_t)(windowRect.bottom - windowRect.top);
}

void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
    Microsoft::WRL::ComPtr<ID3D12Resource> resource,
    D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource.Get(),
        beforeState, afterState);

    commandList->ResourceBarrier(1, &barrier);
}

// Clear a render target.
void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor)
{
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f)
{
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void Render() 
{
    auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
    //auto commandQueue = NFGE::Graphics::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = graphicSystem->GetCurrentCommandList();

    //UINT currentBackBufferIndex = graphicSystem->GetCurrentBackBufferIndex();
    //auto backBuffer = graphicSystem->GetCurrentBackBuffer();
    auto rtv = graphicSystem->GetCurrentRenderTargetView();
    auto dsv = DSVHeap->GetCPUDescriptorHandleForHeapStart();

    //// Clear the render targets.
    //{
    //    TransitionResource(commandList, backBuffer,
    //        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    //    FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

    //    ClearRTV(commandList, rtv, clearColor);
    graphicSystem->BeginRender(NFGE::Graphics::RenderType::Direct);
    ClearDepth(commandList, dsv);
    //}

	commandList->SetPipelineState(pipelineState.Get());
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->IASetIndexBuffer(&indexBufferView);
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    // Update the MVP matrix
    XMMATRIX mvpMatrix = XMMatrixMultiply(modelMatrix, viewMatrix);
    mvpMatrix = XMMatrixMultiply(mvpMatrix, projectionMatrix);
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);
    
    commandList->DrawIndexedInstanced(_countof(cubeIndicies), 1, 0, 0, 0);

    // Present
    /*{
        TransitionResource(commandList, backBuffer,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        mFenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

        currentBackBufferIndex = m_pWindow->Present();

        commandQueue->WaitForFenceValue(m_FenceValues[currentBackBufferIndex]);
    }*/

    graphicSystem->EndRender(NFGE::Graphics::RenderType::Direct);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	NFGE::Core::Window myWindow;
	myWindow.Initialize(hInstance, "Hello Window", 1280, 720);
    NFGE::Graphics::GraphicsSystem::StaticInitialize(myWindow, true, false, 0);

    Load(GetClientWidth(myWindow), GetClientHeight(myWindow));
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
        
        // Update
        float angle = static_cast<float>(elapsedSeconds * 90.0);
        const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
        modelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

        const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
        const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
        const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
        viewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

        float aspectRatio = static_cast<float>(GetClientWidth(myWindow)) / static_cast<float>(GetClientHeight(myWindow));
        projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FoV), aspectRatio, 0.1f, 100.0f);

        Render();
        //NFGE::Graphics::GraphicsSystem::Get()->BeginRender();
        //NFGE::Graphics::GraphicsSystem::Get()->EndRender();
	}

    NFGE::Graphics::GraphicsSystem::StaticTerminate();

	myWindow.Terminate();
	return 0;
}