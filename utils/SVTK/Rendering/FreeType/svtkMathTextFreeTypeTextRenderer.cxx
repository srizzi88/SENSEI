/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMathTextFreeTypeTextRenderer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkMathTextFreeTypeTextRenderer.h"

#include "svtkFreeTypeTools.h"
#include "svtkMathTextUtilities.h"
#include "svtkObjectFactory.h"
#include "svtkStdString.h"
#include "svtkTextProperty.h"
#include "svtkUnicodeString.h"

//------------------------------------------------------------------------------
svtkObjectFactoryNewMacro(svtkMathTextFreeTypeTextRenderer);

//------------------------------------------------------------------------------
void svtkMathTextFreeTypeTextRenderer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->FreeTypeTools)
  {
    os << indent << "FreeTypeTools:" << endl;
    this->FreeTypeTools->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "FreeTypeTools: (nullptr)" << endl;
  }

  if (this->MathTextUtilities)
  {
    os << indent << "MathTextUtilities:" << endl;
    this->MathTextUtilities->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "MathTextUtilities: (nullptr)" << endl;
  }
}

//------------------------------------------------------------------------------
bool svtkMathTextFreeTypeTextRenderer::FreeTypeIsSupported()
{
  return this->FreeTypeTools != nullptr;
}

//------------------------------------------------------------------------------
bool svtkMathTextFreeTypeTextRenderer::MathTextIsSupported()
{
  return this->MathTextUtilities != nullptr && this->MathTextUtilities->IsAvailable();
}

//------------------------------------------------------------------------------
bool svtkMathTextFreeTypeTextRenderer::GetBoundingBoxInternal(
  svtkTextProperty* tprop, const svtkStdString& str, int bbox[4], int dpi, int backend)
{
  if (!bbox || !tprop)
  {
    svtkErrorMacro("No bounding box container and/or text property supplied!");
    return false;
  }

  memset(bbox, 0, 4 * sizeof(int));
  if (str.empty())
  {
    return true;
  }

  if (static_cast<Backend>(backend) == Default)
  {
    backend = this->DefaultBackend;
  }

  if (static_cast<Backend>(backend) == Detect)
  {
    backend = static_cast<int>(this->DetectBackend(str));
  }

  switch (static_cast<Backend>(backend))
  {
    case MathText:
      if (this->MathTextIsSupported())
      {
        if (this->MathTextUtilities->GetBoundingBox(tprop, str.c_str(), dpi, bbox))
        {
          return true;
        }
      }
      svtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      SVTK_FALLTHROUGH;
    case FreeType:
    {
      svtkStdString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      // Interpret string as UTF-8, use the UTF-16 GetBoundingBox overload:
      return this->FreeTypeTools->GetBoundingBox(
        tprop, svtkUnicodeString::from_utf8(cleanString), dpi, bbox);
    }
    case Default:
    case UserBackend:
    default:
      svtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      svtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
  }
  return false;
}

//------------------------------------------------------------------------------
bool svtkMathTextFreeTypeTextRenderer::GetBoundingBoxInternal(
  svtkTextProperty* tprop, const svtkUnicodeString& str, int bbox[], int dpi, int backend)
{
  if (!bbox || !tprop)
  {
    svtkErrorMacro("No bounding box container and/or text property supplied!");
    return false;
  }

  memset(bbox, 0, 4 * sizeof(int));
  if (str.empty())
  {
    return true;
  }

  if (static_cast<Backend>(backend) == Default)
  {
    backend = this->DefaultBackend;
  }

  if (static_cast<Backend>(backend) == Detect)
  {
    backend = static_cast<int>(this->DetectBackend(str));
  }

  switch (static_cast<Backend>(backend))
  {
    case MathText:
      if (this->MathTextIsSupported())
      {
        svtkDebugMacro("Converting UTF16 to UTF8 for MathText rendering.");
        if (this->MathTextUtilities->GetBoundingBox(tprop, str.utf8_str(), dpi, bbox))
        {
          return true;
        }
      }
      svtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      SVTK_FALLTHROUGH;
    case FreeType:
    {
      svtkUnicodeString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->GetBoundingBox(tprop, cleanString, dpi, bbox);
    }
    case Default:
    case UserBackend:
    default:
      svtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      svtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
  }
  return false;
}

//------------------------------------------------------------------------------
bool svtkMathTextFreeTypeTextRenderer::GetMetricsInternal(svtkTextProperty* tprop,
  const svtkStdString& str, svtkTextRenderer::Metrics& metrics, int dpi, int backend)
{
  if (!tprop)
  {
    svtkErrorMacro("No text property supplied!");
    return false;
  }

  metrics = Metrics();
  if (str.empty())
  {
    return true;
  }

  if (static_cast<Backend>(backend) == Default)
  {
    backend = this->DefaultBackend;
  }

  if (static_cast<Backend>(backend) == Detect)
  {
    backend = static_cast<int>(this->DetectBackend(str));
  }

  switch (static_cast<Backend>(backend))
  {
    case MathText:
      if (this->MathTextIsSupported())
      {
        if (this->MathTextUtilities->GetMetrics(tprop, str.c_str(), dpi, metrics))
        {
          return true;
        }
      }
      svtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      SVTK_FALLTHROUGH;
    case FreeType:
    {
      svtkStdString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      // Interpret string as UTF-8, use the UTF-16 GetBoundingBox overload:
      return this->FreeTypeTools->GetMetrics(
        tprop, svtkUnicodeString::from_utf8(cleanString), dpi, metrics);
    }
    case Default:
    case UserBackend:
    default:
      svtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      svtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
  }
  return false;
}

//------------------------------------------------------------------------------
bool svtkMathTextFreeTypeTextRenderer::GetMetricsInternal(svtkTextProperty* tprop,
  const svtkUnicodeString& str, svtkTextRenderer::Metrics& metrics, int dpi, int backend)
{
  if (!tprop)
  {
    svtkErrorMacro("No text property supplied!");
    return false;
  }

  metrics = Metrics();
  if (str.empty())
  {
    return true;
  }

  if (static_cast<Backend>(backend) == Default)
  {
    backend = this->DefaultBackend;
  }

  if (static_cast<Backend>(backend) == Detect)
  {
    backend = static_cast<int>(this->DetectBackend(str));
  }

  switch (static_cast<Backend>(backend))
  {
    case MathText:
      if (this->MathTextIsSupported())
      {
        svtkDebugMacro("Converting UTF16 to UTF8 for MathText rendering.");
        if (this->MathTextUtilities->GetMetrics(tprop, str.utf8_str(), dpi, metrics))
        {
          return true;
        }
      }
      svtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      SVTK_FALLTHROUGH;
    case FreeType:
    {
      svtkUnicodeString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->GetMetrics(tprop, cleanString, dpi, metrics);
    }
    case Default:
    case UserBackend:
    default:
      svtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      svtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
  }
  return false;
}

//------------------------------------------------------------------------------
bool svtkMathTextFreeTypeTextRenderer::RenderStringInternal(svtkTextProperty* tprop,
  const svtkStdString& str, svtkImageData* data, int textDims[2], int dpi, int backend)
{
  if (!data || !tprop)
  {
    svtkErrorMacro("No image container and/or text property supplied!");
    return false;
  }

  if (static_cast<Backend>(backend) == Default)
  {
    backend = this->DefaultBackend;
  }

  if (static_cast<Backend>(backend) == Detect)
  {
    backend = static_cast<int>(this->DetectBackend(str));
  }

  switch (static_cast<Backend>(backend))
  {
    case MathText:
      if (this->MathTextIsSupported())
      {
        if (this->MathTextUtilities->RenderString(str.c_str(), data, tprop, dpi, textDims))
        {
          return true;
        }
      }
      svtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      SVTK_FALLTHROUGH;
    case FreeType:
    {
      svtkStdString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      // Interpret string as UTF-8, use the UTF-16 RenderString overload:
      return this->FreeTypeTools->RenderString(
        tprop, svtkUnicodeString::from_utf8(cleanString), dpi, data, textDims);
    }
    case Default:
    case UserBackend:
    default:
      svtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      svtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
  }
  return false;
}

//------------------------------------------------------------------------------
bool svtkMathTextFreeTypeTextRenderer::RenderStringInternal(svtkTextProperty* tprop,
  const svtkUnicodeString& str, svtkImageData* data, int textDims[], int dpi, int backend)
{
  if (!data || !tprop)
  {
    svtkErrorMacro("No image container and/or text property supplied!");
    return false;
  }

  if (static_cast<Backend>(backend) == Default)
  {
    backend = this->DefaultBackend;
  }

  if (static_cast<Backend>(backend) == Detect)
  {
    backend = static_cast<int>(this->DetectBackend(str));
  }

  switch (static_cast<Backend>(backend))
  {
    case MathText:
      if (this->MathTextIsSupported())
      {
        svtkDebugMacro("Converting UTF16 to UTF8 for MathText rendering.");
        if (this->MathTextUtilities->RenderString(str.utf8_str(), data, tprop, dpi, textDims))
        {
          return true;
        }
      }
      svtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      SVTK_FALLTHROUGH;
    case FreeType:
    {
      svtkUnicodeString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->RenderString(tprop, cleanString, dpi, data, textDims);
    }
    case Default:
    case UserBackend:
    default:
      svtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      svtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
  }
  return false;
}

//------------------------------------------------------------------------------
int svtkMathTextFreeTypeTextRenderer::GetConstrainedFontSizeInternal(const svtkStdString& str,
  svtkTextProperty* tprop, int targetWidth, int targetHeight, int dpi, int backend)
{
  if (!tprop)
  {
    svtkErrorMacro("No text property supplied!");
    return false;
  }

  if (static_cast<Backend>(backend) == Default)
  {
    backend = this->DefaultBackend;
  }

  if (static_cast<Backend>(backend) == Detect)
  {
    backend = static_cast<int>(this->DetectBackend(str));
  }

  switch (static_cast<Backend>(backend))
  {
    case MathText:
      if (this->MathTextIsSupported())
      {
        if (this->MathTextUtilities->GetConstrainedFontSize(
              str.c_str(), tprop, targetWidth, targetHeight, dpi) != -1)
        {
          return tprop->GetFontSize();
        }
      }
      svtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      SVTK_FALLTHROUGH;
    case FreeType:
    {
      svtkStdString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->GetConstrainedFontSize(
        cleanString, tprop, dpi, targetWidth, targetHeight);
    }
    case Default:
    case UserBackend:
    default:
      svtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      svtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
  }
  return false;
}

//------------------------------------------------------------------------------
int svtkMathTextFreeTypeTextRenderer::GetConstrainedFontSizeInternal(const svtkUnicodeString& str,
  svtkTextProperty* tprop, int targetWidth, int targetHeight, int dpi, int backend)
{
  if (!tprop)
  {
    svtkErrorMacro("No text property supplied!");
    return false;
  }

  if (static_cast<Backend>(backend) == Default)
  {
    backend = this->DefaultBackend;
  }

  if (static_cast<Backend>(backend) == Detect)
  {
    backend = static_cast<int>(this->DetectBackend(str));
  }

  switch (static_cast<Backend>(backend))
  {
    case MathText:
      if (this->MathTextIsSupported())
      {
        svtkDebugMacro("Converting UTF16 to UTF8 for MathText rendering.");
        if (this->MathTextUtilities->GetConstrainedFontSize(
              str.utf8_str(), tprop, targetWidth, targetHeight, dpi) != -1)
        {
          return tprop->GetFontSize();
        }
      }
      svtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      SVTK_FALLTHROUGH;
    case FreeType:
    {
      svtkUnicodeString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->GetConstrainedFontSize(
        cleanString, tprop, dpi, targetWidth, targetHeight);
    }
    case Default:
    case UserBackend:
    default:
      svtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      svtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
  }
  return false;
}

//------------------------------------------------------------------------------
bool svtkMathTextFreeTypeTextRenderer::StringToPathInternal(
  svtkTextProperty* tprop, const svtkStdString& str, svtkPath* path, int dpi, int backend)
{
  if (!path || !tprop)
  {
    svtkErrorMacro("No path container and/or text property supplied!");
    return false;
  }

  if (static_cast<Backend>(backend) == Default)
  {
    backend = this->DefaultBackend;
  }

  if (static_cast<Backend>(backend) == Detect)
  {
    backend = static_cast<int>(this->DetectBackend(str));
  }

  switch (static_cast<Backend>(backend))
  {
    case MathText:
      if (this->MathTextIsSupported())
      {
        if (this->MathTextUtilities->StringToPath(str.c_str(), path, tprop, dpi))
        {
          return true;
        }
      }
      svtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      SVTK_FALLTHROUGH;
    case FreeType:
    {
      svtkStdString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->StringToPath(tprop, str, dpi, path);
    }
    case Default:
    case UserBackend:
    default:
      svtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      svtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
  }
  return false;
}

//------------------------------------------------------------------------------
bool svtkMathTextFreeTypeTextRenderer::StringToPathInternal(
  svtkTextProperty* tprop, const svtkUnicodeString& str, svtkPath* path, int dpi, int backend)
{
  if (!path || !tprop)
  {
    svtkErrorMacro("No path container and/or text property supplied!");
    return false;
  }

  if (static_cast<Backend>(backend) == Default)
  {
    backend = this->DefaultBackend;
  }

  if (static_cast<Backend>(backend) == Detect)
  {
    backend = static_cast<int>(this->DetectBackend(str));
  }

  switch (static_cast<Backend>(backend))
  {
    case MathText:
      if (this->MathTextIsSupported())
      {
        svtkDebugMacro("Converting UTF16 to UTF8 for MathText rendering.");
        if (this->MathTextUtilities->StringToPath(str.utf8_str(), path, tprop, dpi))
        {
          return true;
        }
      }
      svtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      SVTK_FALLTHROUGH;
    case FreeType:
    {
      svtkUnicodeString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->StringToPath(tprop, str, dpi, path);
    }
    case Default:
    case UserBackend:
    default:
      svtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      svtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
  }
  return false;
}

//------------------------------------------------------------------------------
void svtkMathTextFreeTypeTextRenderer::SetScaleToPowerOfTwoInternal(bool scale)
{
  if (this->FreeTypeTools)
  {
    this->FreeTypeTools->SetScaleToPowerTwo(scale);
  }
  if (this->MathTextUtilities)
  {
    this->MathTextUtilities->SetScaleToPowerOfTwo(scale);
  }
}

//------------------------------------------------------------------------------
svtkMathTextFreeTypeTextRenderer::svtkMathTextFreeTypeTextRenderer()
{
  this->FreeTypeTools = svtkFreeTypeTools::GetInstance();
  this->MathTextUtilities = svtkMathTextUtilities::GetInstance();
}

//------------------------------------------------------------------------------
svtkMathTextFreeTypeTextRenderer::~svtkMathTextFreeTypeTextRenderer() = default;
