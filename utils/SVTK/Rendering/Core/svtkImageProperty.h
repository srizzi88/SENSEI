/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageProperty
 * @brief   image display properties
 *
 * svtkImageProperty is an object that allows control of the display
 * of an image slice.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 * @sa
 * svtkImage svtkImageMapper3D svtkImageSliceMapper svtkImageResliceMapper
 */

#ifndef svtkImageProperty_h
#define svtkImageProperty_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkScalarsToColors;

class SVTKRENDERINGCORE_EXPORT svtkImageProperty : public svtkObject
{
public:
  svtkTypeMacro(svtkImageProperty, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct a property with no lookup table.
   */
  static svtkImageProperty* New();

  /**
   * Assign one property to another.
   */
  void DeepCopy(svtkImageProperty* p);

  //@{
  /**
   * The window value for window/level.
   */
  svtkSetMacro(ColorWindow, double);
  svtkGetMacro(ColorWindow, double);
  //@}

  //@{
  /**
   * The level value for window/level.
   */
  svtkSetMacro(ColorLevel, double);
  svtkGetMacro(ColorLevel, double);
  //@}

  //@{
  /**
   * Specify a lookup table for the data.  If the data is
   * to be displayed as greyscale, or if the input data is
   * already RGB, there is no need to set a lookup table.
   */
  virtual void SetLookupTable(svtkScalarsToColors* lut);
  svtkGetObjectMacro(LookupTable, svtkScalarsToColors);
  //@}

  //@{
  /**
   * Use the range that is set in the lookup table, instead
   * of setting the range from the Window/Level settings.
   * This is off by default.
   */
  svtkSetMacro(UseLookupTableScalarRange, svtkTypeBool);
  svtkGetMacro(UseLookupTableScalarRange, svtkTypeBool);
  svtkBooleanMacro(UseLookupTableScalarRange, svtkTypeBool);
  //@}

  //@{
  /**
   * The opacity of the image, where 1.0 is opaque and 0.0 is
   * transparent.  If the image has an alpha component, then
   * the alpha component will be multiplied by this value.
   * The default is 1.0.
   */
  svtkSetClampMacro(Opacity, double, 0.0, 1.0);
  svtkGetMacro(Opacity, double);
  //@}

  //@{
  /**
   * The ambient lighting coefficient.  The default is 1.0.
   */
  svtkSetClampMacro(Ambient, double, 0.0, 1.0);
  svtkGetMacro(Ambient, double);
  //@}

  //@{
  /**
   * The diffuse lighting coefficient.  The default is 0.0.
   */
  svtkSetClampMacro(Diffuse, double, 0.0, 1.0);
  svtkGetMacro(Diffuse, double);
  //@}

  //@{
  /**
   * The interpolation type (default: SVTK_LINEAR_INTERPOLATION).
   */
  svtkSetClampMacro(InterpolationType, int, SVTK_NEAREST_INTERPOLATION, SVTK_CUBIC_INTERPOLATION);
  svtkGetMacro(InterpolationType, int);
  void SetInterpolationTypeToNearest() { this->SetInterpolationType(SVTK_NEAREST_INTERPOLATION); }
  void SetInterpolationTypeToLinear() { this->SetInterpolationType(SVTK_LINEAR_INTERPOLATION); }
  void SetInterpolationTypeToCubic() { this->SetInterpolationType(SVTK_CUBIC_INTERPOLATION); }
  virtual const char* GetInterpolationTypeAsString();
  //@}

  //@{
  /**
   * Set the layer number.  This is ignored unless the image is part
   * of a svtkImageStack.  The default layer number is zero.
   */
  svtkSetMacro(LayerNumber, int);
  int GetLayerNumber() { return this->LayerNumber; }
  //@}

  //@{
  /**
   * Make a checkerboard pattern where the black squares are transparent.
   * The pattern is aligned with the camera, and centered by default.
   */
  svtkSetMacro(Checkerboard, svtkTypeBool);
  svtkBooleanMacro(Checkerboard, svtkTypeBool);
  svtkGetMacro(Checkerboard, svtkTypeBool);
  //@}

  //@{
  /**
   * The spacing for checkerboarding.  This is in real units, not pixels.
   */
  svtkSetVector2Macro(CheckerboardSpacing, double);
  svtkGetVector2Macro(CheckerboardSpacing, double);
  //@}

  //@{
  /**
   * The phase offset for checkerboarding, in units of spacing.  Use a
   * value between -1 and +1, where 1 is an offset of one squares.
   */
  svtkSetVector2Macro(CheckerboardOffset, double);
  svtkGetVector2Macro(CheckerboardOffset, double);
  //@}

  //@{
  /**
   * Use an opaque backing polygon, which will be visible where the image
   * is translucent.  When images are in a stack, the backing polygons
   * for all images will be drawn before any of the images in the stack,
   * in order to allow the images in the stack to be composited.
   */
  svtkSetMacro(Backing, svtkTypeBool);
  svtkBooleanMacro(Backing, svtkTypeBool);
  svtkGetMacro(Backing, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the color of the backing polygon.  The default color is black.
   */
  svtkSetVector3Macro(BackingColor, double);
  svtkGetVector3Macro(BackingColor, double);
  //@}

  /**
   * Get the MTime for this property.  If the lookup table is set,
   * the mtime will include the mtime of the lookup table.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkImageProperty();
  ~svtkImageProperty() override;

  svtkScalarsToColors* LookupTable;
  double ColorWindow;
  double ColorLevel;
  svtkTypeBool UseLookupTableScalarRange;
  int InterpolationType;
  int LayerNumber;
  double Opacity;
  double Ambient;
  double Diffuse;
  svtkTypeBool Checkerboard;
  double CheckerboardSpacing[2];
  double CheckerboardOffset[2];
  svtkTypeBool Backing;
  double BackingColor[3];

private:
  svtkImageProperty(const svtkImageProperty&) = delete;
  void operator=(const svtkImageProperty&) = delete;
};

#endif
