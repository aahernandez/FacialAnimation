#pragma once
class DeveloperConsole;

class App
{
public:
	App();
	~App();

	void RunFrame();
	void Input();
	void OnExitRequested();
	bool IsQuitting();

private:
	bool isQuitting;

	void Update(float deltaSeconds);
	void Render();
	float GetDeltaSeconds();
};

extern App* g_theApp;