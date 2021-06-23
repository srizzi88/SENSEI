/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkButtonSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkButtonSource
 * @brief   abstract class for creating various button types
 *
 * svtkButtonSource is an abstract class that defines an API for creating
 * "button-like" objects in SVTK. A button is a geometry with a rectangular
 * region that can be textured. The button is divided into two regions: the
 * texture region and the shoulder region. The points in both regions are
 * assigned texture coordinates. The texture region has texture coordinates
 * consistent with the image to be placed on it.  All points in the shoulder
 * regions are assigned a texture coordinate specified by the user.  In this
 * way the shoulder region can be colored by the texture.
 *
 * Creating a svtkButtonSource requires specifying its center point.
 * (Subclasses have other attributes that must be set to control
 * the shape of the button.) You must also specify how to control
 * the shape of the texture region; i.e., whether to size the
 * texture region proportional to the texture dimensions or whether
 * to size the texture region proportional to the button. Also, buttons
 * can be created single sided are mirrored to create two-sided buttons.
 *
 * @sa
 * svtkEllipticalButtonSource svtkRectangularButtonSource
 *
 * @warning
 * The button is defined in the x-y plane. Use svtkTransformPolyDataFilter
 * or svtkGlyph3D to orient the button in a different direction.
 */

#ifndef svtkButtonSource_h
#define svtkButtonSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_TEXTURE_STYLE_FIT_IMAGE 0
#define SVTK_TEXTURE_STYLE_PROPORTIONAL 1

class SVTKFILTERSSOURCES_EXPORT svtkButtonSource : public svtkPolyDataAlgorithm
{
public:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkButtonSource, svtkPolyDataAlgorithm);

  //@{
  /**
   * Specify a point defining the origin (center) of the button.
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Set/Get the style of the texture region: whether to size it
   * according to the x-y dimensions of the texture, or whether to make
   * the texture region proportional to the width/height of the button.
   */
  svtkSetClampMacro(TextureStyle, int, SVTK_TEXTURE_STYLE_FIT_IMAGE, SVTK_TEXTURE_STYLE_PROPORTIONAL);
  svtkGetMacro(TextureStyle, int);
  void SetTextureStyleToFitImage() { this->SetTextureStyle(SVTK_TEXTURE_STYLE_FIT_IMAGE); }
  void SetTextureStyleToProportional() { this->SetTextureStyle(SVTK_TEXTURE_STYLE_PROPORTIONAL); }
  //@}

  //@{
  /**
   * Set/get the texture dimension. This needs to be set if the texture
   * style is set to fit the image.
   */
  svtkSetVector2Macro(TextureDimensions, int);
  svtkGetVector2Macro(TextureDimensions, int);
  //@}

  //@{
  /**
   * Set/Get the default texture coordinate to set the shoulder region to.
   */
  svtkSetVector2Macro(ShoulderTextureCoordinate, double);
  svtkGetVector2Macro(ShoulderTextureCoordinate, double);
  //@}

  //@{
  /**
   * Indicate whether the button is single or double sided. A double sided
   * button can be viewed from two sides...it looks sort of like a "pill."
   * A single-sided button is meant to viewed from a single side; it looks
   * like a "clam-shell."
   */
  svtkSetMacro(TwoSided, svtkTypeBool);
  svtkGetMacro(TwoSided, svtkTypeBool);
  svtkBooleanMacro(TwoSided, svtkTypeBool);
  //@}

protected:
  svtkButtonSource();
  ~svtkButtonSource() override {}

  double Center[3];
  double ShoulderTextureCoordinate[2];
  int TextureStyle;
  int TextureDimensions[2];
  svtkTypeBool TwoSided;

private:
  svtkButtonSource(const svtkButtonSource&) = delete;
  void operator=(const svtkButtonSource&) = delete;
};

#endif
