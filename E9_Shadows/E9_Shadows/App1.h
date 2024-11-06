// Application.h
#ifndef _APP1_H
#define _APP1_H

// Includes
#include "DXF.h"	// include dxframework
#include "TextureShader.h"
#include "ShadowShader.h"
#include "DepthShader.h"

class App1 : public BaseApplication
{
public:

	App1();
	~App1();
	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN);

	bool frame();

protected:
	bool render();
	void depthPass();
	void finalPass();
	void gui();

private:
	void renderShadowMapToScreen(ShadowMap* shadowMap, OrthoMesh* orthoMesh);

private:
	TextureShader* textureShader;
	PlaneMesh* mesh;
	BaseMesh* mesh2;
	BaseMesh* mesh3;
	std::vector<OrthoMesh*> orthoMeshes;

	std::vector<Light*> lights;
	std::vector<XMFLOAT3> lightDirections;

	AModel* model;
	ShadowShader* shadowShader;
	DepthShader* depthShader;

	ShadowMap* shadowMaps[2];

	XMFLOAT3 modelRotation;
	XMFLOAT3 cubePosition;
	XMFLOAT3 spherePosition;
};

#endif