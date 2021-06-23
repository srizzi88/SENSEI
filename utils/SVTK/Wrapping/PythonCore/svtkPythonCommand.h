/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPythonCommand.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkPythonCommand_h
#define svtkPythonCommand_h

#include "svtkCommand.h"
#include "svtkPython.h"
#include "svtkWrappingPythonCoreModule.h" // For export macro

// To allow Python to use the svtkCommand features
class SVTKWRAPPINGPYTHONCORE_EXPORT svtkPythonCommand : public svtkCommand
{
public:
  svtkTypeMacro(svtkPythonCommand, svtkCommand);

  static svtkPythonCommand* New() { return new svtkPythonCommand; }

  void SetObject(PyObject* o);
  void SetThreadState(PyThreadState* ts);
  void Execute(svtkObject* ptr, unsigned long eventtype, void* callData) override;

  PyObject* obj;
  PyThreadState* ThreadState;

protected:
  svtkPythonCommand();
  ~svtkPythonCommand() override;
};

#endif
// SVTK-HeaderTest-Exclude: svtkPythonCommand.h
