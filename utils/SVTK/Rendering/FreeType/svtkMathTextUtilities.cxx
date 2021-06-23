/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMathTextUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkMathTextUtilities.h"

#include "svtkDebugLeaks.h" // Must be included before any singletons
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"
#include "svtkViewport.h"
#include "svtkWindow.h"

#include <algorithm>

//----------------------------------------------------------------------------
// The singleton, and the singleton cleanup
svtkMathTextUtilities* svtkMathTextUtilities::Instance = nullptr;
svtkMathTextUtilitiesCleanup svtkMathTextUtilities::Cleanup;

//----------------------------------------------------------------------------
// Create the singleton cleanup
// Register our singleton cleanup callback against the FTLibrary so that
// it might be called before the FTLibrary singleton is destroyed.
svtkMathTextUtilitiesCleanup::svtkMathTextUtilitiesCleanup() = default;

//----------------------------------------------------------------------------
// Delete the singleton cleanup
svtkMathTextUtilitiesCleanup::~svtkMathTextUtilitiesCleanup()
{
  svtkMathTextUtilities::SetInstance(nullptr);
}

//----------------------------------------------------------------------------
svtkMathTextUtilities* svtkMathTextUtilities::GetInstance()
{
  if (!svtkMathTextUtilities::Instance)
  {
    svtkMathTextUtilities::Instance =
      static_cast<svtkMathTextUtilities*>(svtkObjectFactory::CreateInstance("svtkMathTextUtilities"));
  }

  return svtkMathTextUtilities::Instance;
}

//----------------------------------------------------------------------------
void svtkMathTextUtilities::SetInstance(svtkMathTextUtilities* instance)
{
  if (svtkMathTextUtilities::Instance == instance)
  {
    return;
  }

  if (svtkMathTextUtilities::Instance)
  {
    svtkMathTextUtilities::Instance->Delete();
  }

  svtkMathTextUtilities::Instance = instance;

  // User will call ->Delete() after setting instance
  if (instance)
  {
    instance->Register(nullptr);
  }
}

//----------------------------------------------------------------------------
int svtkMathTextUtilities::GetConstrainedFontSize(
  const char* str, svtkTextProperty* tprop, int targetWidth, int targetHeight, int dpi)
{
  if (str == nullptr || str[0] == '\0' || targetWidth == 0 || targetHeight == 0 || tprop == nullptr)
  {
    return 0;
  }

  // Use the current font size as a first guess
  int bbox[4];
  double fontSize = tprop->GetFontSize();
  if (!this->GetBoundingBox(tprop, str, dpi, bbox))
  {
    return -1;
  }
  int width = bbox[1] - bbox[0];
  int height = bbox[3] - bbox[2];

  // Bad assumption but better than nothing -- assume the bbox grows linearly
  // with the font size:
  if (width != 0 && height != 0)
  {
    fontSize *= std::min(static_cast<double>(targetWidth) / static_cast<double>(width),
      static_cast<double>(targetHeight) / static_cast<double>(height));
    tprop->SetFontSize(static_cast<int>(fontSize));
    if (!this->GetBoundingBox(tprop, str, dpi, bbox))
    {
      return -1;
    }
    width = bbox[1] - bbox[0];
    height = bbox[3] - bbox[2];
  }

  // Now just step up/down until the bbox matches the target.
  while ((width < targetWidth || height < targetHeight) && fontSize < 200)
  {
    fontSize += 1.;
    tprop->SetFontSize(fontSize);
    if (!this->GetBoundingBox(tprop, str, dpi, bbox))
    {
      return -1;
    }
    width = bbox[1] - bbox[0];
    height = bbox[3] - bbox[2];
  }

  while ((width > targetWidth || height > targetHeight) && fontSize > 0)
  {
    fontSize -= 1.;
    tprop->SetFontSize(fontSize);
    if (!this->GetBoundingBox(tprop, str, dpi, bbox))
    {
      return -1;
    }
    width = bbox[1] - bbox[0];
    height = bbox[3] - bbox[2];
  }

  return fontSize;
}

//----------------------------------------------------------------------------
svtkMathTextUtilities* svtkMathTextUtilities::New()
{
  svtkMathTextUtilities* ret = svtkMathTextUtilities::GetInstance();
  if (ret)
  {
    ret->Register(nullptr);
  }
  return ret;
}

//----------------------------------------------------------------------------
svtkMathTextUtilities::svtkMathTextUtilities() = default;

//----------------------------------------------------------------------------
svtkMathTextUtilities::~svtkMathTextUtilities() = default;

//----------------------------------------------------------------------------
void svtkMathTextUtilities::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Instance: " << this->Instance << endl;
}
