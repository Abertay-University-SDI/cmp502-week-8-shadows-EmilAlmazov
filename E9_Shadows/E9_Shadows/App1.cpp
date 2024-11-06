// Lab1.cpp
// Lab 1 example, simple coloured triangle mesh
#include "App1.h"

App1::App1()
{
	mesh = nullptr;
	textureShader = nullptr;
	light = nullptr;
	depthShader = nullptr;
	shadowShader = nullptr;
	shadowMap = nullptr;
	model = nullptr;
	lightMesh = nullptr;
}

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN)
{
	// Call super/parent init function (required!)
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

	// Create Mesh object and shader object
	mesh = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext());
	mesh2 = new CubeMesh(renderer->getDevice(), renderer->getDeviceContext());
	mesh3 = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext());
	model = new AModel(renderer->getDevice(), "res/teapot.obj");
	textureMgr->loadTexture(L"brick", L"res/brick1.dds");
	textureMgr->loadTexture(L"wood", L"res/wood.png");
	textureMgr->loadTexture(L"default_diffuse", L"res/DefaultDiffuse.png");
	textureMgr->loadTexture(L"check", L"res/checkerboard.png");

	// initial shaders
	textureShader = new TextureShader(renderer->getDevice(), hwnd);
	depthShader = new DepthShader(renderer->getDevice(), hwnd);
	shadowShader = new ShadowShader(renderer->getDevice(), hwnd);

	// Variables for defining shadow map
	int shadowmapWidth = 1024;
	int shadowmapHeight = 1024;
	shadowmapWidth = shadowmapHeight *= 7.0;

	int sceneWidth = 100;
	int sceneHeight = sceneWidth;

	// This is your shadow map
	shadowMap = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);

	// Configure directional light
	light = new Light();
	light->setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
	light->setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
	light->setDirection(0.0f, -0.7f, 0.7f);
	light->setPosition(0.f, 0.f, -10.f);
	light->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 100.f);
	//light->generateProjectionMatrix((float)sceneWidth, (float)sceneHeight);

	lightPosition = light->getPosition();
	lightDirection = light->getDirection();

	lightMesh = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext(), sceneWidth);

	modelRotation = XMFLOAT3(0.f, 0.f, 0.f);
	cubePosition = XMFLOAT3(-5.5f, 1.f, 1.2f);
	spherePosition = XMFLOAT3(5.5f, 1.f, 1.2f);
}

App1::~App1()
{
	// Run base application deconstructor
	BaseApplication::~BaseApplication();

	// Release the Direct3D object.

}


bool App1::frame()
{
	bool result;

	result = BaseApplication::frame();
	if (!result)
	{
		return false;
	}

	// Render the graphics.
	result = render();
	if (!result)
	{
		return false;
	}

	return true;
}

bool App1::render()
{

	// Perform depth pass
	depthPass();
	// Render scene
	finalPass();

	return true;
}

void App1::depthPass()
{
	// Set the render target to be the render to texture.
	shadowMap->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());

	// get the world, view, and projection matrices from the camera and d3d objects.
	light->generateViewMatrix();
	XMMATRIX lightViewMatrix = light->getViewMatrix();
	XMMATRIX lightProjectionMatrix = light->getOrthoMatrix();
	//XMMATRIX lightProjectionMatrix = light->getProjectionMatrix();
	XMMATRIX worldMatrix = renderer->getWorldMatrix();


	// HELPER LAMBDA FUNCTION
	auto renderMesh = [&](BaseMesh* mesh, XMMATRIX worldMatrix, DepthShader* shader)
		{
			mesh->sendData(renderer->getDeviceContext());
			shader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
			shader->render(renderer->getDeviceContext(), mesh->getIndexCount());
		};

	// Render floor
	worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
	renderMesh(mesh, worldMatrix, depthShader);

	// Render MODEL
	XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	worldMatrix = XMMatrixTranslation(0.f, 7.f, 5.f);
	worldMatrix = XMMatrixMultiply(worldMatrix, scaleMatrix);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixRotationRollPitchYaw(modelRotation.x, modelRotation.y, modelRotation.z));
	renderMesh(model, worldMatrix, depthShader);

	// Render CUBE to the left of the model
	worldMatrix = XMMatrixTranslation(cubePosition.x, cubePosition.y, cubePosition.z);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixScaling(2.f, 2.f, 2.f));
	renderMesh(mesh2, worldMatrix, depthShader);

	// Render SPHERE to the right of the model
	worldMatrix = XMMatrixTranslation(spherePosition.x, spherePosition.y, spherePosition.z);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixScaling(2.f, 2.f, 2.f));
	renderMesh(mesh3, worldMatrix, depthShader);

	// Set back buffer as render target and reset view port.
	renderer->setBackBufferRenderTarget();
	renderer->resetViewport();
}

void App1::finalPass()
{
	// Clear the scene. (default blue colour)
	renderer->beginScene(0.39f, 0.58f, 0.92f, 1.0f);
	camera->update();

	// get the world, view, projection, and ortho matrices from the camera and Direct3D objects.
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX viewMatrix = camera->getViewMatrix();
	XMMATRIX projectionMatrix = renderer->getProjectionMatrix();

	auto renderMesh = [&](BaseMesh* mesh, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ShadowShader* shader, ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* shadowMap, Light* light)
		{
			mesh->sendData(renderer->getDeviceContext());
			shader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, texture, shadowMap, light);
			shader->render(renderer->getDeviceContext(), mesh->getIndexCount());
		};

	// Render floor
	worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
	renderMesh(mesh, worldMatrix, viewMatrix, projectionMatrix, shadowShader, textureMgr->getTexture(L"wood"), shadowMap->getDepthMapSRV(), light);

	// Render MODEL
	XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	worldMatrix = XMMatrixTranslation(0.f, 7.f, 5.f);
	worldMatrix = XMMatrixMultiply(worldMatrix, scaleMatrix);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixRotationRollPitchYaw(modelRotation.x, modelRotation.y, modelRotation.z));
	renderMesh(model, worldMatrix, viewMatrix, projectionMatrix, shadowShader, textureMgr->getTexture(L"default"), shadowMap->getDepthMapSRV(), light);


	// Render CUBE to the left of the model
	worldMatrix = XMMatrixTranslation(cubePosition.x, cubePosition.y, cubePosition.z);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixScaling(2.f, 2.f, 2.f));
	renderMesh(mesh2, worldMatrix, viewMatrix, projectionMatrix, shadowShader, textureMgr->getTexture(L"default"), shadowMap->getDepthMapSRV(), light);

	// Render SPHERE to the right of the model
	worldMatrix = XMMatrixTranslation(spherePosition.x, spherePosition.y, spherePosition.z);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixScaling(2.f, 2.f, 2.f));
	renderMesh(mesh3, worldMatrix, viewMatrix, projectionMatrix, shadowShader, textureMgr->getTexture(L"default"), shadowMap->getDepthMapSRV(), light);

#pragma region RENDER SHADOW ORTHO VOLUME NEAR AND FAR PLANES
	// Render the near and far planes of the light's orthographic projection clipping volume (for shadows only)
	//XMMATRIX lightViewMatrix = light->getViewMatrix();
	//XMMATRIX lightProjectionMatrix = light->getOrthoMatrix();
	//XMVECTOR lightDirection = XMLoadFloat3(&light->getDirection());

	//// Near plane
	//XMMATRIX nearPlaneMatrix = XMMatrixTranslationFromVector(lightDirection * 0.1f);
	//nearPlaneMatrix = XMMatrixMultiply(nearPlaneMatrix, lightViewMatrix);
	//renderMesh(mesh, nearPlaneMatrix, viewMatrix, projectionMatrix, shadowShader, textureMgr->getTexture(L"default"), nullptr, light);

	//// Far plane
	//XMMATRIX farPlaneMatrix = XMMatrixTranslationFromVector(lightDirection * 100.0f);
	//farPlaneMatrix = XMMatrixMultiply(farPlaneMatrix, lightViewMatrix);
	//renderMesh(mesh, farPlaneMatrix, viewMatrix, projectionMatrix, shadowShader, textureMgr->getTexture(L"wood"), nullptr, light);
#pragma endregion

	// DEBUGGING - Render the shadow map to the screen
	renderShadowMapToScreen();

	gui();
	renderer->endScene();
}



void App1::gui()
{
	// Force turn off unnecessary shader stages.
	renderer->getDeviceContext()->GSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->HSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->DSSetShader(NULL, NULL, 0);

	// Build UI
	ImGui::Text("FPS: %.2f", timer->getFPS());
	ImGui::Checkbox("Wireframe mode", &wireframeToggle);

	// MODEL controls
	ImGui::Text("Model Controls");
	ImGui::DragFloat3("Model Rotation", reinterpret_cast<float*>(&modelRotation), 0.01f, -XM_2PI, XM_2PI);

	// CUBE controls
	ImGui::Text("Cube Controls");
	ImGui::DragFloat3("Cube Position", reinterpret_cast<float*>(&cubePosition), 0.1f, -50.f, 50.f);

	// SPHERE controls
	ImGui::Text("Sphere Controls");
	ImGui::DragFloat3("Sphere Position", reinterpret_cast<float*>(&spherePosition), 0.1f, -50.f, 50.f);

	// Light controls
	ImGui::Text("Light Controls");
	ImGui::DragFloat3("Light Position", reinterpret_cast<float*>(&lightPosition), 0.1f, -50.f, 50.f);
	light->setPosition(lightPosition.x, lightPosition.y, lightPosition.z);
	ImGui::DragFloat3("Light Direction", reinterpret_cast<float*>(&lightDirection), 0.01f, -1.f, 1.f);
	light->setDirection(lightDirection.x, lightDirection.y, lightDirection.z);

	// Render UI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}



void App1::renderShadowMapToScreen()
{

}
