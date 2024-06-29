#pragma once
#include <YYToolkit/shared.hpp>
#include "ModuleMain.h"

void InstanceCreateLayerBefore(RValue* Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args);
void InstanceCreateLayerAfter(RValue* Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args);
void SpriteDeleteBefore(RValue* Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args);
void InstanceExistsBefore(RValue* Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args);
void DsMapFindValueBefore(RValue* Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args);