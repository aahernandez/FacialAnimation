#include "Engine/Renderer/Window.hpp"

#include <math.h>
#include <cassert>
#include <crtdbg.h>
#include <shellapi.h>
#include <atlstr.h>
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/DeveloperConsole.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Game/App.hpp"
#include "Engine/RHI/DX11.hpp"

#define UNUSED(x) (void)(x);

void Initialize()
{
	g_theApp = new App();
}

void Shutdown()
{
	delete g_theApp;
	g_theApp = nullptr;
}

int WINAPI WinMain(HINSTANCE applicationInstanceHandle, HINSTANCE previousInstanceHandle, LPSTR commandLineString, int showCommand)
{
	UNUSED(commandLineString);
	UNUSED(applicationInstanceHandle);
	UNUSED(previousInstanceHandle);
	UNUSED(showCommand);

	int argCount;

	LPWSTR *szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);

	Initialize();
	if (argCount > 1)
	{
		for (int i = 1; i < argCount; i++)
		{
			std::string stringToPrint = CW2A(szArgList[i]);
			std::vector<std::string> parsedArgument = ParseStringIntoPiecesByDelimiter(stringToPrint, "\\");
			std::vector<std::string> parsedFileName = ParseStringIntoPiecesByDelimiter(parsedArgument.back(), ".");
			std::string fileName = *parsedFileName.begin();

			std::string lastString = parsedArgument.back();
			SkeletonInstance tempSkelInst;
			Motion tempMotion;
			Skeleton tempSkel;
			SkeletonDatabase tempSkelDB;
			MotionDatabase tempMotionDB;
			MeshBuilder tempMeshBuilder;
			MeshDatabase tempMeshDB;
			FileBinaryStream tempFileBinaryStream;
			tempMeshBuilder = *tempMeshDB.CreateOrGetMesh(fileName, &tempSkel, &tempMotion, &tempSkelInst, &tempSkelDB, &tempMotionDB);

			tempSkel.SaveSkeleton(tempFileBinaryStream, Stringf("%s.skel", fileName.c_str()));
			tempMotion.SaveMotion(tempFileBinaryStream, Stringf("%s.motion", fileName.c_str()));
			tempMeshBuilder.SaveMesh(tempFileBinaryStream, Stringf("%s.mesh", fileName.c_str()));
		}
		Shutdown();
	}

	while (!g_theApp->IsQuitting())
	{
		g_theApp->RunFrame();
	}
	Shutdown();
	return 0;
}