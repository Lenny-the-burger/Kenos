//
// Game.h
//

#pragma once

#include "StepTimer.h"
#include "d3dcompiler.h"
#include <stdio.h>

#include "EngineConstants.h"

#include "Mesh.h"
#include "Material.h"
#include "SceneObject.h"
#include "SceneInformation.h"
#include "SceneLightingInformation.h"

#include <vector>
#include <map>

// Weather to set the scene name in the window title, as it causes a small delay on startup
//#define KS_ENABLE_CUSTOM_WINDOW_TITLE

// shh padding i kno
#pragma warning(disable: 4324)

// this is a constant buffer so needs to be a multiple of 16

struct alignas(16) CONSTANT_BUFFER_STRUCT
{
    DirectX::XMFLOAT4X4 viewMatrix;
    DirectX::XMFLOAT4X4 projectionMatrix;
    int sampleLarge;
    float sampleScale;
};

struct alignas(16) SceneObjectsDataCBuffer
{
    int numObjects;
    int objectPolyCount[KS_MAX_SCENEOBJECTS];
    DirectX::XMFLOAT4X4 objectTransforms[KS_MAX_SCENEOBJECTS];
};

// enable padding warnings again
#pragma warning(default: 4324)

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game final
{
public:

    Game() noexcept;
    ~Game() = default;

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;
	
    void SetShouldUpdate(bool should);

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    
    void Clear();
    void Present();

    void CreateDevice();
    void CreateResources();

    void OnDeviceLost();

    void LoadShaders(const wchar_t* vs_path, const wchar_t* ps_path);

    void CreateLayout();
    void InitVertexBuffer();

    void InitConstantBuffer();
    void UpdateShaderCameraConstantBuffer();

    void InitStructuredBuffers();
    void UpdateStructuredBuffers();
    
    // Scene information
    SceneInformation GetSceneInformation() const noexcept;

    // Device resources.
    HWND                                            m_window;
    int                                             m_outputWidth;
    int                                             m_outputHeight;

    D3D_FEATURE_LEVEL                               m_featureLevel;
    Microsoft::WRL::ComPtr<ID3D11Device1>           m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_d3dContext;

    Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;

    // Rendering loop timer.
    DX::StepTimer                                   m_timer;
    
    // Scene information
    SceneInformation localSceneInformation;
	SceneLightingInformation localSceneLightingInformation;

    // does vertex buffer need to be rewritten? saves time when camera isnt moved
    bool buffer_should_update;

    // Flags used for internal stuff
    UINT flags;
    
	// Shaders
    ID3D11VertexShader* vertex_shader_ptr;
    ID3D11GeometryShader* geometry_shader_ptr;
    ID3D11PixelShader* pixel_shader_ptr;

    ID3DBlob* vertex_shader_blob;
    ID3DBlob* geometry_shader_blob;
    ID3DBlob* pixel_shader_blob;
    
    // iput layout
    ID3D11InputLayout* input_layout_ptr;

    // Buffer stuff
    ID3D11Buffer* constant_buffer_ptr;
    ID3D11Buffer* vertex_buffer_ptr;
    UINT vertex_stride;
    UINT vertex_offset;
    UINT vertex_count;

    ID3D11Buffer* lightmapDirBufferPtr;
    ID3D11Buffer* lightMapBufferPtr;

    // write unindexed vertex data to this in 3s, this will be written to the 
    // buffer when buffer_should_update == true. Remember to also 
    /*float& new_vertex_buffer_data_ptr;*/
};
;