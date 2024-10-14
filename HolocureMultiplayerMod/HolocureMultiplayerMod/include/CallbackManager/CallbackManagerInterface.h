#pragma once

#include <YYToolkit/shared.hpp>
using namespace Aurie;
using namespace YYTK;

typedef std::tuple<CInstance*, CInstance*, CCode*, int, RValue*> CodeEventArgs;
typedef void (*CodeEvent)(CodeEventArgs&);
using TRoutine = void(__cdecl*)(RValue* Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args);

struct CallbackManagerInterface : AurieInterfaceBase
{
	virtual AurieStatus Create();
	virtual void Destroy();
	virtual void QueryVersion(
		OUT short& Major,
		OUT short& Minor,
		OUT short& Patch
	);

	/*
	* Call this to register a routine that will run before the code event happens and another routine that will run after.
	* This will run the code event after all before routines run and before any after routines run.
	*/
	virtual AurieStatus RegisterCodeEventCallback(
		IN const std::string& ModName,
		IN const std::string& CodeEventName,
		IN CodeEvent BeforeCodeEventRoutine,
		IN CodeEvent AfterCodeEventRoutine
	);

	/*
	* Call this to register a routine that will run before the code event happens and another routine that will run after.
	* This will run the code event after all before routines run and before any after routines run.
	*/
	virtual AurieStatus RegisterCodeEventCallback(
		IN const std::string& ModName,
		IN const std::string& CodeEventName,
		IN CodeEvent BeforeCodeEventRoutine,
		IN CodeEvent AfterCodeEventRoutine,
		OUT int& CodeEventIndex
	);

	/*
	* Call this to register a routine that will run before the script function happens and another routine that will run after.
	* This will run the script function after all before routines run and before any after routines run.
	*/
	virtual AurieStatus RegisterScriptFunctionCallback(
		IN const std::string& ModName,
		IN const std::string& ScriptFunctionName,
		IN PFUNC_YYGMLScript BeforeScriptFunctionRoutine,
		IN PFUNC_YYGMLScript AfterScriptFunctionRoutine,
		OUT PFUNC_YYGMLScript* OriginalScriptFunctionRoutine
	);

	/*
	* Call this to register a routine that will run before the script function happens and another routine that will run after.
	* This will run the script function after all before routines run and before any after routines run.
	*/
	virtual AurieStatus RegisterScriptFunctionCallback(
		IN const std::string& ModName,
		IN const std::string& ScriptFunctionName,
		IN PFUNC_YYGMLScript BeforeScriptFunctionRoutine,
		IN PFUNC_YYGMLScript AfterScriptFunctionRoutine,
		OUT PFUNC_YYGMLScript* OriginalScriptFunctionRoutine,
		OUT int& ScriptFunctionIndex
	);

	/*
	* Call this to register a routine that will run before the builtin function happens and another routine that will run after.
	* This will run the script function after all before routines run and before any after routines run.
	*/
	virtual AurieStatus RegisterBuiltinFunctionCallback(
		IN const std::string& ModName,
		IN const std::string& BuiltinFunctionName,
		IN TRoutine BeforeBuiltinFunctionRoutine,
		IN TRoutine AfterBuiltinFunctionRoutine,
		OUT TRoutine* OriginalBuiltinFunctionRoutine
	);

	/*
	* Call this to register a routine that will run before the builtin function happens and another routine that will run after.
	* This will run the script function after all before routines run and before any after routines run.
	*/
	virtual AurieStatus RegisterBuiltinFunctionCallback(
		IN const std::string& ModName,
		IN const std::string& BuiltinFunctionName,
		IN TRoutine BeforeBuiltinFunctionRoutine,
		IN TRoutine AfterBuiltinFunctionRoutine,
		OUT TRoutine* OriginalBuiltinFunctionRoutine,
		OUT int& BuiltinFunctionIndex
	);

	/*
	* Call this to get the current Code Event index/name
	*/
	virtual AurieStatus GetCurrentCodeEventInfo(
		IN const std::string& ModName,
		OUT const char** CodeEventName,
		OUT int& CodeEventIndex
	);

	/*
	* Call this to get the current Script Function index/name
	*/
	virtual AurieStatus GetCurrentScriptFunctionInfo(
		IN const std::string& ModName,
		OUT const char** ScriptFunctionName,
		OUT int& ScriptFunctionIndex
	);

	/*
	* Call this to get the current Builtin Function index/name
	*/
	virtual AurieStatus GetCurrentBuiltinFunctionInfo(
		IN const std::string& ModName,
		OUT const char** BuiltinFunctionName,
		OUT int& BuiltinFunctionIndex
	);

	/*
	* Call this to log to the Logs file in the game's directory
	*/
	virtual AurieStatus LogToFile(
		IN const std::string& ModName,
		IN const char* LogFormat,
		...
	);

	/*
	* Will only have an effect inside the before routine that was registered with the callback.
	* Tells the callback manager that a mod wants the original function to be called.
	* WARNING: WILL HAVE UNDEFINED BEHAVIOR IF BOTH CALL AND CANCEL OCCUR
	*/
	virtual void CallOriginalFunction();

	/*
	* Will only have an effect inside the before routine that was registered with the callback.
	* Tells the callback manager that a mod wants the original function to be cancelled.
	* WARNING: WILL HAVE UNDEFINED BEHAVIOR IF BOTH CALL AND CANCEL OCCUR
	*/
	virtual void CancelOriginalFunction();
};