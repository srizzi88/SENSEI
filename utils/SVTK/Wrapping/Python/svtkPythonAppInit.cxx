/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPythonAppInit.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/* Minimal main program -- everything is loaded from the library */

#include "svtkPython.h"
#include "svtkPythonCompatibility.h"

#ifdef SVTK_COMPILED_USING_MPI
#include "svtkMPIController.h"
#include <svtk_mpi.h>
#endif // SVTK_COMPILED_USING_MPI

#include "svtkOutputWindow.h"
#include "svtkPythonInterpreter.h"
#include "svtkVersion.h"
#include "svtkpythonmodules.h"
#include <sys/stat.h>
#include <svtksys/SystemTools.hxx>

#include <string>

#ifdef SVTK_COMPILED_USING_MPI
class svtkMPICleanup
{
public:
  svtkMPICleanup() { this->Controller = nullptr; }
  void Initialize(int* argc, char*** argv)
  {
    MPI_Init(argc, argv);
    this->Controller = svtkMPIController::New();
    this->Controller->Initialize(argc, argv, 1);
    svtkMultiProcessController::SetGlobalController(this->Controller);
  }
  void Cleanup()
  {
    if (this->Controller)
    {
      this->Controller->Finalize();
      this->Controller->Delete();
      this->Controller = nullptr;
      svtkMultiProcessController::SetGlobalController(nullptr);
    }
  }
  ~svtkMPICleanup() { this->Cleanup(); }

private:
  svtkMPIController* Controller;
};

static svtkMPICleanup SVTKMPICleanup;
// AtExitCallback is needed to finalize the MPI controller if the python script
// calls sys.exit() directly.
static void AtExitCallback()
{
  SVTKMPICleanup.Cleanup();
}
#endif // SVTK_COMPILED_USING_MPI

int main(int argc, char** argv)
{
#ifdef SVTK_COMPILED_USING_MPI
  SVTKMPICleanup.Initialize(&argc, &argv);
  Py_AtExit(::AtExitCallback);
#endif // SVTK_COMPILED_USING_MPI

  /**
   * This function is generated and exposed in svtkpythonmodules.h.
   * This registers any Python modules for SVTK for static builds.
   */
  svtkpythonmodules_load();

  // Setup the output window to be svtkOutputWindow, rather than platform
  // specific one. This avoids creating svtkWin32OutputWindow on Windows, for
  // example, which puts all Python errors in a window rather than the terminal
  // as one would expect.
  auto opwindow = svtkOutputWindow::New();
  svtkOutputWindow::SetInstance(opwindow);
  opwindow->Delete();

  // For static builds, help with finding `svtk` packages.
  std::string fullpath;
  std::string error;
  if (argc > 0 && svtksys::SystemTools::FindProgramPath(argv[0], fullpath, error))
  {
    const auto dir = svtksys::SystemTools::GetProgramPath(fullpath);
#if defined(SVTK_BUILD_SHARED_LIBS)
    svtkPythonInterpreter::PrependPythonPath(dir.c_str(), "svtkmodules/__init__.py");
#else
    // since there may be other packages not zipped (e.g. mpi4py), we added path to _svtk.zip
    // to the search path as well.
    svtkPythonInterpreter::PrependPythonPath(dir.c_str(), "_svtk.zip", /*add_landmark=*/false);
    svtkPythonInterpreter::PrependPythonPath(dir.c_str(), "_svtk.zip", /*add_landmark=*/true);
#endif
  }

  return svtkPythonInterpreter::PyMain(argc, argv);
}
