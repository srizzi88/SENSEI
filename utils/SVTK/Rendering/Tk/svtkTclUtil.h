/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTclUtil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkTclUtil_h
#define svtkTclUtil_h

#include "svtkCommand.h"
#include "svtkObject.h"
#include "svtkTcl.h"

#ifdef _WIN32
#define SVTKTCL_EXPORT __declspec(dllexport)
#else
#define SVTKTCL_EXPORT
#endif

extern SVTKTCL_EXPORT void svtkTclUpdateCommand(Tcl_Interp* interp, char* name, svtkObject* obj);

extern SVTKTCL_EXPORT void svtkTclDeleteObjectFromHash(
  svtkObject*, unsigned long eventId, void*, void*);
extern SVTKTCL_EXPORT void svtkTclGenericDeleteObject(ClientData cd);

extern SVTKTCL_EXPORT void svtkTclGetObjectFromPointer(
  Tcl_Interp* interp, void* temp, const char* targetType);

extern SVTKTCL_EXPORT void* svtkTclGetPointerFromObject(
  const char* name, const char* result_type, Tcl_Interp* interp, int& error);

extern SVTKTCL_EXPORT void svtkTclVoidFunc(void*);
extern SVTKTCL_EXPORT void svtkTclVoidFuncArgDelete(void*);
extern SVTKTCL_EXPORT void svtkTclListInstances(Tcl_Interp* interp, ClientData arg);
extern SVTKTCL_EXPORT int svtkTclInDelete(Tcl_Interp* interp);

extern SVTKTCL_EXPORT int svtkTclNewInstanceCommand(
  ClientData cd, Tcl_Interp* interp, int argc, char* argv[]);
extern SVTKTCL_EXPORT void svtkTclDeleteCommandStruct(ClientData cd);
extern SVTKTCL_EXPORT void svtkTclCreateNew(Tcl_Interp* interp, const char* cname,
  ClientData (*NewCommand)(),
  int (*CommandFunction)(ClientData cd, Tcl_Interp* interp, int argc, char* argv[]));

class svtkTclCommand : public svtkCommand
{
public:
  static svtkTclCommand* New() { return new svtkTclCommand; }

  void SetStringCommand(const char* arg);
  void SetInterp(Tcl_Interp* interp) { this->Interp = interp; }

  void Execute(svtkObject*, unsigned long, void*);

  char* StringCommand;
  Tcl_Interp* Interp;

protected:
  svtkTclCommand();
  ~svtkTclCommand();
};

typedef struct _svtkTclVoidFuncArg
{
  Tcl_Interp* interp;
  char* command;
} svtkTclVoidFuncArg;

struct svtkTclCommandArgStruct
{
  void* Pointer;
  Tcl_Interp* Interp;
  unsigned long Tag;
};

struct svtkTclCommandStruct
{
  ClientData (*NewCommand)();
  int (*CommandFunction)(ClientData cd, Tcl_Interp* interp, int argc, char* argv[]);
};

struct svtkTclInterpStruct
{
  Tcl_HashTable InstanceLookup;
  Tcl_HashTable PointerLookup;
  Tcl_HashTable CommandLookup;

  int Number;
  int DebugOn;
  int InDelete;
  int DeleteExistingObjectOnNew;
};

extern SVTKTCL_EXPORT void svtkTclApplicationInitExecutable(int argc, const char* const argv[]);
extern SVTKTCL_EXPORT void svtkTclApplicationInitTclTk(
  Tcl_Interp* interp, const char* const relative_dirs[]);

#endif
// SVTK-HeaderTest-Exclude: svtkTclUtil.h
