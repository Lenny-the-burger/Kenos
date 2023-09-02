//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept :
    m_window(nullptr),
    m_outputWidth(WINDOW_SIZE_W),
    m_outputHeight(WINDOW_SIZE_H),
    m_featureLevel(D3D_FEATURE_LEVEL_11_0)
{
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_window = window;
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);

	localSceneInformation = SceneInformation("assets/cornell_box.json");
    localSceneInformation.UpdateScreenSize(WINDOW_SIZE_W, WINDOW_SIZE_H); // set to dflt window size

	localSceneLightingInformation.SetScene(localSceneInformation);
	localSceneLightingInformation.SetScreenRatio((float) m_outputWidth/m_outputHeight);

    origCamPos = localSceneInformation.getCam().Apos;
    camRot = 0.0f;


    // This is really slow and freezes the window for a couple seconds. TODO: fix this.
#ifdef KS_ENABLE_CUSTOM_WINDOW_TITLE
    // Set window title to scene name
    string sceneTitle = "Kenos - " + localSceneInformation.getSceneName();
    LPCSTR sceneTitleName = sceneTitle.c_str();
    SetWindowTextA(window, sceneTitleName);
#endif

    CreateDevice();

    CreateResources();    

    LoadShaders(L"vertex.cso", L"pixel.cso");

    CreateLayout();

    InitVertexBuffer();

    InitConstantBuffer();
    UpdateShaderCameraConstantBuffer();

    InitStructuredBuffers();

    localSceneLightingInformation.BuildLightTree();
	buffer_should_update = true;

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */
}

#pragma region Pipeline initiliazation

// Initializes the various constant buffers used by the shaders:
// 1. Lightmap directory buffer, each primitve has an acosiated lightmap directory
// 2. lightMap buffer, the structure that holds all of the lighting information
// 3. Flattened lightmap buffer, contains the flatteend lightmap for each primitve.
//    this buffer is only written to by the geometry shader.
void Game::InitStructuredBuffers() {

    // Lightmap directory buffer
    D3D11_BUFFER_DESC sbDesc;
    sbDesc.ByteWidth = sizeof(SurfaceLightmapDirectoryPacked) * localSceneInformation.getGlobalPolyCount();
    sbDesc.Usage = D3D11_USAGE_DYNAMIC;
    sbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    sbDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    sbDesc.StructureByteStride = sizeof(SurfaceLightmapDirectoryPacked);

    // Shader Resource View description for the buffer
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = localSceneInformation.getGlobalPolyCount();

    // Create the buffer and its associated SRV for Lightmap directory
    ID3D11ShaderResourceView* lightmapDirSRV = nullptr;
    HRESULT hr = m_d3dDevice->CreateBuffer(&sbDesc, nullptr, &lightmapDirBufferPtr);
    assert(SUCCEEDED(hr));

    m_d3dDevice->CreateShaderResourceView(lightmapDirBufferPtr, &srvDesc, &lightmapDirSRV);

    m_d3dContext->PSSetShaderResources( 0, 1, &lightmapDirSRV );



    // Lightmap buffer (SurfLight structure)
    int lightmapSize = KS_MAX_SURFACE_LIGHTS * localSceneInformation.getGlobalPolyCount();

    sbDesc.ByteWidth = sizeof(SurfLight) * lightmapSize;
    sbDesc.StructureByteStride = sizeof(SurfLight);

    srvDesc.Buffer.NumElements = lightmapSize;

    // Create the buffer and its associated SRV for lightmap
    ID3D11ShaderResourceView* lightMapSRV = nullptr;
    hr = m_d3dDevice->CreateBuffer(&sbDesc, nullptr, &lightMapBufferPtr);
    assert(SUCCEEDED(hr));

    m_d3dDevice->CreateShaderResourceView(lightMapBufferPtr, &srvDesc, &lightMapSRV);

    m_d3dContext->PSSetShaderResources( 1, 1, &lightMapSRV );
}


void Game::InitConstantBuffer() {
    // Create constant buffer with camera matrices. Setting is done in a deperate function
    // Define the constant data used to communicate with shaders

    // Fill in a buffer description.
    D3D11_BUFFER_DESC cbDesc;
    cbDesc.ByteWidth = sizeof(CONSTANT_BUFFER_STRUCT);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;
    cbDesc.StructureByteStride = 0;

    // Get the data from the camera
    Camera localCam = localSceneInformation.getCam();
    XMMATRIX view = localCam.viewMatrix;
    XMMATRIX projection = localCam.projectionMatrix;

    // store initial data in struct
    CONSTANT_BUFFER_STRUCT shaderConstantData = {};
    ZeroMemory(&shaderConstantData, sizeof(CONSTANT_BUFFER_STRUCT));

    shaderConstantData.sampleLarge = KS_SAMPLE_LARGE;
    shaderConstantData.sampleScale = KS_SAMPLE_SCALE;
    
    XMStoreFloat4x4(&shaderConstantData.viewMatrix, XMMatrixTranspose(view));
    XMStoreFloat4x4(&shaderConstantData.projectionMatrix, XMMatrixTranspose(projection));

    shaderConstantData.screenW = WINDOW_SIZE_W;
    shaderConstantData.screenH = WINDOW_SIZE_H;

    shaderConstantData.globalPolyCount = localSceneInformation.getGlobalPolyCount();

    // Fill in the subresource data.
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = &shaderConstantData;
    InitData.SysMemPitch = 0;
    InitData.SysMemSlicePitch = 0;

    // Create the buffer.
    HRESULT hr = m_d3dDevice->CreateBuffer(&cbDesc, &InitData, &constant_buffer_ptr);
    assert(SUCCEEDED(hr));

    // Set the buffer.
    m_d3dContext->PSSetConstantBuffers(0, 1, &constant_buffer_ptr);
    m_d3dContext->VSSetConstantBuffers(0, 1, &constant_buffer_ptr);

}

void Game::InitVertexBuffer() {

	int faceCount = localSceneInformation.getGlobalPolyCount();
    vertex_count = faceCount * 3;
    
	// vertex inputs:                 POSITION
	UINT bufferSize = vertex_count * (3 * sizeof(float));

    vertex_stride = 3 * sizeof(float);
	vertex_offset = 0;

    D3D11_BUFFER_DESC vertex_buff_descr = {};
    
    vertex_buff_descr.ByteWidth = bufferSize;
    vertex_buff_descr.Usage = D3D11_USAGE_DYNAMIC;
    vertex_buff_descr.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vertex_buff_descr.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	float* init_vertex_data = new float[bufferSize/sizeof(float)];

    Camera localCam = localSceneInformation.getCam();

    // set funcy number for finding in memory
    //init_vertex_data[-1] = 12345.6789;

    XMFLOAT4X4 viewDebug;
    XMStoreFloat4x4(&viewDebug, XMMatrixTranspose(localCam.viewMatrix));
    XMFLOAT4X4 projDebug;
    XMStoreFloat4x4(&projDebug, XMMatrixTranspose(localCam.projectionMatrix));

    // load vertex data into buffer
    for (int i = 0; i < faceCount; i++) {
		// get the vertex data from the scene
        tuple<Vector3, Vector3, Vector3> face = localSceneInformation.getTribyGlobalIndex(i);

        // temp debug transforms
        Vector3 v1 = get<0>(face);
        v1 = XMVector3Transform(v1, localCam.viewMatrix);
        v1 = XMVector3Transform(v1, localCam.projectionMatrix);

		// store the vertex data in the buffer
        init_vertex_data[(9 * i) + 0] = get<0>(face).x;
        init_vertex_data[(9 * i) + 1] = get<0>(face).y;
        init_vertex_data[(9 * i) + 2] = get<0>(face).z;

        init_vertex_data[(9 * i) + 3] = get<1>(face).x;
        init_vertex_data[(9 * i) + 4] = get<1>(face).y;
        init_vertex_data[(9 * i) + 5] = get<1>(face).z;

        init_vertex_data[(9 * i) + 6] = get<2>(face).x;
        init_vertex_data[(9 * i) + 7] = get<2>(face).y;
        init_vertex_data[(9 * i) + 8] = get<2>(face).z;
	}

    D3D11_SUBRESOURCE_DATA vertexBufferData;
    vertexBufferData.pSysMem = init_vertex_data;
    vertexBufferData.SysMemPitch = 0;
    vertexBufferData.SysMemSlicePitch = 0;
    
    HRESULT hr = m_d3dDevice->CreateBuffer(
        &vertex_buff_descr,
        &vertexBufferData,
        &vertex_buffer_ptr);
    assert(SUCCEEDED(hr));

	// cleanup temp data
	delete[] init_vertex_data;
}

void Game::CreateLayout()
{   
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        // TODO: add other per vertex data here
    };
    HRESULT hr = m_d3dDevice->CreateInputLayout(
        inputElementDesc,
        ARRAYSIZE(inputElementDesc),
        vertex_shader_blob->GetBufferPointer(),
        vertex_shader_blob->GetBufferSize(),
        &input_layout_ptr);
    assert(SUCCEEDED(hr));
}

/* Function to load compiled shaders. Requires the vertex, geometry, and pixel shader paths.
* !! Shader version must be xs_4_0, xs_4_1, or xs_5_0 !!
*/
void Game::LoadShaders(const wchar_t* vs_path, const wchar_t* ps_path)
{
    // Load compiled vertex shader
    HRESULT result = D3DReadFileToBlob(vs_path, &vertex_shader_blob);
    assert(SUCCEEDED(result) && "Failed to load vertex shader bytecode.");

    // Create vertex shader
    result = m_d3dDevice->CreateVertexShader(
        vertex_shader_blob->GetBufferPointer(),
        vertex_shader_blob->GetBufferSize(),
        nullptr,
        &vertex_shader_ptr);
    assert(SUCCEEDED(result) && "Failed to create vertex shader.");

    // Load compiled pixel shader
    result = D3DReadFileToBlob(ps_path, &pixel_shader_blob);
    assert(SUCCEEDED(result) && "Failed to load pixel shader bytecode.");

    // Create pixel shader
    result = m_d3dDevice->CreatePixelShader(
        pixel_shader_blob->GetBufferPointer(),
        pixel_shader_blob->GetBufferSize(),
        nullptr,
        &pixel_shader_ptr);
    assert(SUCCEEDED(result) && "Failed to create pixel shader.");
}

SceneInformation Game::GetSceneInformation() const noexcept
{
	return localSceneInformation;
}
#pragma endregion

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    // Everything that happens from here until we are done rendering the frame will
    // have to wait until the next frame to be shown.
    if (buffer_should_update) {
        // Update the buffer
        localSceneLightingInformation.UpdateFinalRDFBuffer();
        UpdateStructuredBuffers();
    }

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here
    //Vector3 offset = Vector3(sin(camRot), cos(camRot), 0);
    //localSceneInformation.setCameraPos(origCamPos + offset);
    //UpdateShaderCameraConstantBuffer();

    camRot += 0.01f;

    elapsedTime;
}

void Game::SetShouldUpdate(bool should) {
    buffer_should_update = should;
}

void Game::UpdateStructuredBuffers() {

    //  1. Update lightmap directory buffer
    vector<SurfaceLightmapDirectoryPacked> newDirs = localSceneLightingInformation.GetDirectoryBuffer();

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

    int lightmapCount = newDirs.size();

    SurfaceLightmapDirectoryPacked* dataPtr = new SurfaceLightmapDirectoryPacked[lightmapCount];

    ZeroMemory(dataPtr, sizeof(SurfaceLightmapDirectoryPacked) * lightmapCount);

    for (int i = 0; i < lightmapCount; i++) {
		dataPtr[i] = newDirs[i];
	}

    m_d3dContext->Map(lightmapDirBufferPtr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    memcpy(mappedResource.pData, dataPtr, sizeof(SurfaceLightmapDirectoryPacked) * lightmapCount);

    m_d3dContext->Unmap(lightmapDirBufferPtr, 0);

    delete[] dataPtr;



    //  2. Update lightmap data buffer
    map<int, vector<SurfLight>> newLightmap = localSceneLightingInformation.GetFinalLightmapBuffer();

    D3D11_MAPPED_SUBRESOURCE mappedResource2; // no idea if we can reuse the old one or not
    ZeroMemory(&mappedResource2, sizeof(D3D11_MAPPED_SUBRESOURCE));

    int pcount = localSceneInformation.getGlobalPolyCount();

    SurfLight* dataPtr2 = new SurfLight[pcount * KS_MAX_SURFACE_LIGHTS];

    ZeroMemory(dataPtr2, sizeof(SurfLight) * pcount * KS_MAX_SURFACE_LIGHTS);

    for (int i = 0; i < pcount; i++) {
        vector<SurfLight> currSurfLights = newLightmap[i];
        int lightCount = currSurfLights.size();

        for (int j = 0; j < KS_MAX_SURFACE_LIGHTS; j++) {
			// Check if the surface light exists
            if (j < lightCount) {
                dataPtr2[i * KS_MAX_SURFACE_LIGHTS + j] = currSurfLights[j];
            }
            // Otherwise we could add an empty light, but we can just leave it as
            // random garbage since the shader will ignore it anyway
		}
    }

    m_d3dContext->Map(lightMapBufferPtr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource2);

    memcpy(mappedResource2.pData, dataPtr2, sizeof(SurfLight) * pcount * KS_MAX_SURFACE_LIGHTS);

    m_d3dContext->Unmap(lightMapBufferPtr, 0);

    delete[] dataPtr2;
}

// Get the current camera matrices and map into constant buffer
void Game::UpdateShaderCameraConstantBuffer() {
    Camera localCamera = localSceneInformation.getCam();

    // Get the data from the camera
    Camera localCam = localSceneInformation.getCam();
    XMMATRIX view = localCam.viewMatrix;
    XMMATRIX projection = localCam.projectionMatrix;

    // store initial data in struct
    CONSTANT_BUFFER_STRUCT shaderConstantData = {};
    ZeroMemory(&shaderConstantData, sizeof(CONSTANT_BUFFER_STRUCT));

    shaderConstantData.sampleLarge = KS_SAMPLE_LARGE;
    shaderConstantData.sampleScale = KS_SAMPLE_SCALE;

    XMStoreFloat4x4(&shaderConstantData.viewMatrix, XMMatrixTranspose(view));
    XMStoreFloat4x4(&shaderConstantData.projectionMatrix, XMMatrixTranspose(projection));

    shaderConstantData.screenW = m_outputWidth;
    shaderConstantData.screenH = m_outputHeight;

    shaderConstantData.globalPolyCount = localSceneInformation.getGlobalPolyCount();

    // Map the camera matrices into the constant buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

    // the constant buffer isnt a homoegenous array, but a struct
    // so we need to map the resource to the struct
    m_d3dContext->Map(constant_buffer_ptr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    memcpy(mappedResource.pData, &shaderConstantData, sizeof(CONSTANT_BUFFER_STRUCT));
    //mappedResource.pData = &shaderConstantData;

    m_d3dContext->Unmap(constant_buffer_ptr, 0);
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    // Start rendering
    m_d3dContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    m_d3dContext->IASetInputLayout( input_layout_ptr );
    m_d3dContext->IASetVertexBuffers(
        0,
        1,
        &vertex_buffer_ptr,
        &vertex_stride,
        &vertex_offset);
    
    m_d3dContext->VSSetShader(vertex_shader_ptr, NULL, 0);
    m_d3dContext->PSSetShader(pixel_shader_ptr, NULL, 0);
    
    m_d3dContext->Draw(vertex_count, 0);
    
    Present();

	buffer_should_update = false;
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    // Clear the views.
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::Black);
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    // Set the viewport.
    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight), 0.f, 1.f };
    m_d3dContext->RSSetViewports(1, &viewport);
}

// Presents the back buffer contents to the screen.
void Game::Present()
{
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        OnDeviceLost();
    }
    else
    {
        DX::ThrowIfFailed(hr);
    }
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowSizeChanged(int width, int height)
{
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);

    CreateResources();

    // TODO: Game window is being resized.

    // update scene information screen size
    localSceneInformation.UpdateScreenSize(m_outputWidth, m_outputHeight);

    // Update camera information in the shader
    UpdateShaderCameraConstantBuffer();

    // update lighting information screen ratio and force update
	localSceneLightingInformation.SetScreenRatio((float) m_outputWidth/m_outputHeight);
    buffer_should_update = true;
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = WINDOW_SIZE_W;
	height = WINDOW_SIZE_H;
}
#pragma endregion

#pragma region Device resources
// These are the resources that depend on the device.
void Game::CreateDevice()
{
    UINT creationFlags = 0;

#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    static const D3D_FEATURE_LEVEL featureLevels[] =
    {
        // TODO: Modify for supported Direct3D feature levels
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };

    // Create the DX11 API device object, and get a corresponding context.
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    DX::ThrowIfFailed(D3D11CreateDevice(
        nullptr,                            // specify nullptr to use the default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        creationFlags,
        featureLevels,
        static_cast<UINT>(std::size(featureLevels)),
        D3D11_SDK_VERSION,
        device.ReleaseAndGetAddressOf(),    // returns the Direct3D device created
        &m_featureLevel,                    // returns feature level of device created
        context.ReleaseAndGetAddressOf()    // returns the device immediate context
    ));

#ifndef NDEBUG
    ComPtr<ID3D11Debug> d3dDebug;
    if (SUCCEEDED(device.As(&d3dDebug)))
    {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D11_MESSAGE_ID hide[] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                // TODO: Add more message IDs here as needed.
            };
            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    DX::ThrowIfFailed(device.As(&m_d3dDevice));
    DX::ThrowIfFailed(context.As(&m_d3dContext));

    // TODO: Initialize device dependent objects here (independent of window size).
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
    // Clear the previous window size specific context.
    m_d3dContext->OMSetRenderTargets(0, nullptr, nullptr);
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_d3dContext->Flush();

    const UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
    const UINT backBufferHeight = static_cast<UINT>(m_outputHeight);
    const DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    const DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    constexpr UINT backBufferCount = 2;

    // If the swap chain already exists, resize it, otherwise create one.
    if (m_swapChain)
    {
        HRESULT hr = m_swapChain->ResizeBuffers(backBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            OnDeviceLost();

            // Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
    }
    else
    {
        // First, retrieve the underlying DXGI Device from the D3D Device.
        ComPtr<IDXGIDevice1> dxgiDevice;
        DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

        // Identify the physical adapter (GPU or card) this device is running on.
        ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

        // And obtain the factory object that created it.
        ComPtr<IDXGIFactory2> dxgiFactory;
        DX::ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = backBufferCount;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        // Create a SwapChain from a Win32 window.
        DX::ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
            m_d3dDevice.Get(),
            m_window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            m_swapChain.ReleaseAndGetAddressOf()
        ));

        // This template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut.
        DX::ThrowIfFailed(dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));
    }

    // Obtain the backbuffer for this window which will be the final 3D rendertarget.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));

    // Create a view interface on the rendertarget to use on bind.
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));

    // Allocate a 2-D surface as the depth/stencil buffer and
    // create a DepthStencil view on this surface to use on bind.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf()));

    DX::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), nullptr, m_depthStencilView.ReleaseAndGetAddressOf()));

    // TODO: Initialize windows-size dependent objects here.
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.

    m_depthStencilView.Reset();
    m_renderTargetView.Reset();
    m_swapChain.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();

    CreateDevice();

    CreateResources();
}
#pragma endregion