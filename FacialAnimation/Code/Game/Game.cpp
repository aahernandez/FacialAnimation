#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/RHI/RHI.hpp"
#include "Engine/RHI/SimpleRenderer.hpp"
#include "Engine/Core/RGBA.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/RHI/ConstantBuffer.hpp"
#include "Engine/RHI/ShaderDatabase.hpp"
#include "Engine/Core/Config.hpp"
#include "Engine/RHI/VertexBuffer.hpp"
#include "Engine/RHI/Font.hpp"
#include "Engine/Core/DeveloperConsole.hpp"
#include "Engine/RHI/IndexBuffer.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/RHI/StructuredBuffer.hpp"
#include "Engine/Audio/SpeechToTextSystem.hpp"
#include "Engine/Time/Time.hpp"
#include "Engine/Core/Noise.hpp"
#include "ThirdParty/FBX/FBX.hpp"
#include "ThirdParty/PocketSphinx/pocketsphinx.h"

Game* g_theGame = nullptr;

Game::Game()
	: m_isPaused(false)
	, m_isRenderingMeshBuilder(false)
	, m_windowWidth(1280)
	, m_windowHeight(720)
	, m_fov(60.f)
	, m_diffuseTexture(nullptr)
	, m_pointSampler(nullptr)
	, m_myShader(nullptr)
	, m_shaders(nullptr)
	, m_timeConstant(nullptr)
	, m_blendShapeNumConstant(nullptr)
	, m_displayType(NONE)
	, m_currentMorphIndex(0)
	, m_timeFaceRunning(0.f)
	, m_isMovingFace(false)
	, m_faceRollAboutX(0.f)
	, m_facePitchAboutY(0.f)
	, m_faceYawAboutZ(0.f)
	, m_faceRotationLerpPercentage(0.f)
	, m_currentAmplitude(0.f)
	, m_currentEmotion(EMOTION_NORMAL)
{
	Initialize();
}

Game::~Game()
{
	Destroy();
}

void Game::Initialize()
{
	g_theConfig->ConfigGetInt(&m_windowWidth, "window_res_x");
	g_theConfig->ConfigGetInt(&m_windowHeight, "window_res_y");

	m_shaders = new ShaderDatabase();
	CreateAndStoreShaderFromConfig();
	m_myShader = m_shaders->GetUntexturedShaderInMap();

	char const *imageFilepath;
	g_theConfig->ConfigGetString(&imageFilepath, "diffuseTexture");
	m_diffuseTexture = new Texture2D(g_theSimpleRenderer->m_rhiDevice, imageFilepath);
	char const *imageFilepath2;
	g_theConfig->ConfigGetString(&imageFilepath2, "normalTexture");
	m_normalTexture = new Texture2D(g_theSimpleRenderer->m_rhiDevice, imageFilepath2);

	Image tanImage;
	tanImage.CreateClear(1, 1, RGBA((unsigned char)255, 224, 189, 255));
	m_skinTexture = new Texture2D(g_theSimpleRenderer->m_rhiDevice, tanImage);

	m_pointSampler = new Sampler(g_theSimpleRenderer->m_rhiDevice, FILTER_POINT, FILTER_POINT);

	g_theConfig->ConfigGetFloat(&m_fov, "fov");
	
	m_time.time = 0.f;
	m_timeConstant = new ConstantBuffer( g_theSimpleRenderer->m_rhiDevice, &m_time, sizeof(m_time));

	m_blendShapesNum.numBlendShapes = NUM_MORPHS_TYPES;
	m_blendShapeNumConstant = new ConstantBuffer(g_theSimpleRenderer->m_rhiDevice, &m_blendShapesNum, sizeof(m_blendShapesNum));
	g_theSimpleRenderer->SetConstantBuffer(3, m_blendShapeNumConstant);

	m_camera.ResetCameraPositionAndOrientation();

	g_theDevConsole = new DeveloperConsole();
	g_theDevConsole->SetProjectionSize(Vector2(1280.f, 720.f), Vector2( 1280.f, 720.f));
	InitDevConsoleFunctions();
	
	g_theDevConsole->m_commands["fbxSkin"]("OriginalElf");
	g_theDevConsole->m_commands["changeSkin"]("Morpher1");
	SetMorphTargetStructuredBuffer();

	g_theInput->ShowMouseCursor();
	m_isMovingFace = true;
}

void Game::InitDevConsoleFunctions()
{
	std::function<void(std::string args)> Func;
	Func = [&](std::string args)
	{
		if (args.compare("next"))
		{
			ChangeToNextShader();
		}
		else if (args.compare("prev"))
		{
			ChangeToPrevShader();
		}
	};
	g_theDevConsole->AddFunctionCommand("shader", "Switches to the next or previous shader. Must use argument \"[0.3,0.9,0.9]next[-]\" or \"[0.3,0.9,0.9]prev[-]\"", Func);

	Func = [&](std::string args)
	{
		m_currentFbxName = args;
		std::string filePath = "Data/Models/FBX/";
		filePath += args;
		filePath += ".fbx";
		FbxListFile(filePath.c_str());
	};
	g_theDevConsole->AddFunctionCommand("fbxList", "Lists out the nodes of an fbx file.", Func);

	Func = [&](std::string args)
	{
		m_currentFbxName = args;
		std::string filePath = "Data/Models/FBX/";
		filePath += args;
		filePath += ".fbx";
		FbxLoadMesh(&m_fbxMeshBuilder, m_skeleton, m_motion, m_skeletonInstance, filePath.c_str(), args.c_str(), &m_skelDatabase, &m_motionDatabase);
		VertexBuffer *vertexBuffer = g_theSimpleRenderer->m_rhiDevice->CreateVertexBuffer(m_fbxMeshBuilder.m_vertices);
		IndexBuffer *indexBuffer = g_theSimpleRenderer->m_rhiDevice->CreateIndexBuffer(m_fbxMeshBuilder.m_indices);
		fbxVBA.push_back(vertexBuffer);
		fbxIBA.push_back(indexBuffer);
		m_isRenderingMeshBuilder = true;
		m_displayType = MESH;
	};
	g_theDevConsole->AddFunctionCommand("fbxMesh", "Loads a mesh into the mesh builder.", Func);

	Func = [&](std::string args)
	{
		m_currentFbxName = args;
		m_skeleton = *m_skelDatabase.CreateOrGetSkeleton(m_currentFbxName);

		m_skeleton.GetVertexBufferVector(fbxVBA, fbxIBA, g_theSimpleRenderer);
		m_skeleton.SaveSkeleton(m_fileStream, Stringf("%s.skel", m_currentFbxName.c_str()));
		m_isRenderingMeshBuilder = true;
		m_displayType = SKELETON;
	};
	g_theDevConsole->AddFunctionCommand("fbxSkel", "Loads a skeleton and displays it.", Func);

	Func = [&](std::string args)
	{
		m_currentFbxName = args;
		std::string filePath = "Data/Models/FBX/";
		filePath += args;
		filePath += ".fbx";
		m_skeleton = *m_skelDatabase.CreateOrGetSkeleton(m_currentFbxName);
		m_motion = *m_motionDatabase.CreateOrGetMotion(m_currentFbxName, &m_skeleton);
		m_skeletonInstance.m_skeleton = &m_skeleton;
		m_motion.m_currentTime = 0.f;
		m_skeleton.SaveSkeleton(m_fileStream, Stringf("%s.skel", m_currentFbxName.c_str()));
		m_motion.SaveMotion(m_fileStream, Stringf("%s.motion", m_currentFbxName.c_str()));
		m_isRenderingMeshBuilder = true;
		m_displayType = MOTION;
	};
	g_theDevConsole->AddFunctionCommand("fbxMotion", "Loads a skeleton and displays it.", Func);

	Func = [&](std::string args)
	{
		if (!fbxVBA.empty())
		{
			delete fbxVBA[0];
			fbxVBA[0] = nullptr;
			fbxVBA.clear();

			delete fbxIBA[0];
			fbxIBA[0] = nullptr;
			fbxIBA.clear();
		}

		m_currentFbxName = args;
		m_fbxMeshBuilder = *m_meshDatabase.CreateOrGetMesh(m_currentFbxName, &m_skeleton, &m_motion, &m_skeletonInstance, &m_skelDatabase, &m_motionDatabase);
		
		m_skeleton.SaveSkeleton(m_fileStream, Stringf("%s.skel", m_currentFbxName.c_str()));
		m_motion.SaveMotion(m_fileStream, Stringf("%s.motion", m_currentFbxName.c_str()));
		m_fbxMeshBuilder.SaveMesh(m_fileStream, Stringf("%s.mesh", m_currentFbxName.c_str()));
		
		VertexBuffer *vertexBuffer = g_theSimpleRenderer->m_rhiDevice->CreateVertexBuffer(m_fbxMeshBuilder.m_vertices);
		IndexBuffer *indexBuffer = g_theSimpleRenderer->m_rhiDevice->CreateIndexBuffer(m_fbxMeshBuilder.m_indices);
		fbxVBA.push_back(vertexBuffer);
		fbxIBA.push_back(indexBuffer);
		m_isRenderingMeshBuilder = true;
		m_displayType = SKINNING;
	};
	g_theDevConsole->AddFunctionCommand("fbxSkin", "Loads a mesh, skeleton, and motion, and displays them.", Func);

	Func = [&](std::string args)
	{
		m_currentFbxName = args;
		FbxLoadMeshFromFile(&m_fbxMeshBuilder, m_skeleton, m_motion, m_skeletonInstance, m_currentFbxName.c_str());
		VertexBuffer *vertexBuffer = g_theSimpleRenderer->m_rhiDevice->CreateVertexBuffer(m_fbxMeshBuilder.m_vertices);
		IndexBuffer *indexBuffer = g_theSimpleRenderer->m_rhiDevice->CreateIndexBuffer(m_fbxMeshBuilder.m_indices);
		fbxVBA.push_back(vertexBuffer);
		fbxIBA.push_back(indexBuffer);
		m_isRenderingMeshBuilder = true;
		m_displayType = SKINNING;
	};
	g_theDevConsole->AddFunctionCommand("fbxDragged", "Loads the mesh, skeleton, and motion from the file dragged onto exe.", Func);

	Func = [&](std::string args)
	{
		m_currentFbxName = args;
		m_skeleton.LoadSkeleton(m_fileStream, Stringf("%s.skel", args.c_str()));
		m_skeleton.GetVertexBufferVector(fbxVBA, fbxIBA, g_theSimpleRenderer);
		m_isRenderingMeshBuilder = true;
		m_displayType = SKELETON;
	};
	g_theDevConsole->AddFunctionCommand("fbxLoadSkel", "Loads a skeleton from file and displays it.", Func);

	Func = [&](std::string args)
	{
		m_currentFbxName = args;
		m_skeleton.LoadSkeleton(m_fileStream, Stringf("%s.skel", args.c_str()));
		m_motion.LoadMotion(m_fileStream, Stringf("%s.motion", args.c_str()));

		m_skeletonInstance.m_skeleton = &m_skeleton;
		m_motion.m_currentTime = 0.f;
		m_isRenderingMeshBuilder = true;
		m_displayType = MOTION;
	};
	g_theDevConsole->AddFunctionCommand("fbxLoadMotion", "Loads motion from file and displays it.", Func);
	
	Func = [&](std::string args)
	{
		m_currentFbxName = args;

		m_skeleton.LoadSkeleton(m_fileStream, Stringf("%s.skel", args.c_str()));
		m_motion.LoadMotion(m_fileStream, Stringf("%s.motion", args.c_str()));
		m_skeletonInstance.m_skeleton = &m_skeleton;
		m_motion.m_currentTime = 0.f;
		
		m_fbxMeshBuilder.LoadMesh(m_fileStream, Stringf("%s.mesh", args.c_str()));
		VertexBuffer *vertexBuffer = g_theSimpleRenderer->m_rhiDevice->CreateVertexBuffer(m_fbxMeshBuilder.m_vertices);
		IndexBuffer *indexBuffer = g_theSimpleRenderer->m_rhiDevice->CreateIndexBuffer(m_fbxMeshBuilder.m_indices);
		fbxVBA.push_back(vertexBuffer);
		fbxIBA.push_back(indexBuffer);

		m_isRenderingMeshBuilder = true;
		m_displayType = SKINNING;
	};
	g_theDevConsole->AddFunctionCommand("fbxLoadSkin", "Loads skinnging info from file and displays it.", Func);

	Func = [&](std::string args)
	{
		if (!fbxVBA.empty())
		{
			delete fbxVBA[0];
			fbxVBA[0] = nullptr;
			fbxVBA.clear();

			delete fbxIBA[0];
			fbxIBA[0] = nullptr;
			fbxIBA.clear();
		}

		m_currentFbxName = args;
		m_fbxMeshBuilder = *MeshDatabase::m_meshes[args];

		VertexBuffer *vertexBuffer = g_theSimpleRenderer->m_rhiDevice->CreateVertexBuffer(m_fbxMeshBuilder.m_vertices);
		IndexBuffer *indexBuffer = g_theSimpleRenderer->m_rhiDevice->CreateIndexBuffer(m_fbxMeshBuilder.m_indices);
		fbxVBA.push_back(vertexBuffer);
		fbxIBA.push_back(indexBuffer);
		m_isRenderingMeshBuilder = true;
		m_displayType = SKINNING;
	};
	g_theDevConsole->AddFunctionCommand("changeSkin", "Changes a mesh, skeleton, and motion, and displays them from the database.", Func);

	Func = [&](std::string args)
	{
		MeshBuilder *builder = MeshDatabase::m_meshes[args];

		VertexBuffer *vertexBuffer = g_theSimpleRenderer->m_rhiDevice->CreateVertexBuffer(builder->m_vertices);
		IndexBuffer *indexBuffer = g_theSimpleRenderer->m_rhiDevice->CreateIndexBuffer(builder->m_indices);
		fbxVBA.push_back(vertexBuffer);
		fbxIBA.push_back(indexBuffer);
	};
	g_theDevConsole->AddFunctionCommand("fbxAddSkin", "Loads a mesh, skeleton, and motion, and displays them.", Func);
}

void Game::Update(float deltaSeconds)
{
	deltaSeconds  = ChangeSimulationSpeed(deltaSeconds);
	KeyDown();
	KeyUp();

	UpdateFacialAnimation(deltaSeconds);
	
	m_time.time += deltaSeconds;
	m_timeConstant->Update(g_theSimpleRenderer->m_rhiContext, &m_time);
	m_blendShapeNumConstant->Update(g_theSimpleRenderer->m_rhiContext, &m_blendShapesNum);

	g_theDevConsole->UpdateCaret(deltaSeconds);

	if (m_displayType == SKINNING)
	{
		m_motion.AddToCurrentTime(deltaSeconds);
		m_skeletonInstance.ApplyMotion(&m_motion);
	}

	if (g_theDevConsole->m_isConsoleActive)
		return;

	UpdateCamera(deltaSeconds);
}

void Game::Render()
{
	RenderDefaults();
	RenderDraw();
	RenderConsole();
	g_theSimpleRenderer->DrawMeshBuilder();
	Present();
	g_theSimpleRenderer->m_meshBuilder->Clear();
}

void Game::RenderDefaults()
{
	g_theSimpleRenderer->SetRenderTarget(nullptr);
	g_theSimpleRenderer->ClearColor(RGBA::BLACK);
	g_theSimpleRenderer->ClearDepth();
	g_theSimpleRenderer->SetViewport(0, 0, (unsigned int)m_windowWidth, (unsigned int)m_windowHeight);

	float aspectRatio = (float)m_windowWidth / (float)m_windowHeight;
	g_theSimpleRenderer->SetViewMatrixTranslationAndRotation(m_camera.m_position, m_camera.m_rollAboutX, m_camera.m_pitchAboutY, m_camera.m_yawAboutZ);
	g_theSimpleRenderer->SetPerspectiveProjection(D2R(m_fov), aspectRatio, 0.01f, 1000.f);
	g_theSimpleRenderer->EnableDepthTest(true, true);

	if (m_displayType == SKINNING)
	{
		g_theSimpleRenderer->SetAmbientLight(0.5f);
		g_theSimpleRenderer->EnablePointLight(0, -m_camera.m_position, RGBA::WHITE, 200.f);
		g_theSimpleRenderer->SetShader(g_theSimpleRenderer->m_morphTargetShader);
	}
}

void Game::RenderDraw()
{
	if (m_isRenderingMeshBuilder)
	{
		if (m_displayType == SKINNING)
		{
			g_theSimpleRenderer->SetModelMatrix(m_faceRotation);
			g_theSimpleRenderer->SetTexture(m_skinTexture);
			g_theSimpleRenderer->DrawVertexBufferArray(PRIMITIVE_TRIANGLES, fbxVBA, fbxIBA);
			g_theSimpleRenderer->SetModelMatrix(m_identityMatrix);
		}
	}
}

void Game::RenderConsole() const
{
	g_theSimpleRenderer->SetShader(g_theSimpleRenderer->m_lightShader);
	g_theSimpleRenderer->EnableBlend(BLEND_SRC_ALPHA, BLEND_INV_SRC_ALPHA);
	g_theSimpleRenderer->SetViewMatrixTranslationAndRotation(Vector3(), 0.f, 0.f, 0.f);
	g_theSimpleRenderer->ClearDepth();
	g_theSimpleRenderer->EnableDepthTest(false, false);
	g_theSimpleRenderer->SetOrthoProjection(Vector2(0.f, 0.f), Vector2(1280.f, 720.f));

//	g_theSimpleRenderer->DrawText2D(Vector2(20.f, 680.f), 1.f, RGBA::WHITE, g_theSimpleRenderer->m_font, GetNameForEmotion().c_str() );
// 	g_theSimpleRenderer->DrawText2D(Vector2(20.f, 650.f), 1.f, RGBA::WHITE, g_theSimpleRenderer->m_font, Stringf("Blink Target: %f", m_morphWeightTargets[L_BLINK]).c_str() );
// 	g_theSimpleRenderer->DrawText2D(Vector2(20.f, 620.f), 1.f, RGBA::WHITE, g_theSimpleRenderer->m_font, Stringf("Current Amplitude: %f", g_theAudio->GetSpeechAmplitude()).c_str() );
// 	g_theSimpleRenderer->DrawText2D(Vector2(20.f, 590.f), 1.f, RGBA::WHITE, g_theSimpleRenderer->m_font, Stringf("Current Index: %i", g_theAudio->GetSpeechIndex()).c_str() );

	if (g_theDevConsole->m_isConsoleActive)
	{
		g_theDevConsole->Render();
	}
}

void Game::Present() const
{
	g_theSimpleRenderer->Present();
}

void Game::Destroy()
{
	if (!fbxVBA.empty())
	{
		delete fbxVBA[0];
		fbxVBA[0] = nullptr;
		fbxVBA.clear();
	}

	if (!fbxIBA.empty())
	{
		delete fbxIBA[0];
		fbxIBA[0] = nullptr;
		fbxIBA.clear();
	}

	delete g_theDevConsole;
	g_theDevConsole = nullptr;

	delete m_blendShapeNumConstant;
	m_blendShapeNumConstant = nullptr;

	delete m_timeConstant;
	m_timeConstant = nullptr;

	if (m_pointSampler != nullptr)
		delete m_pointSampler;
	m_pointSampler = nullptr;

	delete m_diffuseTexture;
	m_diffuseTexture = nullptr;

	delete m_normalTexture;
	m_normalTexture = nullptr;

	m_myShader = nullptr;

	delete m_shaders;
	m_shaders = nullptr;
}

float Game::ChangeSimulationSpeed(float deltaSeconds) const
{
	if (m_isPaused)
		deltaSeconds = 0.f;

	return deltaSeconds;
}

void Game::KeyDown()
{
}

void Game::KeyUp()
{
	if (g_theInput->WasKeyJustPressed('1'))
	{
		g_theSpeechToTextSystem->GetPhonemeTimesFromFile("Data/Audio/goforward.raw", timedWords);
		SoundID testSoundID = g_theAudio->CreateOrGetSound("Data/Audio/goforward.wav");
		g_theAudio->PlaySpeech(testSoundID);
		m_timeFaceRunning = 0.00000001f;
		timedWordsIter = timedWords.begin();
	}
	if (g_theInput->WasKeyJustPressed('2'))
	{
		g_theSpeechToTextSystem->GetPhonemeTimesFromFile("Data/Audio/something.raw", timedWords);
		SoundID testSoundID = g_theAudio->CreateOrGetSound("Data/Audio/something.wav");
		g_theAudio->PlaySpeech(testSoundID);
		m_timeFaceRunning = 0.00000001f;
		timedWordsIter = timedWords.begin();
	}
}

void Game::UpdateFacialAnimation(float deltaSeconds)
{
	UpdateBlinking();
	UpdateEmotion();
	UpdateMorphWeightsBasedOnTimedWords(deltaSeconds);
	UpdateMorphWeights();
	UpdateFaceFollow(deltaSeconds);
}

void Game::UpdateEmotion()
{
	if (m_currentEmotion == EMOTION_NORMAL)
	{
		return;
	}
	else if (m_currentEmotion == EMOTION_ANGER)
	{
		m_morphWeightTargets[ANGER] = 1.f;
	}
	else if (m_currentEmotion == EMOTION_CONCERN)
	{
		m_morphWeightTargets[CONCERN] = 1.f;
	}
	else if (m_currentEmotion == EMOTION_SURPISE)
	{
		m_morphWeightTargets[BROWS_UP] = 1.f;
	}
	else if (m_currentEmotion == EMOTION_SUSPICION)
	{
		m_morphWeightTargets[L_SQUINT] = 1.f;
		m_morphWeightTargets[R_SQUINT] = 1.f;
	}
	else if (m_currentEmotion == EMOTION_SMIRK)
	{
		m_morphWeightTargets[L_SMIRK] = 1.f;
	}
	else if (m_currentEmotion == EMOTION_SCARED)
	{
		m_morphWeightTargets[SCARED] = 1.f;
	}
}

void Game::UpdateBlinking()
{
	float noiseThisFrame = Compute1dPerlinNoise( (float)GetCurrentTimeSeconds(), 1.f, 1, 0.1f, 1.f, true, 0);

	if (noiseThisFrame > 0.98f)
	{
		m_morphWeightTargets[L_BLINK] = 1.f;
		m_morphWeightTargets[R_BLINK] = 1.f;
	}
	else
	{
		float subtractorValue = 0.8f;
		m_morphWeightTargets[L_BLINK] -= subtractorValue;
		m_morphWeightTargets[R_BLINK] -= subtractorValue;

		if (m_morphWeightTargets[L_BLINK] <= 0.f && m_morphWeightTargets[R_BLINK] <= 0.f)
		{
			m_morphWeightTargets[L_BLINK] = 0.f;
			m_morphWeightTargets[R_BLINK] = 0.f;
		}
	}
}

void Game::UpdateFaceFollow(float deltaSeconds)
{
	UpdateFaceFollowTarget();
	UpdateFaceFollowCurrent(deltaSeconds);
}

void Game::UpdateFaceFollowTarget()
{
	Vector3 dir = Vector3(0.f, 0.f, 0.f) - m_camera.m_position;
	Vector3 forward = dir.GetNormalized();
	Vector3 right = Vector3(0.f, 1.f, 0.f).Cross(forward);
	Vector3 up = forward.Cross(right);

	Vector4 i = Vector4(right, 0.0f);
	Vector4 j = Vector4(up, 0.0f);
	Vector4 k = Vector4(forward, 0.0f);
	Vector4 t = Vector4(Vector3(0.f, 0.f, 0.f), 1.0f);

	m_targetFaceRotation = Matrix4(i, j, k, t);

	if (!m_isMovingFace)
	{
		return;
	}

	const float ROLL_DEGREES_PER_MOUSE_MOVE = 0.05f;
	const float YAW_DEGREES_PER_MOUSE_MOVE = -0.05f;
	const float PITCH_DEGREES_PER_MOUSE_MOVE = 0.03f;
	const float MOVE_SPEED = 1.0f;

	Vector2 screenSize(1280.f, 720.f);
	Vector2 mouseCenterScreenPos = screenSize * 0.5f;
	Vector2 mouseScreenCoords = g_theInput->GetMouseWindowCoords();
	mouseScreenCoords = RangeMap2D(mouseScreenCoords, Vector2(0.f, 1280.f), Vector2(0.f, 720.f), Vector2(-1280.f, 1280.f), Vector2(-720.f, 720.f));

	m_faceRollAboutX = -ROLL_DEGREES_PER_MOUSE_MOVE * mouseScreenCoords.y;
	m_facePitchAboutY = PITCH_DEGREES_PER_MOUSE_MOVE * mouseScreenCoords.x;

	m_targetFaceRotation.RotateDegreesAboutX(m_faceRollAboutX);
	m_targetFaceRotation.RotateDegreesAboutY(m_facePitchAboutY);
}

void Game::UpdateFaceFollowCurrent(float deltaSeconds)
{
	m_faceRotationLerpPercentage += deltaSeconds;
	if (m_faceRotationLerpPercentage > 1.f)
	{
		m_faceRotationLerpPercentage = 1.f;
		m_camera.m_hasMoved = false;
	}

	m_faceRotation = LerpLinear(m_oldRotation, m_targetFaceRotation, m_faceRotationLerpPercentage);
}

void Game::UpdateMorphWeights()
{
	float divisorValue = 15.f;
	for (int morphTypeIndex = 0; morphTypeIndex < NUM_MORPHS_TYPES; morphTypeIndex++)
	{
		if (morphTypeIndex == L_BLINK || morphTypeIndex == R_BLINK)
		{
			divisorValue = 0.5f;
		}
		else
		{
			divisorValue = 15.f;
		}

		if (m_morphWeightTargets[morphTypeIndex] > m_morphWeights[morphTypeIndex])
		{
			m_morphWeights[morphTypeIndex] += m_morphWeightTargets[morphTypeIndex] / divisorValue;
			if (m_morphWeights[morphTypeIndex] > m_morphWeightTargets[morphTypeIndex])
				m_morphWeights[morphTypeIndex] = m_morphWeightTargets[morphTypeIndex];
		}
		else
			m_morphWeights[morphTypeIndex] = m_morphWeightTargets[morphTypeIndex];

		if (m_morphWeights[morphTypeIndex] > 1.f)
			m_morphWeights[morphTypeIndex] = 1.f;
	}

	m_morphWeightsSB->Update(g_theSimpleRenderer->m_rhiContext, m_morphWeights);
}

void Game::UpdateMorphWeightsBasedOnAmplitude()
{
	float currentAmplitude = g_theAudio->GetSpeechAmplitude() * 100;
	if (currentAmplitude > 1.f)
		currentAmplitude = 1.f;

	MorphType_e morphType1 = FV;
	MorphType_e morphType2 = WQ;
	MorphType_e morphType3 = AI;
	MorphType_e morphType4 = O;
	MorphType_e morphType5 = E;

	float ampPercent1 = 0.12f;
	float ampPercent2 = 0.26f;
	float ampPercent3 = 0.47f;
	float ampPercent4 = 0.58f;
	float ampPercent5 = 0.8f;

	float subtractorValue = 0.06f;

	for (int morphTypeIndex = 0; morphTypeIndex < NUM_MORPHS_TYPES; morphTypeIndex++)
	{
		m_morphWeightTargets[morphTypeIndex] -= subtractorValue;

		if (m_morphWeightTargets[morphTypeIndex] < 0)
			m_morphWeightTargets[morphTypeIndex] = 0.f;
	}

	if (currentAmplitude < ampPercent1)
		m_morphWeightTargets[morphType1] = currentAmplitude + ampPercent2;
	else if (currentAmplitude < ampPercent2)
		m_morphWeightTargets[morphType2] = currentAmplitude + ampPercent1;
	else if (currentAmplitude < ampPercent3)
		m_morphWeightTargets[morphType3] = currentAmplitude;
	else if (currentAmplitude < ampPercent4)
		m_morphWeightTargets[morphType4] = currentAmplitude - ampPercent1;
	else if (currentAmplitude < ampPercent5)
		m_morphWeightTargets[morphType5] = currentAmplitude - ampPercent2;

	for (int morphTypeIndex = 0; morphTypeIndex < NUM_MORPHS_TYPES; morphTypeIndex++)
	{
		if (m_morphWeightTargets[morphTypeIndex] > m_morphWeights[morphTypeIndex])
		{
			m_morphWeights[morphTypeIndex] += m_morphWeightTargets[morphTypeIndex] / 15.f;
			if (m_morphWeights[morphTypeIndex] > m_morphWeightTargets[morphTypeIndex])
				m_morphWeights[morphTypeIndex] = m_morphWeightTargets[morphTypeIndex];
		}
		else
			m_morphWeights[morphTypeIndex] = m_morphWeightTargets[morphTypeIndex];

		if (m_morphWeights[morphTypeIndex] > 1.f)
			m_morphWeights[morphTypeIndex] = 1.f;
	}

	m_morphWeightsSB->Update(g_theSimpleRenderer->m_rhiContext, m_morphWeights);
}

void Game::UpdateMorphWeightsBasedOnTimedWords(float deltaSeconds)
{
	if (m_timeFaceRunning == 0.f || timedWordsIter == timedWords.end())
		return;

	m_timeFaceRunning += deltaSeconds;

	float subtractorValue = 0.1f;
	for (int morphTypeIndex = 0; morphTypeIndex < NUM_MORPHS_TYPES; morphTypeIndex++)
	{
		m_morphWeightTargets[morphTypeIndex] -= subtractorValue;

		if (m_morphWeightTargets[morphTypeIndex] < 0)
			m_morphWeightTargets[morphTypeIndex] = 0.f;
	}

	if ((*timedWordsIter)->startTime < m_timeFaceRunning && (*timedWordsIter)->endTime > m_timeFaceRunning)
	{
		m_currentAmplitude = g_theAudio->GetSpeechAmplitude() * 500;
		if (m_currentAmplitude < 0.f)
			m_currentAmplitude = 0.f;
		else if (m_currentAmplitude > 1.f)
			m_currentAmplitude = 1.f;


		MorphType_e mType = GetMorphTypeFromString((*timedWordsIter)->word);
		if (m_currentAmplitude >= 0.f && m_currentAmplitude <= 1.f)
			m_morphWeightTargets[mType] = m_currentAmplitude;
		else
			m_morphWeightTargets[mType] = 1.f;
	}
	else if ((*timedWordsIter)->endTime < m_timeFaceRunning)
	{
		++timedWordsIter;
	}
}

MorphType_e Game::GetMorphTypeFromString(std::string phoneme)
{
	if (phoneme == "OW" || phoneme == "O" || phoneme == "OH")
		return O;
	else if (phoneme == "AO" || phoneme == "AE" || phoneme == "IY" || phoneme == "IH" || phoneme == "AA" || phoneme == "AY" || phoneme == "AH" || phoneme == "AW" || phoneme == "EY")
		return AI;
	else if (phoneme == "EH")
		return E;
	else if (phoneme == "W" || phoneme == "Q" || phoneme == "QU")
		return WQ;
	else if (phoneme == "UH" || phoneme == "UW" || phoneme == "ER" || phoneme == "R")
		return U;
	else if (phoneme == "L" || phoneme == "TH" || phoneme == "T")
		return LTH;
	else if (phoneme == "F" || phoneme == "V")
		return FV;
	else if (phoneme == "M" || phoneme == "B" || phoneme == "P" || phoneme == "S" || phoneme == "D" || phoneme == "K" || phoneme == "N")
		return MBP;
	else 
		return CONSONENTS;

}

void Game::CreateAndStoreShaderFromConfig()
{
	int varNum = 1;
	std::string shaderString = "shader";
	std::string varName = shaderString;
	varName += std::to_string(varNum);
	while (g_theConfig->IsConfigSet(varName.c_str()))
	{
		char const *shaderFileName;
		g_theConfig->ConfigGetString(&shaderFileName, varName.c_str());
		m_shaders->CreateShaderFromFile(g_theSimpleRenderer->m_rhiDevice, shaderFileName);
		
		varName = shaderString;
		varNum++;
		varName += std::to_string(varNum);
	}
}

void Game::ChangeToNextShader()
{
	m_myShader = m_shaders->GetNextShaderInMap();
}

void Game::ChangeToPrevShader()
{
	m_myShader = m_shaders->GetPreviousShaderInMap();
}

void Game::IncrementMorphTarget()
{
	int morphTypeInt = (int)m_currentMorphType;
	morphTypeInt++;
	m_currentMorphType = (MorphType_e)morphTypeInt;
	if (m_currentMorphType == NUM_MORPHS_TYPES)
		m_currentMorphType = AI;
}

void Game::DecrementMorphTarget()
{
	int morphTypeInt = (int)m_currentMorphType;

	if (morphTypeInt == 0)
		morphTypeInt = (int)NUM_MORPHS_TYPES;
	morphTypeInt--;

	m_currentMorphType = (MorphType_e)morphTypeInt;
}

void Game::IncrementCurrentEmotion()
{
	int emotionInt = (int)m_currentEmotion;
	emotionInt++;
	m_currentEmotion = (Emotion_e)emotionInt;
	if (m_currentEmotion == NUM_EMOTIONS)
		m_currentEmotion = EMOTION_NORMAL;
}

void Game::DecrementCurrentEmotion()
{
	int emotionInt = (int)m_currentEmotion;
	if (emotionInt == 0)
		emotionInt = (int)NUM_EMOTIONS;
	emotionInt--;
	m_currentEmotion = (Emotion_e)emotionInt;
}

void Game::SetMorphTargetStructuredBuffer()
{
	for (unsigned int morphTypesIndex = 0; morphTypesIndex < NUM_MORPHS_TYPES; morphTypesIndex++)
	{
		m_morphWeights[morphTypesIndex] = 0;
	}

	std::vector<Vector3> m_vertices;
	m_currentMorphType = AI;

	unsigned int morphIndicesIndex = 0;
	unsigned int vertexCount = 0;
	MeshBuilder *baseMesh = MeshDatabase::m_meshes["Morpher1"];

	do 
	{
		m_morphIndices[morphIndicesIndex] = vertexCount;
		morphIndicesIndex++;

		MeshBuilder *mesh = MeshDatabase::m_meshes[GetNameForMorphType()];
		for (unsigned int vertexIndex = 0; vertexIndex < (unsigned int)mesh->m_indices.size(); vertexIndex++)
		{
			unsigned int index = mesh->m_indices[vertexIndex];
			Vector3 &baseMeshPos = baseMesh->m_vertices[index].position;
			Vector3 &meshPos = mesh->m_vertices[index].position;
			m_vertices.push_back(meshPos - baseMeshPos);
			vertexCount++;
		}
		IncrementMorphTarget();
	} while (m_currentMorphType != AI);

	m_morphTargetSB = new StructuredBuffer(g_theSimpleRenderer->m_rhiDevice, m_vertices.data(), sizeof(Vector3), m_vertices.size());
	g_theSimpleRenderer->SetStructuredBuffer(4, m_morphTargetSB);

	m_morphWeightsSB = new StructuredBuffer(g_theSimpleRenderer->m_rhiDevice, m_morphWeights, sizeof(float), NUM_MORPHS_TYPES);
	g_theSimpleRenderer->SetStructuredBuffer(3, m_morphWeightsSB);

	m_morphIndicesSB = new StructuredBuffer(g_theSimpleRenderer->m_rhiDevice, m_morphIndices, sizeof(unsigned int), NUM_MORPHS_TYPES);
	g_theSimpleRenderer->SetStructuredBuffer(2, m_morphIndicesSB);
}

std::string Game::GetNameForMorphType() const
{
	switch (m_currentMorphType)
	{
	case AI:
		return "AI";
	case ANGER:
		return "anger";
	case BROWS_UP:
		return "BrowsUp";
	case CONCERN:
		return "Concern";
	case CONSONENTS:
		return "Consonents";
	case E:
		return "E";
	case FV:
		return "F,V";
	case LTH:
		return "L,TH";
	case L_BLINK:
		return "LEyeBlink";
	case L_SQUINT:
		return "LEyeSquint";
	case L_SMIRK:
		return "LSmirk";
	case MBP:
		return "MBP";
	case O:
		return "O";
	case R_BLINK:
		return "REyeBlink";
	case R_SQUINT:
		return "REyeSquint";
	case SCARED:
		return "Scared";
	case U:
		return "U";
	case WQ:
		return "W,Q";
	}
	return "AI";
}

std::string Game::GetNameForEmotion() const
{
	switch (m_currentEmotion)
	{
	case EMOTION_ANGER:
		return "Anger";
	case EMOTION_SURPISE:
		return "Surprise";
	case EMOTION_CONCERN:
		return "Concern";
	case EMOTION_SUSPICION:
		return "Suspicion";
	case EMOTION_SMIRK:
		return "Smirk";
	case EMOTION_SCARED:
		return "Scared";
	}
	return "Normal";
}

void Game::UpdateCamera(float deltaSeconds)
{
	if (m_isMovingFace == true)
		return;

	const float YAW_DEGREES_PER_MOUSE_MOVE = -0.05f;
	const float PITCH_DEGREES_PER_MOUSE_MOVE = 0.03f;
	const float MOVE_SPEED = 1.0f;

	Vector2 screenSize = g_theInput->GetScreenSize();
	Vector2 mouseRecentPos = screenSize * 0.5f;
	Vector2 mouseScreenCoords = g_theInput->GetMouseScreenCoords();
	Vector2 mouseMoveSinceLastFrame = mouseScreenCoords - mouseRecentPos;

	g_theInput->SetMouseScreenCoords(mouseRecentPos);
	m_camera.m_pitchAboutY -= PITCH_DEGREES_PER_MOUSE_MOVE * mouseMoveSinceLastFrame.y;
	m_camera.m_pitchAboutY = Clamp(m_camera.m_pitchAboutY, -89.9f, 89.9f);
	m_camera.m_yawAboutZ += YAW_DEGREES_PER_MOUSE_MOVE * mouseMoveSinceLastFrame.x;

	const float moveDistanceThisFrame = MOVE_SPEED * deltaSeconds;
	Vector3 cameraForwardXY = m_camera.GetForwardXY();
	Vector3 cameraLeftXY = m_camera.GetLeftXY();
	Vector3 moveDisplacementThisFrame(0.f, 0.f, 0.f);

	bool hasMovedThisFrame = false;
	float scale = 100.f;
	if (g_theInput->IsKeyDown('W'))
	{
		moveDisplacementThisFrame += cameraForwardXY * scale;
		hasMovedThisFrame = true;
	}

	if (g_theInput->IsKeyDown('S'))
	{
		moveDisplacementThisFrame -= cameraForwardXY * scale;
		hasMovedThisFrame = true;
	}

	if (g_theInput->IsKeyDown('A'))
	{
		moveDisplacementThisFrame += cameraLeftXY * scale;
		hasMovedThisFrame = true;
	}

	if (g_theInput->IsKeyDown('D'))
	{
		moveDisplacementThisFrame -= cameraLeftXY * scale;
		hasMovedThisFrame = true;
	}

	if (g_theInput->IsKeyDown(' '))
	{
		moveDisplacementThisFrame.y -= 1.f * scale;
		hasMovedThisFrame = true;
	}

	if (g_theInput->IsKeyDown('Z'))
	{
		moveDisplacementThisFrame.y += 1.0f * scale;
		hasMovedThisFrame = true;
	}

	m_camera.m_position += (moveDisplacementThisFrame * moveDistanceThisFrame);

	if (g_theInput->WasKeyJustPressed('O'))
	{
		m_camera.ResetCameraPositionAndOrientation();
		hasMovedThisFrame = true;
	}

	if (hasMovedThisFrame == true)
	{
		if (m_camera.m_hasMoved == false)
			m_oldRotation = m_targetFaceRotation;
		else
			m_oldRotation = m_faceRotation;
		
		m_camera.m_hasMoved = true;
		m_faceRotationLerpPercentage = 0.f;
	}
}
