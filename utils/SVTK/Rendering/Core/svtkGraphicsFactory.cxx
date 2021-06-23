/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphicsFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkObjectFactory.h"

#include "svtkDebugLeaks.h"
#include "svtkGraphicsFactory.h"
#include "svtkToolkits.h"

#include "svtkCriticalSection.h"

#include <cstdlib>

static svtkSimpleCriticalSection svtkUseMesaClassesCriticalSection;
static svtkSimpleCriticalSection svtkOffScreenOnlyModeCriticalSection;
int svtkGraphicsFactory::UseMesaClasses = 0;

#ifdef SVTK_USE_OFFSCREEN
int svtkGraphicsFactory::OffScreenOnlyMode = 1;
#else
int svtkGraphicsFactory::OffScreenOnlyMode = 0;
#endif

svtkStandardNewMacro(svtkGraphicsFactory);

const char* svtkGraphicsFactory::GetRenderLibrary()
{
  const char* temp;

  // first check the environment variable
  temp = getenv("SVTK_RENDERER");

  // Backward compatibility
  if (temp)
  {
    if (!strcmp("oglr", temp))
    {
      temp = "OpenGL";
    }
    else if (!strcmp("woglr", temp))
    {
      temp = "Win32OpenGL";
    }
    else if (strcmp("OpenGL", temp) && strcmp("Win32OpenGL", temp))
    {
      svtkGenericWarningMacro(<< "SVTK_RENDERER set to unsupported type:" << temp);
      temp = nullptr;
    }
  }

  // if nothing is set then work down the list of possible renderers
  if (!temp)
  {
#if defined(SVTK_DISPLAY_X11_OGL) || defined(SVTK_OPENGL_HAS_OSMESA)
    temp = "OpenGL";
#endif
#ifdef SVTK_DISPLAY_WIN32_OGL
    temp = "Win32OpenGL";
#endif
#ifdef SVTK_DISPLAY_COCOA
    temp = "CocoaOpenGL";
#endif
  }

  return temp;
}

svtkObject* svtkGraphicsFactory::CreateInstance(const char* svtkclassname)
{
  // first check the object factory
  svtkObject* ret = svtkObjectFactory::CreateInstance(svtkclassname);
  if (ret)
  {
    return ret;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkGraphicsFactory::SetUseMesaClasses(int use)
{
  svtkUseMesaClassesCriticalSection.Lock();
  svtkGraphicsFactory::UseMesaClasses = use;
  svtkUseMesaClassesCriticalSection.Unlock();
}

//----------------------------------------------------------------------------
int svtkGraphicsFactory::GetUseMesaClasses()
{
  return svtkGraphicsFactory::UseMesaClasses;
}

//----------------------------------------------------------------------------
void svtkGraphicsFactory::SetOffScreenOnlyMode(int use)
{
  svtkOffScreenOnlyModeCriticalSection.Lock();
  svtkGraphicsFactory::OffScreenOnlyMode = use;
  svtkOffScreenOnlyModeCriticalSection.Unlock();
}

//----------------------------------------------------------------------------
int svtkGraphicsFactory::GetOffScreenOnlyMode()
{
  return svtkGraphicsFactory::OffScreenOnlyMode;
}

//----------------------------------------------------------------------------
void svtkGraphicsFactory::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
