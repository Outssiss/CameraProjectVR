#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <openvr.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtc/matrix_inverse.hpp>
#include "textureLoader.h"
#include "shader.h"
#include "Matrices.h"


class openvr {

	GLFWwindow* window;
	int width;
	int height;
	vr::IVRSystem* m_pHMD;
	struct FramebufferDesc
	{
		GLuint m_nDepthBufferId;
		GLuint m_nRenderTextureId;
		GLuint m_nRenderFramebufferId;
	};
	FramebufferDesc leftEyeDesc;
	FramebufferDesc rightEyeDesc;

	uint32_t m_nRenderWidth;
	uint32_t m_nRenderHeight;


	Matrix4 m_model;

	Matrix4 m_mat4HMDPose;
	Matrix4 m_mat4eyePosLeft;
	Matrix4 m_mat4eyePosRight;

	Matrix4 m_mat4ProjectionCenter;
	Matrix4 m_mat4ProjectionLeft;
	Matrix4 m_mat4ProjectionRight;

	//TFG
	uint8_t* m_pCameraFrameBuffer;
	uint32_t m_nCameraFrameBufferSize;

	Matrix4 m_mat4TrackedCameraProjectionLeft;
	Matrix4 m_mat4TrackedCameraProjectionRight;
	Matrix4 m_mat4TrackedCameraEyePosLeft;
	Matrix4 m_mat4TrackedCameraEyePosRight;

	GLuint texture_test;

	vr::EVRTrackedCameraFrameType m_CameraFrameType;

	float m_fTrackedCameraProjectionDistance;
	vr::CameraVideoStreamFrameHeader_t m_CurrentFrameHeader;
	//

	//glm::mat4 m_mat4

	float m_fNearClip;
	float m_fFarClip;

	GLuint shader_scene;
	GLuint shader_quad;
	GLuint shader_square;


	GLuint vao_scene;
	GLuint vao_quad;
	unsigned int vao_square;
	GLuint texture_camera;

	//TFG
	GLuint quadCamera;
	//

	GLuint m_nSceneMatrixLocation;
	GLuint m_nSceneModelLocation;
	GLuint m_nQuadCameraMatrixLocation;


	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	Matrix4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];

	uint32_t m_nCameraFrameWidth;
	uint32_t m_nCameraFrameHeight;

	vr::TrackingUniverseOrigin m_rTrackingUniverse;

	vr::TrackedCameraHandle_t m_hTrackedCamera;

	bool drawControllerAsCube;

public:
	bool init();
	void runMainLoop();
	void createShader();
	void setupScene();
	void setupCameras();
	//TFG
	bool setupFrontCameras();
	void updateCameraFrameBuffer();
	void setupQuadCamera();
	//
	void setupStereoRenderTargets();
	void renderStereoTargets();

	void renderScene(vr::Hmd_Eye nEye);

	void drawScreenQuad();

	void updateHMDMatrixPose();

	Matrix4 GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
	Matrix4 GetHMDMatrixPoseEye(vr::Hmd_Eye nEye);

	Matrix4 getCurrentViewProjectionMatrix(vr::Hmd_Eye nEye);

	Matrix4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t& matPose);

	//TFG
	Matrix4 GetTrackedCameraMatrixProjection(vr::Hmd_Eye nEye);
	Matrix4 GetTrackedCameraMatrixPoseEye(vr::Hmd_Eye nEye);

	Matrix4 GetTrackedCameraToTrackingSpaceMatrix(vr::Hmd_Eye nEye);
	//

	void createFrameBuffer(int nWidth, int nHeight, FramebufferDesc& framebufferDesc);

	std::string GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = NULL);
};