#pragma once
#include <YYToolkit/YYTK_Shared.hpp>
#include "ModuleMain.h"

void InstanceCreateLayerBefore(RValue& Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args);
void InstanceCreateLayerAfter(RValue& Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args);
void SpriteDeleteBefore(RValue& Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args);
void InstanceExistsBefore(RValue& Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args);
void DsMapFindValueBefore(RValue& Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args);
void InstanceCreateDepthAfter(RValue& Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args);
void InstanceFindBefore(RValue& Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args);