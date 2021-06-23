/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkIconGlyphFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkIconGlyphFilter
 * @brief   Filter that generates a polydata consisting of
 * quads with texture coordinates referring to a set of icons within a sheet
 * of icons.
 *
 * svtkIconGlyphFilter takes in a svtkPointSet where each point corresponds to
 * the center of an icon. Scalar integer data must also be set to give each
 * point an icon index. This index is a zero based row major index into an
 * image that contains a grid of icons (each icon is the same size). You must
 * also specify 1) the size of the icon in the icon sheet (in pixels), 2) the
 * size of the icon sheet (in pixels), and 3) the display size of each icon
 * (again in display coordinates, or pixels).
 *
 * Various other parameters are used to control how this data is combined. If
 * UseIconSize is true then the DisplaySize is ignored. If PassScalars is true,
 * then the scalar index information is passed to the output. Also, there is an
 * optional IconScale array which, if UseIconScaling is on, will scale each icon
 * independently.
 *
 * @sa
 * svtkPolyDataAlgorithm svtkGlyph3D svtkGlyph2D
 */

#ifndef svtkIconGlyphFilter_h
#define svtkIconGlyphFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_ICON_GRAVITY_TOP_RIGHT 1
#define SVTK_ICON_GRAVITY_TOP_CENTER 2
#define SVTK_ICON_GRAVITY_TOP_LEFT 3
#define SVTK_ICON_GRAVITY_CENTER_RIGHT 4
#define SVTK_ICON_GRAVITY_CENTER_CENTER 5
#define SVTK_ICON_GRAVITY_CENTER_LEFT 6
#define SVTK_ICON_GRAVITY_BOTTOM_RIGHT 7
#define SVTK_ICON_GRAVITY_BOTTOM_CENTER 8
#define SVTK_ICON_GRAVITY_BOTTOM_LEFT 9

#define SVTK_ICON_SCALING_OFF 0
#define SVTK_ICON_SCALING_USE_SCALING_ARRAY 1

class SVTKFILTERSGENERAL_EXPORT svtkIconGlyphFilter : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard SVTK methods.
   */
  static svtkIconGlyphFilter* New();
  svtkTypeMacro(svtkIconGlyphFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the Width and Height, in pixels, of an icon in the icon sheet.
   */
  svtkSetVector2Macro(IconSize, int);
  svtkGetVectorMacro(IconSize, int, 2);
  //@}

  //@{
  /**
   * Specify the Width and Height, in pixels, of an icon in the icon sheet.
   */
  svtkSetVector2Macro(IconSheetSize, int);
  svtkGetVectorMacro(IconSheetSize, int, 2);
  //@}

  //@{
  /**
   * Specify the Width and Height, in pixels, of the size of the icon when it
   * is rendered. By default, the IconSize is used to set the display size
   * (i.e., UseIconSize is true by default). Note that assumes that
   * IconScaling is disabled, or if enabled, the scale of a particular icon
   * is 1.
   */
  svtkSetVector2Macro(DisplaySize, int);
  svtkGetVectorMacro(DisplaySize, int, 2);
  //@}

  //@{
  /**
   * Specify whether the Quad generated to place the icon on will be either
   * the dimensions specified by IconSize or the DisplaySize.
   */
  svtkSetMacro(UseIconSize, bool);
  svtkGetMacro(UseIconSize, bool);
  svtkBooleanMacro(UseIconSize, bool);
  //@}

  //@{
  /**
   * Specify how to specify individual icons. By default, icon scaling
   * is off, but if it is on, then the filter looks for an array named
   * "IconScale" to control individual icon size.
   */
  svtkSetMacro(IconScaling, int);
  svtkGetMacro(IconScaling, int);
  void SetIconScalingToScalingOff() { this->SetIconScaling(SVTK_ICON_SCALING_OFF); }
  void SetIconScalingToScalingArray() { this->SetIconScaling(SVTK_ICON_SCALING_USE_SCALING_ARRAY); }
  //@}

  //@{
  /**
   * Specify whether to pass the scalar icon index to the output. By
   * default this is not passed since it can affect color during the
   * rendering process. Note that all other point data is passed to
   * the output regardless of the value of this flag.
   */
  svtkSetMacro(PassScalars, bool);
  svtkGetMacro(PassScalars, bool);
  svtkBooleanMacro(PassScalars, bool);
  //@}

  //@{
  /**
   * Specify if the input points define the center of the icon quad or one of
   * top right corner, top center, top left corner, center right, center, center
   * center left, bottom right corner, bottom center or bottom left corner.
   */
  svtkSetMacro(Gravity, int);
  svtkGetMacro(Gravity, int);
  void SetGravityToTopRight() { this->SetGravity(SVTK_ICON_GRAVITY_TOP_RIGHT); }
  void SetGravityToTopCenter() { this->SetGravity(SVTK_ICON_GRAVITY_TOP_CENTER); }
  void SetGravityToTopLeft() { this->SetGravity(SVTK_ICON_GRAVITY_TOP_LEFT); }
  void SetGravityToCenterRight() { this->SetGravity(SVTK_ICON_GRAVITY_CENTER_RIGHT); }
  void SetGravityToCenterCenter() { this->SetGravity(SVTK_ICON_GRAVITY_CENTER_CENTER); }
  void SetGravityToCenterLeft() { this->SetGravity(SVTK_ICON_GRAVITY_CENTER_LEFT); }
  void SetGravityToBottomRight() { this->SetGravity(SVTK_ICON_GRAVITY_BOTTOM_RIGHT); }
  void SetGravityToBottomCenter() { this->SetGravity(SVTK_ICON_GRAVITY_BOTTOM_CENTER); }
  void SetGravityToBottomLeft() { this->SetGravity(SVTK_ICON_GRAVITY_BOTTOM_LEFT); }
  //@}

  //@{
  /**
   * Specify an offset (in pixels or display coordinates) that offsets the icons
   * from their generating points.
   */
  svtkSetVector2Macro(Offset, int);
  svtkGetVectorMacro(Offset, int, 2);
  //@}

protected:
  svtkIconGlyphFilter();
  ~svtkIconGlyphFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int IconSize[2];      // Size in pixels of an icon in an icon sheet
  int IconSheetSize[2]; // Size in pixels of the icon sheet
  int DisplaySize[2];   // Size in pixels of the icon when displayed

  int Gravity;
  bool UseIconSize;
  int IconScaling;
  bool PassScalars;
  int Offset[2];

private:
  svtkIconGlyphFilter(const svtkIconGlyphFilter&) = delete;
  void operator=(const svtkIconGlyphFilter&) = delete;

  void IconConvertIndex(int id, int& j, int& k);
};

inline void svtkIconGlyphFilter::IconConvertIndex(int id, int& j, int& k)
{
  int dimX = this->IconSheetSize[0] / this->IconSize[0];
  int dimY = this->IconSheetSize[1] / this->IconSize[1];

  j = id - dimX * static_cast<int>(id / dimX);
  k = dimY - static_cast<int>(id / dimX) - 1;
}

#endif
