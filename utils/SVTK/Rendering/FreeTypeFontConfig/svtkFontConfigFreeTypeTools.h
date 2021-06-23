/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFontConfigFreeTypeTools.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkFontConfigFreeTypeTools
 * @brief   Subclass of svtkFreeTypeTools that uses
 * system installed fonts.
 *
 *
 * svtkFontConfigFreeTypeTools defers to svtkFreeTypeTools for rendering and
 * rasterization, but sources fonts from a FontConfig system lookup. If the
 * lookup fails, the compiled fonts of svtkFreeType are used instead.
 *
 * @warning
 * Do not instantiate this class directly. Rather, call
 * svtkFreeTypeTools::GetInstance() to ensure that the singleton design is
 * correctly applied.
 * Be aware that FontConfig lookup is disabled by default. To enable, call
 * svtkFreeTypeTools::GetInstance()->ForceCompiledFontsOff();
 */

#ifndef svtkFontConfigFreeTypeTools_h
#define svtkFontConfigFreeTypeTools_h

#include "svtkFreeTypeTools.h"
#include "svtkRenderingFreeTypeFontConfigModule.h" // For export macro

class SVTKRENDERINGFREETYPEFONTCONFIG_EXPORT svtkFontConfigFreeTypeTools : public svtkFreeTypeTools
{
public:
  svtkTypeMacro(svtkFontConfigFreeTypeTools, svtkFreeTypeTools);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a new object of this type, but it is not preferred to use this
   * method directly. Instead, call svtkFreeTypeTools::GetInstance() and let
   * the object factory create a new instance. In this way the singleton
   * pattern of svtkFreeTypeTools is preserved.
   */
  static svtkFontConfigFreeTypeTools* New();

  /**
   * Modified version of svtkFreeTypeTools::LookupFace that locates FontConfig
   * faces. Falls back to the Superclass method for compiled fonts if the
   * FontConfig lookup fails.
   */
  static bool LookupFaceFontConfig(svtkTextProperty* tprop, FT_Library lib, FT_Face* face);

protected:
  svtkFontConfigFreeTypeTools();
  ~svtkFontConfigFreeTypeTools() override;

  /**
   * Reimplemented from Superclass to use the FontConfig face lookup callback.
   */
  FT_Error CreateFTCManager() override;

private:
  svtkFontConfigFreeTypeTools(const svtkFontConfigFreeTypeTools&) = delete;
  void operator=(const svtkFontConfigFreeTypeTools&) = delete;
};

#endif // svtkFontConfigFreeTypeTools_h
