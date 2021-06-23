/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLGL2PSHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLGL2PSHelper.h"

#include "svtkObjectFactory.h"

// Static allocation:
svtkOpenGLGL2PSHelper* svtkOpenGLGL2PSHelper::Instance = nullptr;

//------------------------------------------------------------------------------
svtkAbstractObjectFactoryNewMacro(svtkOpenGLGL2PSHelper);

//------------------------------------------------------------------------------
void svtkOpenGLGL2PSHelper::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
svtkOpenGLGL2PSHelper* svtkOpenGLGL2PSHelper::GetInstance()
{
  return svtkOpenGLGL2PSHelper::Instance;
}

//------------------------------------------------------------------------------
void svtkOpenGLGL2PSHelper::SetInstance(svtkOpenGLGL2PSHelper* obj)
{
  if (obj == svtkOpenGLGL2PSHelper::Instance)
  {
    return;
  }

  if (svtkOpenGLGL2PSHelper::Instance)
  {
    svtkOpenGLGL2PSHelper::Instance->Delete();
  }

  if (obj)
  {
    obj->Register(nullptr);
  }

  svtkOpenGLGL2PSHelper::Instance = obj;
}

//------------------------------------------------------------------------------
svtkOpenGLGL2PSHelper::svtkOpenGLGL2PSHelper()
  : RenderWindow(nullptr)
  , ActiveState(Inactive)
  , TextAsPath(false)
  , PointSize(1.f)
  , LineWidth(1.f)
  , PointSizeFactor(5.f / 7.f)
  , LineWidthFactor(5.f / 7.f)
  , LineStipple(0xffff)
{
}

//------------------------------------------------------------------------------
svtkOpenGLGL2PSHelper::~svtkOpenGLGL2PSHelper() = default;
