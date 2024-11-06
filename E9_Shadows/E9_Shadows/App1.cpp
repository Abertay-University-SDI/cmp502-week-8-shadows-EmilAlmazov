// Lab1.cpp
// Lab 1 example, simple coloured triangle mesh
#include "App1.h"

App1::App1()
{
	mesh = nullptr;
	textureShader = nullptr;
	depthShader = nullptr;
	shadowShader = nullptr;
	model = nullptr;
}

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN)
{
	// Call super/parent init function (required!)
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

	// Create Mesh object and shader object
	mesh = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext());
	mesh2 = new CubeMesh(renderer->getDevice(), renderer->getDeviceContext());
	mesh3 = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext());
	float orthoWidth = screenWidth / 4;
	float orthoHeight = screenHeight / 4;
	OrthoMesh* orthoMesh = new OrthoMesh(renderer->getDevice(), renderer->getDeviceContext(), orthoWidth, orthoHeight, -(screenWidth / 2 - orthoWidth / 2), screenHeight / 2 - orthoHeight / 2);
	orthoMeshes.push_back(orthoMesh);
	orthoMesh = new OrthoMesh(renderer->getDevice(), renderer->getDeviceContext(), orthoWidth, orthoHeight, +(screenWidth / 2 - orthoWidth / 2), screenHeight / 2 - orthoHeight / 2);
	orthoMeshes.push_back(orthoMesh);
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
	shadowMaps[0] = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);
	shadowMaps[1] = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);

	// Configure directional light
	Light* light = new Light();
	light->setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f); // Dark light
	light->setDiffuseColour(1.0f, 0.0f, 0.0f, 1.0f); // Red light
	light->setDirection(0.f, -1.f, 0.f); // Pointing down and to the right
	light->setPosition(0.f, 10.f, 0.f);
	light->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 100.f);
	lights.push_back(light);
	lightDirections.push_back(light->getDirection());

	// Configure 2nd directional light
	Light* light2 = new Light();
	light2->setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
	light2->setDiffuseColour(0.0f, 1.0f, 0.0f, 1.0f); // Green light
	light2->setDirection(0.f, -1.f, 0.f); // Pointing down and to the left
	light2->setPosition(0.f, 10.f, 0.f);
	light2->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 100.f);
	lights.push_back(light2);
	lightDirections.push_back(light2->getDirection());


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
	// HELPER LAMBDA FUNCTION
	auto renderMesh = [&](BaseMesh* mesh, DepthShader* shader, XMMATRIX worldMatrix, XMMATRIX lightViewMatrix, XMMATRIX lightProjectionMatrix)
		{
			mesh->sendData(renderer->getDeviceContext());
			shader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
			shader->render(renderer->getDeviceContext(), mesh->getIndexCount());
		};

	for (int i = 0; i < lights.size(); ++i)
	{
		// Set the render target to be the render to texture.
		shadowMaps[i]->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());

		// get the world, view, and projection matrices from the camera and d3d objects.
		lights[i]->generateViewMatrix();
		XMMATRIX lightViewMatrix = lights[i]->getViewMatrix();
		XMMATRIX lightProjectionMatrix = lights[i]->getOrthoMatrix();
		XMMATRIX worldMatrix = renderer->getWorldMatrix();

		// Render floor
		worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
		renderMesh(mesh, depthShader, worldMatrix, lightViewMatrix, lightProjectionMatrix);

		// Render MODEL
		XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
		worldMatrix = XMMatrixTranslation(0.f, 7.f, 5.f);
		worldMatrix = XMMatrixMultiply(worldMatrix, scaleMatrix);
		worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixRotationRollPitchYaw(modelRotation.x, modelRotation.y, modelRotation.z));
		renderMesh(model, depthShader, worldMatrix, lightViewMatrix, lightProjectionMatrix);

		// Render CUBE to the left of the model
		worldMatrix = XMMatrixTranslation(cubePosition.x, cubePosition.y, cubePosition.z);
		worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixScaling(2.f, 2.f, 2.f));
		renderMesh(mesh2, depthShader, worldMatrix, lightViewMatrix, lightProjectionMatrix);

		// Render SPHERE to the right of the model
		worldMatrix = XMMatrixTranslation(spherePosition.x, spherePosition.y, spherePosition.z);
		worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixScaling(2.f, 2.f, 2.f));
		renderMesh(mesh3, depthShader, worldMatrix, lightViewMatrix, lightProjectionMatrix);

		// Set back buffer as render target and reset view port.
		renderer->setBackBufferRenderTarget();
		renderer->resetViewport();
	}
}

void App1::finalPass()
{
	// Clear the scene. (default blue colour)
	renderer->beginScene(0.39f, 0.58f, 0.92f, 1.0f);
	camera->update();

	ID3D11ShaderResourceView* depthMaps[2] = {
		shadowMaps[0]->getDepthMapSRV(),
		shadowMaps[1]->getDepthMapSRV()
	};

	auto renderMesh = [&](BaseMesh* mesh, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ShadowShader* shader, ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView** shadowMaps, std::vector<Light*> lights)
		{
			mesh->sendData(renderer->getDeviceContext());
			shader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, texture, shadowMaps, lights);
			shader->render(renderer->getDeviceContext(), mesh->getIndexCount());
		};


	// get the world, view, projection, and ortho matrices from the camera and Direct3D objects.
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX viewMatrix = camera->getViewMatrix();
	XMMATRIX projectionMatrix = renderer->getProjectionMatrix();
	
	// Render floor
	worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
	renderMesh(mesh, worldMatrix, viewMatrix, projectionMatrix, shadowShader, textureMgr->getTexture(L"wood"), depthMaps, lights);

	// Render MODEL
	XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	worldMatrix = XMMatrixTranslation(0.f, 7.f, 5.f);
	worldMatrix = XMMatrixMultiply(worldMatrix, scaleMatrix);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixRotationRollPitchYaw(modelRotation.x, modelRotation.y, modelRotation.z));
	renderMesh(model, worldMatrix, viewMatrix, projectionMatrix, shadowShader, textureMgr->getTexture(L"default"), depthMaps, lights);

	// Render CUBE to the left of the model
	worldMatrix = XMMatrixTranslation(cubePosition.x, cubePosition.y, cubePosition.z);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixScaling(2.f, 2.f, 2.f));
	renderMesh(mesh2, worldMatrix, viewMatrix, projectionMatrix, shadowShader, textureMgr->getTexture(L"default"), depthMaps, lights);

	// Render SPHERE to the right of the model
	worldMatrix = XMMatrixTranslation(spherePosition.x, spherePosition.y, spherePosition.z);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixScaling(2.f, 2.f, 2.f));
	renderMesh(mesh3, worldMatrix, viewMatrix, projectionMatrix, shadowShader, textureMgr->getTexture(L"default"), depthMaps, lights);


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
	for (size_t i = 0; i < orthoMeshes.size(); ++i)
	{
		renderShadowMapToScreen(shadowMaps[i], orthoMeshes[i]);
	}

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
	ImGui::DragFloat3("Model Rotation", reinterpret_cast<float*>(&modelRotation), 0.01f, -XM_2PI, XM_2PI);

	//  === Light controls ===
	
	// 1st light
	XMFLOAT3& lightDirection = lightDirections[0];
	ImGui::DragFloat3("Light 1 Direction", reinterpret_cast<float*>(&lightDirection), 0.01f, -1.f, 1.f);
	lights[0]->setDirection(lightDirection.x, lightDirection.y, lightDirection.z);
	
	// 2nd light
	XMFLOAT3& lightDirection2 = lightDirections[1];
	ImGui::DragFloat3("Light 2 Direction", reinterpret_cast<float*>(&lightDirection2), 0.01f, -1.f, 1.f);
	lights[1]->setDirection(lightDirection2.x, lightDirection2.y, lightDirection2.z);


	// === Render UI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}



void App1::renderShadowMapToScreen(ShadowMap* shadowMap, OrthoMesh* orthoMesh)
{
	// RENDER THE RENDER TEXTURE SCENE
// Requires 2D rendering and an ortho mesh.
	renderer->setZBuffer(false);
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX orthoMatrix = renderer->getOrthoMatrix(); // ortho matrix for 2D rendering
	XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix(); // Default camera position for orthographic rendering

	// Render ortho mesh with texture shader set.
	orthoMesh->sendData(renderer->getDeviceContext());
	textureShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, orthoViewMatrix, orthoMatrix, shadowMap->getDepthMapSRV());
	textureShader->render(renderer->getDeviceContext(), orthoMesh->getIndexCount());
	renderer->setZBuffer(true);
}
