//
// Game.h
//

#pragma once

#include "StepTimer.h"
#include "d3dcompiler.h"
#include <stdio.h>

#include "Mesh.h"
#include "Material.h"
#include "SceneObject.h"
#include "SceneInformation.h"
#include "SceneLightingInformation.h"

#include <vector>
#include <map>

#define WINDOW_SIZE_W 1000
#define WINDOW_SIZE_H 800

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

    HRESULT CompileShader(_In_ LPCWSTR srcFile, _In_ LPCSTR entryPoint, _In_ LPCSTR profile, _Outptr_ ID3DBlob** blob);
    void LoadShaders(const wchar_t* vs_path, const wchar_t* ps_path);
    void CreateLayout();
    void InitVertexBuffer();

    void MapNewBufferData();
    
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
    ID3D11PixelShader* pixel_shader_ptr;

    ID3DBlob* vertex_shader_blob;
    ID3DBlob* pixel_shader_blob;

    // iput layout
    ID3D11InputLayout* input_layout_ptr;

    // Buffer stuff
    ID3D11Buffer* vertex_buffer_ptr;
    UINT vertex_stride;
    UINT vertex_offset;
    UINT vertex_count;

    // write unindexed vertex data to this in 3s, this will be written to the 
    // buffer when buffer_should_update == true. Remember to also 
    /*float& new_vertex_buffer_data_ptr;*/
};
;