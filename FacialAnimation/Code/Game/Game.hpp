//Some code based off of code by Squirrel Eiserloh
#pragma once
#include "Game/Camera.hpp"

#include "Engine/RHI/Sampler.hpp"
#include "Engine/RHI/Skeleton.hpp"
#include "Engine/RHI/Texture2D.hpp"
#include "Engine/RHI/MeshBuilder.hpp"
#include "Engine/RHI/MeshDatabase.hpp"
#include "Engine/Input/XboxControl.hpp"
#include "Engine/Math/MathUtilities.hpp"
#include "Engine/RHI/SimpleRenderer.hpp"
#include "Engine/RHI/MotionDatabase.hpp"
#include "Engine/RHI/SkeletonDatabase.hpp"
#include "Engine/RHI/SkeletonInstance.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Streams/FileBinaryStream.hpp"

struct TimedWord_t;
class ShaderProgram;
class ShaderDatabase;
class Font;

struct TimeConstant_t
{
	float time;
	Vector3 padding;
};

struct BlendShapeNumConstant_t
{
	unsigned int numBlendShapes;
	unsigned int padding1;
	unsigned int padding2;
	unsigned int padding3;
};

enum FbxDisplayType_e
{
	NONE,
	MESH,
	SKELETON,
	MOTION,
	SKINNING,
	NUM_TYPES
};

enum MorphType_e
{
	AI,
	ANGER,
	BROWS_UP,
	CONCERN,
	CONSONENTS,
	E,
	FV,
	LTH,
	L_BLINK,
	L_SQUINT,
	L_SMIRK,
	MBP,
	O,
	R_BLINK,
	R_SQUINT,
	SCARED,
	U,
	WQ,
	NUM_MORPHS_TYPES,
};

enum Emotion_e
{
	EMOTION_NORMAL,
	EMOTION_ANGER,
	EMOTION_SURPISE,
	EMOTION_CONCERN,
	EMOTION_SUSPICION,
	EMOTION_SMIRK,
	EMOTION_SCARED,
	NUM_EMOTIONS,
};

class Game
{
public:
	float m_fov;
	bool m_isPaused;
	int m_windowWidth;
	int m_windowHeight;
	int m_currentMorphIndex;
	bool m_isRenderingMeshBuilder;
	MorphType_e m_currentMorphType;
	Emotion_e m_currentEmotion;

	Camera m_camera;
	Motion m_motion;
	Skeleton m_skeleton;
	Sampler *m_pointSampler;
	ShaderDatabase *m_shaders;
	ShaderProgram *m_myShader;
	Texture2D *m_normalTexture;
	Texture2D *m_diffuseTexture;
	Texture2D *m_skinTexture;
	MeshDatabase m_meshDatabase;
	std::string m_currentFbxName;
	MeshBuilder m_fbxMeshBuilder;
	FbxDisplayType_e m_displayType;
	FileBinaryStream m_fileStream;
	MotionDatabase m_motionDatabase;
	SkeletonDatabase m_skelDatabase;
	std::vector<IndexBuffer*> fbxIBA;
	std::vector<VertexBuffer*> fbxVBA;
	SkeletonInstance m_skeletonInstance;
	StructuredBuffer *m_morphTargetSB;
	float m_morphWeightTargets[NUM_MORPHS_TYPES];
	float m_morphWeights[NUM_MORPHS_TYPES];
	StructuredBuffer *m_morphWeightsSB;
	unsigned int m_morphIndices[NUM_MORPHS_TYPES];
	StructuredBuffer *m_morphIndicesSB;
	float m_timeFaceRunning;
	std::vector<TimedWord_t*> timedWords;
	std::vector<TimedWord_t*>::iterator timedWordsIter;
	Matrix4 m_faceRotation;
	Matrix4 m_oldRotation;
	Matrix4 m_targetFaceRotation;
	float m_faceRollAboutX;
	float m_facePitchAboutY;
	float m_faceYawAboutZ;
	bool m_isMovingFace;
	float m_faceRotationLerpPercentage;
	float m_currentAmplitude;
	Matrix4 m_identityMatrix;
	Vector2 m_prevMousePosition;

	TimeConstant_t m_time;
	ConstantBuffer *m_timeConstant;

	BlendShapeNumConstant_t m_blendShapesNum;
	ConstantBuffer *m_blendShapeNumConstant;


	Game();
	~Game();

	void Initialize();
	void InitDevConsoleFunctions();
	void Update(float deltaSeconds);
	void Render();
	void RenderDefaults();
	void RenderDraw();
	void RenderConsole() const;
	void Present() const;
	void Destroy();

	float ChangeSimulationSpeed(float deltaSeconds) const;
	void KeyDown();
	void KeyUp();

	void UpdateFacialAnimation(float deltaSeconds);
	void UpdateEmotion();
	void UpdateBlinking();
	void UpdateFaceFollow(float deltaSeconds);
	void UpdateFaceFollowTarget();
	void UpdateFaceFollowCurrent(float deltaSeconds);
	void UpdateMorphWeights();
	void UpdateMorphWeightsBasedOnAmplitude();
	void UpdateMorphWeightsBasedOnTimedWords(float deltaSeconds);
	MorphType_e GetMorphTypeFromString(std::string phoneme); 

	void CreateAndStoreShaderFromConfig();
	void ChangeToNextShader();
	void ChangeToPrevShader();
	void IncrementMorphTarget();
	void DecrementMorphTarget();
	void IncrementCurrentEmotion();
	void DecrementCurrentEmotion();
	void SetMorphTargetStructuredBuffer();
	std::string GetNameForMorphType() const;
	std::string GetNameForEmotion() const;

	void UpdateCamera(float deltaSeconds);
};

extern Game* g_theGame;