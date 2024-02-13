#include "vrApp.h"

using namespace glm;

void ThreadSleep(unsigned long nMilliseconds)
{
#if defined(_WIN32)
  ::Sleep(nMilliseconds);
#elif defined(POSIX)
  usleep(nMilliseconds * 1000);
#endif
}

bool openvr::init() { 
	glfwInit();
    width = 640; height = 320;
    


	window = glfwCreateWindow(width, height, "Opengl", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	GLenum nGlewError = glewInit();



	if (nGlewError != GLEW_OK)
	{
		printf("%s - Error initializing GLEW! %s\n", __FUNCTION__, glewGetErrorString(nGlewError));
		return false;
	}
	glGetError(); // to clear the error caused deep in GLEW

	textureLoader::Init();

	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
	if (eError != vr::VRInitError_None)
	{
		m_pHMD = NULL;
		char buf[1024];
		sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		return false;
	}

    m_rTrackingUniverse = vr::ETrackingUniverseOrigin::TrackingUniverseStanding;
    vr::VRCompositor()->SetTrackingSpace(m_rTrackingUniverse);

	m_fNearClip = 0.01f;
	m_fFarClip = 30.0f;

    m_model = Matrix4();
	//initGL
	createShader();
	setupScene();
	setupCameras();
	setupStereoRenderTargets();

    if (!setupFrontCameras())
    {
        printf("Error during front cameras setup");
        return false;
    }
    else
    {
        printf("Front cameras initialized correctly");
    }

	//init Compositor
	vr::EVRInitError peError = vr::VRInitError_None;

	if (!vr::VRCompositor())
	{
		printf("Compositor initialization failed. See log file for details\n");
		return false;
	}

	return true;
}

void openvr::createShader() {
    Shader lshader_quad("res/shaders/fbovertex.vert", "res/shaders/fbofragment.frag");
    Shader lshader_square("res/shaders/quad.vert", "res/shaders/quad.frag");
	
    shader_quad = lshader_quad.ID;
    shader_square = lshader_square.ID; //TFG

    m_nQuadCameraMatrixLocation = glGetUniformLocation(shader_square, "matrix");
}

void openvr::setupQuadCamera()
{

    while (glGetError() != GL_NO_ERROR) {

    }

    float vertices[] = {
         0.3f,  0.3f, 0.0f,  // top right
         0.3f, -0.3f, 0.0f,  // bottom right
        -0.3f, -0.3f, 0.0f,  // bottom left
        -0.3f,  0.3f, 0.0f    // top left
    };

    for (int i = 0; i < 12; i++) vertices[i] *= 1.0f;

    texture_test = textureLoader::loadImage("res/textures/img_test.png");

    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &vao_square);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(vao_square);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLint posCamAttrib = glGetAttribLocation(shader_square, "position");

    glVertexAttribPointer(posCamAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(posCamAttrib);

    //texcoords
    GLint texAttrib = glGetAttribLocation(shader_square, "texcoord");
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GL_FLOAT), (void*)(sizeof(GL_FLOAT) * 6));
    glEnableVertexAttribArray(texAttrib);

    //sampler uniform
    GLint texpos = glGetAttribLocation(shader_square, "tex");
    glUniform1i(texpos, 0);


    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void openvr::setupScene() {
  //companion window

  glUseProgram(shader_quad);
  GLfloat screenQuadVertices[] =
  {//	x		y	r	g	b
    -1.0, -1.0, 1.0, 0.0, 0.0, 0.0, 0.0,
    1.0, -1.0, 1.0, 1.0, 1.0, 1.0, 0.0,
    1.0,  1.0, 0.0, 1.0, 1.0, 1.0, 1.0,
    -1.0,  1.0, 1.0, 1.0, 1.0, 0.0, 1.0
  };

  glGenVertexArrays(1, &vao_quad);
  glBindVertexArray(vao_quad);


  //vertex Attributes
  GLuint vbo2;
  glGenBuffers(1, &vbo2);
  glBindBuffer(GL_ARRAY_BUFFER, vbo2);
  glBufferData(GL_ARRAY_BUFFER, sizeof(screenQuadVertices), screenQuadVertices, GL_DYNAMIC_DRAW);

  //load layout location of position
  GLint posAttrib2 = glGetAttribLocation(shader_quad, "position");
  glVertexAttribPointer(posAttrib2, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(GL_FLOAT), 0);
  glEnableVertexAttribArray(posAttrib2);

  //color
  GLint colAttrib2 = glGetAttribLocation(shader_quad, "color");
  glVertexAttribPointer(colAttrib2, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GL_FLOAT), (void*)(sizeof(GL_FLOAT) * 2));
  glEnableVertexAttribArray(colAttrib2);

  //texcoords
  GLint texAttrib2 = glGetAttribLocation(shader_quad, "texcoord");
  glVertexAttribPointer(texAttrib2, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(GL_FLOAT), (void*)(sizeof(GL_FLOAT) * 5));
  glEnableVertexAttribArray(texAttrib2);

  //sampler uniform
  GLint texpos2 = glGetAttribLocation(shader_quad, "tex");
  glUniform1i(texpos2, 0);

  glBindVertexArray(0);

  setupQuadCamera();
}

void openvr::setupCameras() { 

	m_mat4ProjectionLeft = GetHMDMatrixProjectionEye(vr::Eye_Left);
	m_mat4ProjectionRight = GetHMDMatrixProjectionEye(vr::Eye_Right);

	m_mat4eyePosLeft = GetHMDMatrixPoseEye(vr::Eye_Left);
	m_mat4eyePosRight = GetHMDMatrixPoseEye(vr::Eye_Right);

    m_mat4HMDPose = Matrix4(); //Init with identity

}

bool openvr::setupFrontCameras()
{
    m_CameraFrameType = vr::VRTrackedCameraFrameType_Undistorted;
    
    bool bHasCamera = false;
    vr::EVRTrackedCameraError nCameraError = vr::VRTrackedCamera()->HasCamera(vr::k_unTrackedDeviceIndex_Hmd, &bHasCamera);

    if (nCameraError != vr::VRTrackedCameraError_None)
    {
        printf("Tracked Camera Error! (%s)\n", vr::VRTrackedCamera()->GetCameraErrorNameFromEnum(nCameraError));
        return false;
    }

    if (!bHasCamera)
    {
        printf("No Tracked Camera Available!\n");
        return false;
    }

    vr::ETrackedPropertyError propertyError;
    char buffer[128];
    vr::VRSystem()->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_CameraFirmwareDescription_String, buffer, sizeof(buffer), &propertyError);

    uint32_t nNewBufferSize = 0;
    vr::VRTrackedCamera()->GetCameraFrameSize(vr::k_unTrackedDeviceIndex_Hmd, m_CameraFrameType, &m_nCameraFrameWidth, &m_nCameraFrameHeight, &nNewBufferSize) != vr::VRTrackedCameraError_None;

    if (nNewBufferSize && nNewBufferSize != m_nCameraFrameBufferSize)
    {
        delete[] m_pCameraFrameBuffer;
        m_nCameraFrameBufferSize = nNewBufferSize;
        m_pCameraFrameBuffer = new uint8_t[m_nCameraFrameBufferSize];
        memset(m_pCameraFrameBuffer, 0, m_nCameraFrameBufferSize);
    }

    vr::VRTrackedCamera()->AcquireVideoStreamingService(vr::k_unTrackedDeviceIndex_Hmd, &m_hTrackedCamera);

    m_mat4TrackedCameraProjectionLeft = GetTrackedCameraMatrixProjection(vr::Eye_Left);
    m_mat4TrackedCameraProjectionRight = GetTrackedCameraMatrixProjection(vr::Eye_Right);
    m_mat4TrackedCameraEyePosLeft = GetTrackedCameraMatrixPoseEye(vr::Eye_Left);
    m_mat4TrackedCameraEyePosRight = GetTrackedCameraMatrixPoseEye(vr::Eye_Right);

    return true;

}

void openvr::updateCameraFrameBuffer()
{
    vr::EVRTrackedCameraError error;
    vr::CameraVideoStreamFrameHeader_t header;

    error = vr::VRTrackedCamera()->GetVideoStreamFrameBuffer(m_hTrackedCamera, m_CameraFrameType, m_pCameraFrameBuffer, m_nCameraFrameBufferSize, &header, sizeof(header));

    if (error == vr::EVRTrackedCameraError::VRTrackedCameraError_None)
    {
        memcpy(&m_CurrentFrameHeader, &header, sizeof(header));
        //printf("Camera image obtained correctly");
    }
}

Matrix4 openvr::GetTrackedCameraMatrixPoseEye(vr::Hmd_Eye nEye)
{
    vr::HmdMatrix34_t matrix[2];
    vr::TrackedPropertyError error;

    vr::VRSystem()->GetArrayTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_CameraToHeadTransforms_Matrix34_Array, vr::k_unHmdMatrix34PropertyTag, &matrix, sizeof(matrix), &error);

    uint32_t cameraIndex = nEye == vr::Hmd_Eye::Eye_Left ? 0 : 1;
    vr::HmdMatrix34_t matCamera = matrix[cameraIndex];

    Matrix4 matrixObj(
        matCamera.m[0][0], matCamera.m[1][0], matCamera.m[2][0], 0.0,
        matCamera.m[0][1], matCamera.m[1][1], matCamera.m[2][1], 0.0,
        matCamera.m[0][2], matCamera.m[1][2], matCamera.m[2][2], 0.0,
        matCamera.m[0][3], matCamera.m[1][3], matCamera.m[2][3], 1.0f
    );

    return matrixObj;
}

Matrix4 openvr::GetTrackedCameraMatrixProjection(vr::Hmd_Eye nEye)
{
    m_fTrackedCameraProjectionDistance = 5.0;
    uint32_t cameraIndex = nEye == vr::Hmd_Eye::Eye_Left ? 0 : 1;

    vr::HmdMatrix44_t mat;

    vr::EVRTrackedCameraError error1 = vr::VRTrackedCamera()->GetCameraProjection(vr::k_unTrackedDeviceIndex_Hmd, cameraIndex, m_CameraFrameType, m_fNearClip, m_fTrackedCameraProjectionDistance, &mat);

    if (error1 != vr::EVRTrackedCameraError::VRTrackedCameraError_None)
    {
        printf("Unable to get camera projection matrix: %s\n", vr::VRTrackedCamera()->GetCameraErrorNameFromEnum(error1));
    }

    Matrix4 matrixObj(
        mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
        mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
        mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
        mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
    );

    matrixObj.invert();


    return matrixObj;
    
}

Matrix4 openvr::GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye) {

	vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, m_fNearClip, m_fFarClip);


    Matrix4 mat4OpenVR = Matrix4(mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
                                 mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
                                 mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
                                 mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]);

    return mat4OpenVR;
}

Matrix4 openvr::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{

	vr::HmdMatrix34_t matEye = m_pHMD->GetEyeToHeadTransform(nEye);

    Matrix4 mat4OpenVR = Matrix4(matEye.m[0][0], matEye.m[1][0], matEye.m[2][0], 0.0,
                                 matEye.m[0][1], matEye.m[1][1], matEye.m[2][1], 0.0,
                                 matEye.m[0][2], matEye.m[1][2], matEye.m[2][2], 0.0,
                                 matEye.m[0][3], matEye.m[1][3], matEye.m[2][3], 1.0f);

    return mat4OpenVR.invert();
}

Matrix4 openvr::GetTrackedCameraToTrackingSpaceMatrix(vr::Hmd_Eye nEye)
{
    vr::HmdMatrix34_t mat = m_CurrentFrameHeader.trackedDevicePose.mDeviceToAbsoluteTracking;

    Matrix4 LeftCameraToTracking(
        mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0f,
        mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0f,
        mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0f,
        mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
    );

    Matrix4 trackedCameraMatrix;

    if (nEye == vr::Eye_Left)
    {
        m_mat4TrackedCameraProjectionLeft = GetTrackedCameraMatrixProjection(nEye);

        trackedCameraMatrix = LeftCameraToTracking * m_mat4TrackedCameraProjectionLeft;
    }
    else if (nEye == vr::Eye_Right)
    {
        // Get the right camera relative to the left
        Matrix4 leftCameraPos(m_mat4TrackedCameraEyePosLeft);
        leftCameraPos.invert();
        Matrix4 cameraOffset(leftCameraPos * m_mat4TrackedCameraEyePosRight);


        m_mat4TrackedCameraProjectionRight = GetTrackedCameraMatrixProjection(nEye);

        trackedCameraMatrix = LeftCameraToTracking * cameraOffset * m_mat4TrackedCameraProjectionRight;
    }

    return trackedCameraMatrix;
}

void openvr::setupStereoRenderTargets() {

	m_pHMD->GetRecommendedRenderTargetSize(&m_nRenderWidth, &m_nRenderHeight);

	createFrameBuffer(m_nRenderWidth, m_nRenderHeight, leftEyeDesc);
	createFrameBuffer(m_nRenderWidth, m_nRenderHeight, rightEyeDesc);

}

void openvr::createFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc) {

	glGenFramebuffers(1, &framebufferDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId);

	
	glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
	glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, nWidth, nHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);

	glGenTextures(1, &framebufferDesc.m_nRenderTextureId);
	glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nRenderTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nRenderTextureId, 0);

	glBindTexture(GL_TEXTURE_2D, 0);


	// check FBO status
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer creation successful." << std::endl;
	}
	else {
		std::cout << "Framebuffer creation FAILED." << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


}

Matrix4 openvr::getCurrentViewProjectionMatrix(vr::Hmd_Eye nEye)
{
  
    Matrix4 matMVP = Matrix4();

    if (nEye == vr::Eye_Left)
    {
        matMVP = m_mat4ProjectionLeft * m_mat4eyePosLeft;

    }
    else if (nEye == vr::Eye_Right)
    {
        matMVP = m_mat4ProjectionRight * m_mat4eyePosRight;
    }

    return matMVP;
}

void openvr::renderScene(vr::Hmd_Eye nEye) {

    while (glGetError() != GL_NO_ERROR){}

    glClearColor(0.3f, 0.2f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_square);
    Matrix4 currentViewProjectionMatrix = getCurrentViewProjectionMatrix(nEye);

    glUniformMatrix4fv(m_nQuadCameraMatrixLocation, 1, GL_FALSE, currentViewProjectionMatrix.get());
    glBindVertexArray(vao_square);
    glBindTexture(GL_TEXTURE_2D, texture_test);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  
}

void openvr::renderStereoTargets() {

  glBindFramebuffer(GL_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
  glBindTexture(GL_TEXTURE_2D, leftEyeDesc.m_nRenderTextureId);
  glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
  renderScene(vr::Eye_Left);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId);
  glBindTexture(GL_TEXTURE_2D, rightEyeDesc.m_nRenderTextureId);
  glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
  renderScene(vr::Eye_Right);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

Matrix4 openvr::ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t& matPose)
{
    Matrix4 matrixObj(
        matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
        matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
        matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
        matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
    );
    return matrixObj;
}

void openvr::updateHMDMatrixPose() {

  vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);
  
  m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd] = ConvertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);
     
  if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
  {
    m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd];
    m_mat4HMDPose = m_mat4HMDPose.invert();
  }

}

void openvr::drawScreenQuad() {

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, width/2, height);

    glBindTexture(GL_TEXTURE_2D, rightEyeDesc.m_nRenderTextureId);
    glUseProgram(shader_quad);
    glBindVertexArray(vao_quad);
    glDrawArrays(GL_POLYGON, 0, 4);

    glViewport(width/2, 0, width, height);

    glBindTexture(GL_TEXTURE_2D, leftEyeDesc.m_nRenderTextureId);
    glUseProgram(shader_quad);
    glBindVertexArray(vao_quad);
    glDrawArrays(GL_POLYGON, 0, 4);

    glBindVertexArray(0);

}

std::string openvr::GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError)
{
  uint32_t unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
  if (unRequiredBufferLen == 0)
    return "";

  char *pchBuffer = new char[unRequiredBufferLen];
  unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
  std::string sResult = pchBuffer;
  delete[] pchBuffer;
  return sResult;
}

void openvr::runMainLoop(){

    vr::EVRCompositorError submitError;
	while (!glfwWindowShouldClose(window)) {
		
        updateCameraFrameBuffer();

        updateHMDMatrixPose();

	    renderStereoTargets();

        drawScreenQuad();

		vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)leftEyeDesc.m_nRenderTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
        submitError = vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
        if (submitError != vr::VRCompositorError_None) {
            printf("Error when submitting: %d ", submitError);
        }

		vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)rightEyeDesc.m_nRenderTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
        submitError = vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
        if (submitError != vr::VRCompositorError_None) {
            printf("Error when submitting: %d ", submitError);
        }


		glfwSwapBuffers(window);
		glfwPollEvents();

    
	}


}

int main() {

	openvr *op = new openvr();

	op->init();
	
	op->runMainLoop();

    return 0;
}
