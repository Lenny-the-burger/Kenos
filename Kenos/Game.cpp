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

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    // TODO: Provide parameters for swapchain format, depth/stencil format, and backbuffer count.
    //   Add DX::DeviceResources::c_AllowTearing to opt-in to variable rate displays.
    //   Add DX::DeviceResources::c_EnableHDR for HDR10 display.
    m_deviceResources->RegisterDeviceNotify(this);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */
}

SceneInformation Game::GetSceneInformation() const noexcept
{
	return localSceneInformation;
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

float rotTemp = 0;

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.

    vector<SceneObject>& sceneObjects = localSceneInformation.getSceneObjects();
    SceneObject& sceneObject = sceneObjects[0];
    
	Vector3 newRot = Vector3(rotTemp, 0, 0);
	sceneObject.SetRotation(newRot);

	rotTemp += 0.01f;

    
    elapsedTime;
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

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();


    // render
    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthNone(), 0);
    context->RSSetState(m_states->CullNone());

    m_effect->Apply(context);

    context->IASetInputLayout(m_inputLayout.Get());

    m_batch->Begin();
    
    
    
	Camera localCamera = localSceneInformation.getCam();

	XMVECTOR camPlane = XMPlaneFromPoints(localCamera.Apos, localCamera.Bpos, localCamera.Cpos);
    
    tuple<Vector3, Vector3, Vector3> currTri;

	Vector3 halfScreenVect = Vector3(1920/2, 1080/2, 0);
    
    for (int globalIndex = 0; globalIndex < localSceneInformation.getGlobalPolyCount(); globalIndex++) {
		currTri = localSceneInformation.getTribyGlobalIndex(globalIndex);
        
		Vector3 v1 = get<0>(currTri);
		Vector3 v2 = get<1>(currTri);
		Vector3 v3 = get<2>(currTri);

        // do backface culling
		Vector3 normal = (v2 - v1).Cross(v3 - v1);
		if (normal.Dot(localCamera.focalPoint - (v1 + v2 + v3)/3) < 0) {
			continue;
		}

        // intersect the camera plane with a line that goes through each vert
        // of the triangle and the focal point
		v1 = XMPlaneIntersectLine(camPlane, localCamera.focalPoint, v1);
		v2 = XMPlaneIntersectLine(camPlane, localCamera.focalPoint, v2);
		v3 = XMPlaneIntersectLine(camPlane, localCamera.focalPoint, v3);

		// normalize points
        v1 = localSceneInformation.untransformFromCam(v1);
		v2 = localSceneInformation.untransformFromCam(v2);
		v3 = localSceneInformation.untransformFromCam(v3);
        
		v1 *= 100;
		v2 *= 100;
		v3 *= 100;

		// assuming a 1920x1080 screen resolution for now
		v1 += halfScreenVect;
		v2 += halfScreenVect;
		v3 += halfScreenVect;

		// flip the y axis because the screen is upside down for some reason
		v1.y = 1080 - v1.y;
		v2.y = 1080 - v2.y;
		v3.y = 1080 - v3.y;

		// Color based on global index
		float idxRatio = (float)globalIndex / (float)localSceneInformation.getGlobalPolyCount();
		Color color = Color{ idxRatio, idxRatio, idxRatio };

		VertexPositionColor screenv1(v1, color);
		VertexPositionColor screenv2(v2, color);
		VertexPositionColor screenv3(v3, color);

		m_batch->DrawTriangle(screenv1, screenv2, screenv3);
    }

    m_batch->End();
    // render
    
    
    context;

    m_deviceResources->PIXEndEvent();

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto const viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
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

void Game::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 800;
    height = 600;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_states = std::make_unique<CommonStates>(device);

    m_effect = std::make_unique<BasicEffect>(device);
    m_effect->SetVertexColorEnabled(true);

    DX::ThrowIfFailed(
        CreateInputLayoutFromEffect<VertexType>(device, m_effect.get(),
            m_inputLayout.ReleaseAndGetAddressOf())
    );

    auto context = m_deviceResources->GetD3DDeviceContext();
    m_batch = std::make_unique<PrimitiveBatch<VertexType>>(context);
    

    // Initialize scene geometry and do the initial visibility data and light tree
    // compute here
	localSceneInformation = SceneInformation("assets/scene.json");
    
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();

    Matrix proj = Matrix::CreateScale(2.f / float(size.right),
        -2.f / float(size.bottom), 1.f)
        * Matrix::CreateTranslation(-1.f, 1.f, 0.f);
    m_effect->SetProjection(proj);
}

void Game::OnDeviceLost()
{
    m_states.reset();
    m_effect.reset();
    m_batch.reset();
    m_inputLayout.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
