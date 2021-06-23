/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextRenderer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTextRenderer.h"

#include "svtkDebugLeaks.h" // Must be included before any singletons
#include "svtkImageData.h"
#include "svtkObjectFactory.h"
#include "svtkPath.h"
#include "svtkStdString.h"
#include "svtkTextProperty.h"
#include "svtkUnicodeString.h"

#include <svtksys/RegularExpression.hxx>

//----------------------------------------------------------------------------
// The singleton, and the singleton cleanup
svtkTextRenderer* svtkTextRenderer::Instance = nullptr;
svtkTextRendererCleanup svtkTextRenderer::Cleanup;

//----------------------------------------------------------------------------
svtkTextRendererCleanup::svtkTextRendererCleanup() = default;

//----------------------------------------------------------------------------
svtkTextRendererCleanup::~svtkTextRendererCleanup()
{
  svtkTextRenderer::SetInstance(nullptr);
}

//----------------------------------------------------------------------------
void svtkTextRenderer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Instance: " << svtkTextRenderer::Instance << endl;
  os << indent << "MathTextRegExp: " << this->MathTextRegExp << endl;
  os << indent << "MathTextRegExp2: " << this->MathTextRegExp2 << endl;
}

//----------------------------------------------------------------------------
svtkTextRenderer* svtkTextRenderer::New()
{
  svtkTextRenderer* instance = svtkTextRenderer::GetInstance();
  if (instance)
  {
    instance->Register(nullptr);
  }
  return instance;
}

//----------------------------------------------------------------------------
svtkTextRenderer* svtkTextRenderer::GetInstance()
{
  if (svtkTextRenderer::Instance)
  {
    return svtkTextRenderer::Instance;
  }

  svtkTextRenderer::Instance =
    static_cast<svtkTextRenderer*>(svtkObjectFactory::CreateInstance("svtkTextRenderer"));

  return svtkTextRenderer::Instance;
}

//----------------------------------------------------------------------------
void svtkTextRenderer::SetInstance(svtkTextRenderer* instance)
{
  if (svtkTextRenderer::Instance == instance)
  {
    return;
  }

  if (svtkTextRenderer::Instance)
  {
    svtkTextRenderer::Instance->Delete();
  }

  svtkTextRenderer::Instance = instance;

  if (instance)
  {
    instance->Register(nullptr);
  }
}

//----------------------------------------------------------------------------
svtkTextRenderer::svtkTextRenderer()
  : MathTextRegExp(new svtksys::RegularExpression("[^\\]\\$.*[^\\]\\$"))
  , MathTextRegExp2(new svtksys::RegularExpression("^\\$.*[^\\]\\$"))
  , DefaultBackend(Detect)
{
}

//----------------------------------------------------------------------------
svtkTextRenderer::~svtkTextRenderer()
{
  delete this->MathTextRegExp;
  delete this->MathTextRegExp2;
}

//----------------------------------------------------------------------------
int svtkTextRenderer::DetectBackend(const svtkStdString& str)
{
  if (!str.empty())
  {
    // the svtksys::RegularExpression class doesn't support {...|...} "or"
    // branching, so we check the first character to see which regexp to use:
    //
    // Find unescaped "$...$" patterns where "$" is not the first character:
    //   MathTextRegExp  = "[^\\]\\$.*[^\\]\\$"
    // Find unescaped "$...$" patterns where "$" is the first character:
    //   MathTextRegExp2 = "^\\$.*[^\\]\\$"
    if ((str[0] == '$' && this->MathTextRegExp2->find(str)) || this->MathTextRegExp->find(str))
    {
      return static_cast<int>(MathText);
    }
  }
  return static_cast<int>(FreeType);
}

//----------------------------------------------------------------------------
int svtkTextRenderer::DetectBackend(const svtkUnicodeString& str)
{
  if (!str.empty())
  {
    // the svtksys::RegularExpression class doesn't support {...|...} "or"
    // branching, so we check the first character to see which regexp to use:
    //
    // Find unescaped "$...$" patterns where "$" is not the first character:
    //   MathTextRegExp  = "[^\\]\\$.*[^\\]\\$"
    // Find unescaped "$...$" patterns where "$" is the first character:
    //   MathTextRegExp2 = "^\\$.*[^\\]\\$"
    if ((str[0] == '$' && this->MathTextRegExp2->find(str.utf8_str())) ||
      this->MathTextRegExp->find(str.utf8_str()))
    {
      return static_cast<int>(MathText);
    }
  }
  return static_cast<int>(FreeType);
}

//----------------------------------------------------------------------------
void svtkTextRenderer::CleanUpFreeTypeEscapes(svtkStdString& str)
{
  size_t ind = str.find("\\$");
  while (ind != std::string::npos)
  {
    str.replace(ind, 2, "$");
    ind = str.find("\\$", ind + 1);
  }
}

//----------------------------------------------------------------------------
void svtkTextRenderer::CleanUpFreeTypeEscapes(svtkUnicodeString& str)
{
  // svtkUnicodeString has only a subset of the std::string API available, so
  // this method is more complex than the std::string overload.
  svtkUnicodeString::const_iterator begin = str.begin();
  svtkUnicodeString::const_iterator end = str.end();
  svtkUnicodeString tmp;

  for (svtkUnicodeString::const_iterator it = begin; it != end; ++it)
  {
    if (*it != '\\')
    {
      continue;
    }

    // No operator+ in the unicode string iterator. Copy and advance it:
    svtkUnicodeString::const_iterator nextChar = it;
    std::advance(nextChar, 1);
    if (*nextChar != '$')
    {
      continue;
    }

    // We found a "\$" in the string. Append [begin, it) into tmp.
    tmp.append(begin, it);

    // Add the dollar sign
    tmp.append(svtkUnicodeString::from_utf8("$"));

    // Reset the iterators to continue checking the rest of the string.
    begin = it;
    std::advance(it, 1);
    std::advance(begin, 2);
  }

  // Append the last bit of the string to tmp
  tmp.append(begin, end);

  // Update the input with the cleaned up string:
  str = tmp;
}
