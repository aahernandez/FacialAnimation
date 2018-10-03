#include "Game/GameCommon.hpp"
#include "Engine/Audio/SpeechToTextSystem.hpp"
#include "Engine/RHI/SimpleRenderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Config.hpp"

SpeechToTextSystem *g_theSpeechToTextSystem = nullptr;
SimpleRenderer *g_theSimpleRenderer = nullptr;
InputSystem *g_theInput = nullptr;
AudioSystem *g_theAudio = nullptr;
Config *g_theConfig = nullptr;