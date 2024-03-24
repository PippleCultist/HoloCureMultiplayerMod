#pragma once
#include <YYToolkit/shared.hpp>
#include <CallbackManager/CallbackManagerInterface.h>
#include "ModuleMain.h"

inline RValue getInstanceVariable(RValue instance, VariableNames variableName)
{
	RValue result;
	RValue inputArgs[2];
	inputArgs[0] = instance;
	inputArgs[1] = GMLVarIndexMapGMLHash[variableName];
	origStructGetFromHashFunc(&result, globalInstance, nullptr, 2, inputArgs);
	return result;
//	return g_ModuleInterface->CallBuiltin("struct_get_from_hash", { instance, GMLVarIndexMapGMLHash[variableName]});
}

inline RValue getInstanceVariable(CInstance* instance, VariableNames variableName)
{
	RValue result;
	RValue inputArgs[2];
	inputArgs[0] = instance;
	inputArgs[1] = GMLVarIndexMapGMLHash[variableName];
	origStructGetFromHashFunc(&result, globalInstance, nullptr, 2, inputArgs);
	return result;
//	return g_ModuleInterface->CallBuiltin("struct_get_from_hash", { instance, GMLVarIndexMapGMLHash[variableName] });
}

inline void setInstanceVariable(RValue instance, VariableNames variableName, RValue setValue)
{
	RValue result;
	RValue inputArgs[3];
	inputArgs[0] = instance;
	inputArgs[1] = GMLVarIndexMapGMLHash[variableName];
	inputArgs[2] = setValue;
	origStructSetFromHashFunc(&result, globalInstance, nullptr, 3, inputArgs);
//	g_ModuleInterface->CallBuiltin("struct_set_from_hash", { instance, GMLVarIndexMapGMLHash[variableName], setValue });
}

inline void setInstanceVariable(CInstance* instance, VariableNames variableName, RValue setValue)
{
	RValue result;
	RValue inputArgs[3];
	inputArgs[0] = instance;
	inputArgs[1] = GMLVarIndexMapGMLHash[variableName];
	inputArgs[2] = setValue;
	origStructSetFromHashFunc(&result, globalInstance, nullptr, 3, inputArgs);
//	g_ModuleInterface->CallBuiltin("struct_set_from_hash", { instance, GMLVarIndexMapGMLHash[variableName], setValue });
}