# Facial Animation
## Description
A simple lip syncing and facial animation system. I explored blend shapes and speech-to-text systems in its design. More information can be found on my [portfolio](https://www.aahernandez.net/facial-animation).

## Controls
'1'&ensp;&ensp;&ensp;&ensp;- Play first sound bite.  
'2'&ensp;&ensp;&ensp;&ensp;- Play second sound bite.  

## Run
An executable is provided in the [Run_Win32](FacialAnimation/Run_Win32) folder.

## Build
The project was built using Visual Studio 17. If you would like to built it yourself, you can open the visual studio [solution](FacialAnimation/FacialAnimation.sln) to do so. You must have a copy of the FBX SDK 2019.0 VS 2015 for Windows which can be found on the [Autodesk website](https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2019-0). An environment variable must then be created on your PC called FBXSDK_DIR with the value being the directory where the FBX SDK is installed. On my system, this path looked like this "C:\Program Files\Autodesk\FBX\FBX SDK\2019.0". It is recommended that you restart your computer after setting the environment variable. You must also have a copy of my [engine](https://github.com/aahernandez/Engine) to build it, and the engine and facial animation project must both have the same parent folder like so:

|—parent folder  
&ensp;&ensp;&ensp;&ensp;|—Facial Animation  
&ensp;&ensp;&ensp;&ensp;|—Engine  