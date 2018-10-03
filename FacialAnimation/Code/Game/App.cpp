#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Time/Time.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/RHI/SimpleRenderer.hpp"
#include "Engine/Core/Config.hpp"
#include "Engine/Core/DeveloperConsole.hpp"
#include "Engine/Audio/SpeechToTextSystem.hpp"
#include "Game/GameConfig.hpp"

App* g_theApp = nullptr;

const float MIN_FRAMES_PER_SECOND = 10.f;
const float MAX_FRAMES_PER_SECOND = 60.f;
const float MIN_SECONDS_PER_FRAME = (1.f / MAX_FRAMES_PER_SECOND);
const float MAX_SECONDS_PER_FRAME = (1.f / MIN_FRAMES_PER_SECOND);

App::App()
	: isQuitting(false)
{
	g_theConfig = new Config("Data/Configs/game.config");

	g_theSpeechToTextSystem = new SpeechToTextSystem();
	
	g_theSimpleRenderer = new SimpleRenderer();
	int windowWidth;
	int windowHeight;
	int outputMode;
	const char *fontName;

	g_theConfig->ConfigGetInt(&windowWidth, "window_res_x");
	g_theConfig->ConfigGetInt(&windowHeight, "window_res_y");
	g_theConfig->ConfigGetInt(&outputMode, "window_style");
	g_theConfig->ConfigGetString(&fontName, "font");
	g_theSimpleRenderer->Setup((unsigned int) windowWidth, (unsigned int) windowHeight, (e_RHIOutputMode) outputMode, fontName);

	char const *title;
	g_theConfig->ConfigGetString(&title, "window_title");
	g_theSimpleRenderer->SetWindowTitle(title);

	g_theAudio = new AudioSystem();
	g_theInput = new InputSystem();
	g_theInput->HideMouseCursor();
	g_theGame = new Game();

	std::function<void(std::string args)> Func;
	Func = [&](std::string args) { OnExitRequested(); };
	g_theDevConsole->AddFunctionCommand("quit", "Exits the app", Func);
}

App::~App()
{
	delete g_theGame;
	g_theGame = nullptr;

	delete g_theInput;
	g_theInput = nullptr;

	delete g_theSimpleRenderer;
	g_theSimpleRenderer = nullptr;

	delete g_theSpeechToTextSystem;
	g_theSpeechToTextSystem = nullptr;

	delete g_theConfig;
	g_theConfig = nullptr;
}

void App::RunFrame()
{
	Input();
	float deltaSeconds = GetDeltaSeconds();
	Update(deltaSeconds);
	Render();
}

void App::Update(float deltaSeconds)
{
	if (g_theGame)
		g_theGame->Update(deltaSeconds);
}

void App::Input()
{
	if (g_theInput)
		g_theInput->UpdateInputState();
	
	if (g_theInput->WasKeyJustPressed(KEY_ESCAPE))
	{
		if (g_theDevConsole->m_isConsoleActive)
		{
			if (g_theDevConsole->IsCurrentLineEmpty())
				g_theDevConsole->ToggleConsole();
			else
				g_theDevConsole->ClearCurrentLine();
		}
		else 
		{
			OnExitRequested();
		}
	}
}

void App::OnExitRequested()
{
	isQuitting = true;
}

bool App::IsQuitting()
{
	return isQuitting;
}

void App::Render()
{
	if (g_theGame)
		g_theGame->Render();
}

float App::GetDeltaSeconds()
{
	double timeNow = GetCurrentTimeSeconds();
	static double lastFrameTime = timeNow;
	double deltaSeconds = timeNow - lastFrameTime;

	while (deltaSeconds < MIN_SECONDS_PER_FRAME * 0.999f)
	{
		timeNow = GetCurrentTimeSeconds();
		deltaSeconds = timeNow - lastFrameTime;
	}
	lastFrameTime = timeNow;

	if (deltaSeconds > MAX_SECONDS_PER_FRAME)
		deltaSeconds = MAX_SECONDS_PER_FRAME;

	return (float) deltaSeconds;
}
