//
// Game.cpp
//

#include "pch.h"
#include "Game.h"


//toreorganise
#include <fstream>

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace ImGui;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
#ifdef DXTK_AUDIO
    if (m_audEngine)
    {
        m_audEngine->Suspend();
    }
#endif
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_window = window;
	m_input.Initialise(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

	//setup imgui.  its up here cos we need the window handle too
	//pulled from imgui directx11 example
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(window);		//tie to our window
	ImGui_ImplDX11_Init(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext());	//tie to directx

	m_fullscreenRect.left = 0;
	m_fullscreenRect.top = 0;
	m_fullscreenRect.right = 800;
	m_fullscreenRect.bottom = 600;

	m_CameraViewRect.left = 500;
	m_CameraViewRect.top = 0;
	m_CameraViewRect.right = 800;
	m_CameraViewRect.bottom = 240;

	//setup light
	m_Light.setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
	m_Light.setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
	m_Light.setPosition(2.0f, 1.0f, 1.0f);
	m_Light.setDirection(-1.0f, -1.0f, 0.0f);

	//setup camera
	m_Camera01.setPosition(Vector3(6.0f, 15.0f, 5.0f));
	m_Camera01.setRotation(Vector3(180.0f, 180.0f, 0.0f));	//orientation is -90 becuase zero will be looking up at the sky straight up. 

    m_red = 0.0;
    m_green = 0.0;
    m_blue = 0.0;
    m_total_seconds = 0.0;
    clicked = false;
    rotating = false;
    is_fullScreen = false;

    m_Sound = new SoundClass;
    bool result = m_Sound->Initialize(window);

#ifdef DXTK_AUDIO
    // Create DirectXTK for Audio objects
    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif

    m_audEngine = std::make_unique<AudioEngine>(eflags);

    m_audioEvent = 0;
    m_audioTimerAcc = 10.f;
    m_retryDefault = false;

    m_waveBank = std::make_unique<WaveBank>(m_audEngine.get(), L"adpcmdroid.xwb");

    m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"MusicMono_adpcm.wav");
    m_effect1 = m_soundEffect->CreateInstance();
    m_effect2 = m_waveBank->CreateInstance(10);

    m_effect1->Play(true);
    m_effect2->Play();
#endif
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
	//take in input
	m_input.Update();								//update the hardware
	m_gameInputCommands = m_input.getGameInput();	//retrieve the input for our game
	
	//Update all game objects
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

	//Render all game content. 
    Render();

#ifdef DXTK_AUDIO
    // Only update audio engine once per frame
    if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
#endif

	
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{	
    GetWindowRect(m_window, &m_fullscreenRect);

	//this is hacky,  i dont like this here.  
	auto device = m_deviceResources->GetD3DDevice();

	//note that currently.  Delta-time is not considered in the game object movement. 
	if (m_gameInputCommands.left)
	{
		Vector3 rotation = m_Camera01.getRotation();
		rotation.y = rotation.y += m_Camera01.getRotationSpeed();
		m_Camera01.setRotation(rotation);
	}
	if (m_gameInputCommands.right)
	{
		Vector3 rotation = m_Camera01.getRotation();
		rotation.y = rotation.y -= m_Camera01.getRotationSpeed();
		m_Camera01.setRotation(rotation);
	}
	if (m_gameInputCommands.forward)
	{
		Vector3 position = m_Camera01.getPosition(); //get the position
		position += (m_Camera01.getForward()*m_Camera01.getMoveSpeed()); //add the forward vector
		m_Camera01.setPosition(position);
	}
	if (m_gameInputCommands.back)
	{
		Vector3 position = m_Camera01.getPosition(); //get the position
		position -= (m_Camera01.getForward()*m_Camera01.getMoveSpeed()); //add the forward vector
		m_Camera01.setPosition(position);
	}

    if (m_gameInputCommands.up) {
        Vector3 position = m_Camera01.getPosition(); //get the position
        position += (m_Camera01.getUp() * m_Camera01.getMoveSpeed()); //add the forward vector
        m_Camera01.setPosition(position);
    }

    if (m_gameInputCommands.down) {
        Vector3 position = m_Camera01.getPosition(); //get the position
        position -= (m_Camera01.getUp() * m_Camera01.getMoveSpeed()); //add the forward vector
        m_Camera01.setPosition(position);
    }

    if (m_gameInputCommands.upRotate)
    {
        Vector3 rotation = m_Camera01.getRotation();
        rotation.x = rotation.x -= m_Camera01.getRotationSpeed();
        m_Camera01.setRotation(rotation);
    }
    if (m_gameInputCommands.downRotate)
    {
        Vector3 rotation = m_Camera01.getRotation();
        rotation.x = rotation.x += m_Camera01.getRotationSpeed();
        m_Camera01.setRotation(rotation);
    }

	if (m_gameInputCommands.generate)
	{
		m_Terrain.GenerateHeightMap(device);
	}

    if (m_gameInputCommands.smooth)
    {
        m_Terrain.Smooth(device);
    }
    if (m_gameInputCommands.perlin)
    {
        m_Terrain.PerlinNoise(device);
    }
    if (m_gameInputCommands.cellular)
    {
        m_Terrain.CellularAutomata(device);
    }
    if (m_gameInputCommands.Conway)
    {
        m_Terrain.ConwayGame(device);
    }

    if (m_gameInputCommands.initialCellAlive)
    {
        m_Terrain.InitiliseAliveStatus(device);
    }
    
    if (m_gameInputCommands.leftButton && (m_timer.GetTotalSeconds() - click_time) > 0.3 ) {
        if (TestIntersection(m_gameInputCommands.mouseX, m_gameInputCommands.mouseY)) {
            clicked = true;
            rotating = !rotating;
        }
        else {
            clicked = false;
        }
        click_time = m_timer.GetTotalSeconds();
    }



	m_Camera01.Update();	//camera update.
	m_Terrain.Update();		//terrain update.  doesnt do anything at the moment. 

	m_view = m_Camera01.getCameraMatrix();
	m_world = Matrix::Identity;

	/*create our UI*/
	SetupGUI();

#ifdef DXTK_AUDIO
    m_audioTimerAcc -= (float)timer.GetElapsedSeconds();
    if (m_audioTimerAcc < 0)
    {
        if (m_retryDefault)
        {
            m_retryDefault = false;
            if (m_audEngine->Reset())
            {
                // Restart looping audio
                m_effect1->Play(true);
            }
        }
        else
        {
            m_audioTimerAcc = 4.f;

            m_waveBank->Play(m_audioEvent++);

            if (m_audioEvent >= 11)
                m_audioEvent = 0;
        }
    }
#endif

  
	if (m_input.Quit())
	{
		ExitGame();
	}
}
#pragma endregion

//bool Game::TestIntersection(int mouseX, int mouseY) {
//    float pointX, pointY;
//    float m_screenWidth = 800.0f;
//    float m_screenHeight = 600.0f;
//    float dist = 1000.0f;
//
//    XMMATRIX projectionMatrix, viewMatrix, inverseViewMatrix, worldMatrix, translateMatrix, inverseWorldMatrix;
//    XMFLOAT3 rayOrigin, rayDirection;
//    XMVECTOR direction, origin;
//    
//    bool intersect, result;
//
//
//    // Move the mouse cursor coordinates into the -1 to +1 range.
//    pointX = ((2.0f * (float)mouseX) / (float)m_screenWidth) - 1.0f;
//    pointY = (((2.0f * (float)mouseY) / (float)m_screenHeight) - 1.0f) ;
//
//    // Adjust the points using the projection matrix to account for the aspect ratio of the viewport.
//    projectionMatrix = m_projection;
//    pointX = pointX / XMVectorGetX(projectionMatrix.r[0]);
//    pointY = pointY / XMVectorGetY(projectionMatrix.r[1]);
//
//    // Get the inverse of the view matrix.
//
//    m_view = m_Camera01.getCameraMatrix();
//    viewMatrix = m_view;
//
//    inverseViewMatrix = XMMatrixInverse(NULL, viewMatrix);
//
//    XMFLOAT3 temp_dir;
//
//    //direction.x = (pointX * inverseViewMatrix) + (pointY * inverseViewMatrix) + inverseViewMatrix;
//    //direction.y = (pointX * inverseViewMatrix) + (pointY * inverseViewMatrix) + inverseViewMatrix;
//    //direction.z = (pointX * inverseViewMatrix) + (pointY * inverseViewMatrix) + inverseViewMatrix;
//
//    //direction.x = (pointX * inverseViewMatrix._11) + (pointY * inverseViewMatrix._21) + inverseViewMatrix._31;
//    //direction.y = (pointX * inverseViewMatrix._12) + (pointY * inverseViewMatrix._22) + inverseViewMatrix._32;
//    //direction.z = (pointX * inverseViewMatrix._13) + (pointY * inverseViewMatrix._23) + inverseViewMatrix._33;
//
//    temp_dir.x = (pointX * XMVectorGetX(inverseViewMatrix.r[0]) + pointY * XMVectorGetY(inverseViewMatrix.r[0]) + XMVectorGetZ(inverseViewMatrix.r[0]));
//    temp_dir.y = (pointX * XMVectorGetX(inverseViewMatrix.r[1]) + pointY * XMVectorGetY(inverseViewMatrix.r[1]) + XMVectorGetZ(inverseViewMatrix.r[1]));
//    temp_dir.z = (pointX * XMVectorGetX(inverseViewMatrix.r[2]) + pointY * XMVectorGetY(inverseViewMatrix.r[2]) + XMVectorGetZ(inverseViewMatrix.r[2]));
//
//    direction = XMLoadFloat3(&temp_dir);
//    // Get the origin of the picking ray which is the position of the camera.
//    origin = m_Camera01.getPosition();
//
//    // Get the world matrix and translate to the location of the sphere.
//    m_world = SimpleMath::Matrix::Identity;
//    worldMatrix = m_world;
////    translateMatrix = XMMatrixTranslation(-5.0f, 1.0f, 5.0f);
//    translateMatrix = XMMatrixTranslation(2.0f, 3.0f, 0.0f);
//
//    worldMatrix = XMMatrixMultiply(worldMatrix, translateMatrix);
//
//
//    // Now get the inverse of the translated world matrix.
//    inverseWorldMatrix = XMMatrixInverse(NULL, worldMatrix);
//
//    // Now transform the ray origin and the ray direction from view space to world space.
//    origin = XMVector3TransformCoord(origin, inverseWorldMatrix);
//    direction = XMVector3TransformNormal(direction, inverseWorldMatrix);
//
//    // Normalize the ray direction.
//    direction = XMVector3Normalize(direction);
//
//
//    XMStoreFloat3(&rayOrigin, origin);
//    XMStoreFloat3(&rayDirection, direction);
//
//     //Now perform the ray-sphere intersection test.
//    if (RaySphereIntersect(rayOrigin, rayDirection, 1.0f)) {
//        return true;
//    }
//
//
//    //if (m_sphere.Intersects(origin, XMVector3Normalize(direction), dist)) {
//    //    return true;
//    //    }
//
//    return false;
//}

bool Game::TestIntersection(int mouseX, int mouseY) {

    float m_screenWidth = m_fullscreenRect.right - m_fullscreenRect.left;
    float m_screenHeight = m_fullscreenRect.bottom - m_fullscreenRect.top;

    XMMATRIX world, view, proj;
    m_world = SimpleMath::Matrix::Identity;
    world = m_world;
    m_view = m_Camera01.getCameraMatrix();
    view = m_view;
    proj = m_projection;

    XMVECTOR originScreen,directionScreen;
    XMVECTOR origin, direction;
    originScreen = XMVectorSet(mouseX, mouseY, 0.1, 1.0f);
    directionScreen = XMVectorSet(mouseX, mouseY, 1, 1.0f);

    origin = XMVector3Unproject(originScreen, 0, 0, m_screenWidth, m_screenHeight, 0, 1, proj, view, world);
    direction = XMVector3Unproject(directionScreen, 0, 0, m_screenWidth, m_screenHeight, 0, 1, proj, view, world);
    direction = XMVector3Normalize(direction - origin);

    SimpleMath::Ray ray(origin, direction);
    float dist;

    bool rayHit = m_sphere.Intersects(origin, direction, dist);
    return rayHit;
}
bool Game::RaySphereIntersect(XMFLOAT3 rayOrigin, XMFLOAT3 rayDirection, float radius) {
    float a, b, c, discriminant;
    XMFLOAT3 Origin_dis = rayOrigin;
    m_rayOrigin = rayOrigin;
    m_rayDirection = rayDirection;
    //Origin_dis.x  -= 2.0;
    //Origin_dis.y -= 3.0;
    //Origin_dis.z -= 0.0;
    // Calculate the a, b, and c coefficients.
   // rayDirection = m_Camera01.getForward();
    a = (rayDirection.x * rayDirection.x) + (rayDirection.y * rayDirection.y) + (rayDirection.z * rayDirection.z);
    b = ((rayDirection.x * Origin_dis.x) + (rayDirection.y * Origin_dis.y) + (rayDirection.z * Origin_dis.z)) * 2.0f;
    c = ((Origin_dis.x * Origin_dis.x) + (Origin_dis.y * Origin_dis.y) + (Origin_dis.z * Origin_dis.z)) - (radius * radius);

    // Find the discriminant.
    discriminant = (b * b) - (4 * a * c);

    // if discriminant is negative the picking ray missed the sphere, otherwise it intersected the sphere.
    if (discriminant < 0.0f)
    {
        return false;
    }

    return true;
}



#pragma region Frame Render
// Draws the scene.
void Game::Render()
{

    if (rotating == true) {
        m_total_seconds -= m_timer.GetElapsedSeconds();
    }
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTargetView = m_deviceResources->GetRenderTargetView();
	auto depthTargetView = m_deviceResources->GetDepthStencilView();

    // Draw Text to the screen
  //  m_sprites->Begin();
		//m_font->DrawString(m_sprites.get(), L"Procedural Methods", XMFLOAT2(10, 10), Colors::Yellow);
  //  m_sprites->End();

	//Set Rendering states. 
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	context->RSSetState(m_states->CullClockwise());
    //context->RSSetState(m_states->Wireframe());

    ReflectionRenderPass();

	//prepare transform for floor object. 
	m_world = SimpleMath::Matrix::Identity; //set world back to identity
	SimpleMath::Matrix newPosition3 = SimpleMath::Matrix::CreateTranslation(0.0f, -0.6f, 0.0f);
	SimpleMath::Matrix newScale = SimpleMath::Matrix::CreateScale(0.1);		//scale the terrain down a little. 
	m_world = m_world * newScale *newPosition3;
    m_reflectionShader.EnableReflectionShader(context);

    m_reflectionShader.SetShaderParameters(context, &m_world, &m_view, &m_projection, m_reflectTexture.Get(), m_seaTexture.Get(), m_FirstRenderPass->getShaderResourceView(), &m_reflection, XMFLOAT4(m_red, m_green, m_blue,1.0));
	//setup and draw cube
	//m_BasicShaderPair.EnableShader(context);
	//m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get());
   // m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get());
	m_Terrain.Render(context);
    //m_FloorModel.IndexRender(context);

    m_world = SimpleMath::Matrix::Identity;

    SimpleMath::Matrix rotation = SimpleMath::Matrix::CreateRotationX(m_total_seconds);
    SimpleMath::Matrix newScale1 = SimpleMath::Matrix::CreateScale(1.0f, 1.0f, 1.0f);
    SimpleMath::Matrix newPosition = SimpleMath::Matrix::CreateTranslation(14.0f, 5.0f, 4.0f);
    m_world = m_world * rotation * newScale1 *  newPosition ;
    //m_BasicShaderPair.EnableShader(context);
    //m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get());
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_moonTexture.Get());
    m_sphere = BoundingSphere(Vector3(8.0f, 5.0f, 4.0f),1.2f);
   

    //render our model
    //m_BasicModel.IndexRender(context);
    m_moonModel.IndexRender(context);


    //m_world = SimpleMath::Matrix::Identity;
    //SimpleMath::Matrix newScale2 = SimpleMath::Matrix::CreateScale(2.0f, 2.0f, 2.0f);
    //SimpleMath::Matrix newPosition1 = SimpleMath::Matrix::CreateTranslation(8.0f, 5.0f, 4.0f);
    //m_world = m_world * newScale2 * newPosition1;
    ////m_BasicShaderPair.EnableShader(context);
    ////m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get());
    //m_BasicShaderPair.EnableShader(context);
    //m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get());
    //m_BasicModel.IndexRender(context);


	//render our GUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	

 //   m_sprites->Begin();
 //   m_sprites->Draw(m_FirstRenderPass->getShaderResourceView(), m_CameraViewRect);
	//m_sprites->End();

    // Show the new frame.
    m_deviceResources->Present();
}



void Game::ReflectionRenderPass()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTargetView = m_deviceResources->GetRenderTargetView();
    auto depthTargetView = m_deviceResources->GetDepthStencilView();
    // Set the render target to be the render to texture.
    m_FirstRenderPass->setRenderTarget(context);
    // Clear the render to texture.
    m_FirstRenderPass->clearRenderTarget(context, 0.0f, 0.0f, 0.0f, 0.0f);
    //m_Camera01.RenderReflection(-2.0f);
    float rot_x = m_Camera01.getRotation().x;
    m_Camera01.RenderReflection(-0.6f, rot_x);
    m_reflection = m_Camera01.getReflectionMatrix();


    m_world = SimpleMath::Matrix::Identity;
    SimpleMath::Matrix newScale1 = SimpleMath::Matrix::CreateScale(1.0f, 1.0f, 1.0f);
    SimpleMath::Matrix newPosition = SimpleMath::Matrix::CreateTranslation(14.0f, 5.0f, 4.0f);
    //m_total_seconds -= m_timer.GetElapsedSeconds();
    SimpleMath::Matrix rotation = SimpleMath::Matrix::CreateRotationX(m_total_seconds);
    m_world = m_world * rotation * newScale1 * newPosition;
    // Turn our shaders on,  set parameters
    m_BasicShaderPair.EnableShader(context);
    //m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_reflection, &m_projection, &m_Light, m_texture1.Get());
    //m_BasicModel.IndexRender(context);

    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_reflection, &m_projection, &m_Light, m_moonTexture.Get());
    m_moonModel.IndexRender(context);

    context->OMSetRenderTargets(1, &renderTargetView, depthTargetView);
}





// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::Black);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}

#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
#ifdef DXTK_AUDIO
    m_audEngine->Suspend();
#endif
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

#ifdef DXTK_AUDIO
    m_audEngine->Resume();
#endif
}

void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    
    if (width != 800 && !is_fullScreen) {
        auto device = m_deviceResources->GetD3DDevice();
        m_FirstRenderPass = new RenderTexture(device,width, height, 1, 20);
        is_fullScreen = true;
    }if (width == 800 && is_fullScreen) {
        auto device = m_deviceResources->GetD3DDevice();
        m_FirstRenderPass = new RenderTexture(device, width, height, 1, 20);
        is_fullScreen = false;
    }
    /*else {
        m_FirstRenderPass = new RenderTexture(device, 600, 800, 1, 20);
    }*/
}

#ifdef DXTK_AUDIO
void Game::NewAudioDevice()
{
    if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
}
#endif

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    width = 800;
    height = 600;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto device = m_deviceResources->GetD3DDevice();

    m_states = std::make_unique<CommonStates>(device);
    m_fxFactory = std::make_unique<EffectFactory>(device);
    m_sprites = std::make_unique<SpriteBatch>(context);
    m_font = std::make_unique<SpriteFont>(device, L"SegoeUI_18.spritefont");
	m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);

	//setup our terrain
	m_Terrain.Initialize(device, 128, 128);
    
	//setup our test model
	m_BasicModel.InitializeSphere(device);
	//m_BasicModel2.InitializeModel(device,"drone.obj");
	//m_BasicModel3.InitializeBox(device, 10.0f, 0.1f, 10.0f);	//box includes dimensions

	//load and set up our Vertex and Pixel Shaders
    m_FloorModel.InitializeBox(device, 8.0f, 0.2f, 8.0f);	//box includes dimensions
	m_BasicShaderPair.InitStandard(device, L"light_vs.cso", L"light_ps.cso");
    m_reflectionShader.InitializeShader(device, L"reflection_vs.cso", L"reflection_ps.cso");

    m_moonModel.InitializeModel(device, "./material/Moon.obj");
    CreateDDSTextureFromFile(device, L"./material/moon.dds", nullptr, m_moonTexture.ReleaseAndGetAddressOf());

	//load Textures
	CreateDDSTextureFromFile(device, L"./material/seafloor.dds",		nullptr,	m_texture1.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"./material/EvilDrone_Diff.dds", nullptr,	m_texture2.ReleaseAndGetAddressOf());
    CreateDDSTextureFromFile(device, L"./material/ice.dds", nullptr, m_reflectTexture.ReleaseAndGetAddressOf());
    CreateDDSTextureFromFile(device, L"./material/sea.dds", nullptr, m_seaTexture.ReleaseAndGetAddressOf());

	//Initialise Render to texture
    float width = m_fullscreenRect.right - m_fullscreenRect.left;
    float height = m_fullscreenRect.bottom - m_fullscreenRect.top;
    m_FirstRenderPass = new RenderTexture(device,800 ,600 , 1, 20);	//for our rendering, We dont use the last two properties. but.  they cant be zero and they cant be the same. 
    //BoundingBox box;
   // BoundingBox::CreateFromPoints(box,);

}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
    float aspectRatio = float(size.right) / float(size.bottom);
    float fovAngleY = 70.0f * XM_PI / 180.0f;

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 2.0f;
    }

    // This sample makes use of a right-handed coordinate system using row-major matrices.
    m_projection = Matrix::CreatePerspectiveFieldOfView(
        fovAngleY,
        aspectRatio,
        0.01f,
        100.0f
    );
}

void Game::SetupGUI()
{

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Cell Automata Parameters");
        ImGui::Text("Island Generation Parameter:");
        ImGui::SliderInt("DeathLimit", m_Terrain.GetDeathLimit(), 0, 8);
        ImGui::SliderInt("BirthLimit", m_Terrain.GetBirthLimit(), 0, 8);
        ImGui::SliderInt("AliveProb", m_Terrain.GetAliveProb(), 0, 100);
        ImGui::Text("Water Color Filter:");
        ImGui::SliderFloat("R", &m_red, 0.0f, 1.0f);
        ImGui::SliderFloat("G", &m_green, 0.0f, 1.0f);
        ImGui::SliderFloat("B", &m_blue, 0.0f, 1.0f);
        ImGui::Text("Press WS to move forward and back");
        ImGui::Text("Press ADZX to rotate camera");
        ImGui::Text("Press R to initialize cell");
        ImGui::Text("Press C to run island generation");
        ImGui::Text("Press V to run Conway's Game of Life");

        std::string x = std::to_string(m_gameInputCommands.mouseX);
        std::string y = std::to_string(m_gameInputCommands.mouseY);
 /*       ImGui::Text("Mouse Position:");

        ImGui::Text(x.c_str());
        ImGui::Text(y.c_str());*/

	ImGui::End();
}


void Game::OnDeviceLost()
{
    m_states.reset();
    m_fxFactory.reset();
    m_sprites.reset();
    m_font.reset();
	m_batch.reset();
	m_testmodel.reset();
    m_batchInputLayout.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion
