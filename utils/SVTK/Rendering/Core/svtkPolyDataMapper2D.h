/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataMapper2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyDataMapper2D
 * @brief   draw svtkPolyData onto the image plane
 *
 * svtkPolyDataMapper2D is a mapper that renders 3D polygonal data
 * (svtkPolyData) onto the 2D image plane (i.e., the renderer's viewport).
 * By default, the 3D data is transformed into 2D data by ignoring the
 * z-coordinate of the 3D points in svtkPolyData, and taking the x-y values
 * as local display values (i.e., pixel coordinates). Alternatively, you
 * can provide a svtkCoordinate object that will transform the data into
 * local display coordinates (use the svtkCoordinate::SetCoordinateSystem()
 * methods to indicate which coordinate system you are transforming the
 * data from).
 *
 * @sa
 * svtkMapper2D svtkActor2D
 */

#ifndef svtkPolyDataMapper2D_h
#define svtkPolyDataMapper2D_h

#include "svtkMapper2D.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkCoordinate;
class svtkPolyData;
class svtkScalarsToColors;
class svtkUnsignedCharArray;

class SVTKRENDERINGCORE_EXPORT svtkPolyDataMapper2D : public svtkMapper2D
{
public:
  svtkTypeMacro(svtkPolyDataMapper2D, svtkMapper2D);
  static svtkPolyDataMapper2D* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the input to the mapper.
   */
  void SetInputData(svtkPolyData* in);
  svtkPolyData* GetInput();
  //@}

  //@{
  /**
   * Specify a lookup table for the mapper to use.
   */
  void SetLookupTable(svtkScalarsToColors* lut);
  svtkScalarsToColors* GetLookupTable();
  //@}

  /**
   * Create default lookup table. Generally used to create one when none
   * is available with the scalar data.
   */
  virtual void CreateDefaultLookupTable();

  //@{
  /**
   * Turn on/off flag to control whether scalar data is used to color objects.
   */
  svtkSetMacro(ScalarVisibility, svtkTypeBool);
  svtkGetMacro(ScalarVisibility, svtkTypeBool);
  svtkBooleanMacro(ScalarVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Control how the scalar data is mapped to colors.  By default
   * (ColorModeToDefault), unsigned char scalars are treated as
   * colors, and NOT mapped through the lookup table, while everything
   * else is. ColorModeToDirectScalar extends ColorModeToDefault such
   * that all integer types are treated as colors with values in the
   * range 0-255 and floating types are treated as colors with values
   * in the range 0.0-1.0. Setting
   * ColorModeToMapScalars means that all scalar data will be mapped through
   * the lookup table.  (Note that for multi-component scalars, the
   * particular component to use for mapping can be specified using the
   * ColorByArrayComponent() method.)
   */
  svtkSetMacro(ColorMode, int);
  svtkGetMacro(ColorMode, int);
  void SetColorModeToDefault();
  void SetColorModeToMapScalars();
  void SetColorModeToDirectScalars();
  //@}

  /**
   * Return the method of coloring scalar data.
   */
  const char* GetColorModeAsString();

  //@{
  /**
   * Control whether the mapper sets the lookuptable range based on its
   * own ScalarRange, or whether it will use the LookupTable ScalarRange
   * regardless of it's own setting. By default the Mapper is allowed to set
   * the LookupTable range, but users who are sharing LookupTables between
   * mappers/actors will probably wish to force the mapper to use the
   * LookupTable unchanged.
   */
  svtkSetMacro(UseLookupTableScalarRange, svtkTypeBool);
  svtkGetMacro(UseLookupTableScalarRange, svtkTypeBool);
  svtkBooleanMacro(UseLookupTableScalarRange, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify range in terms of scalar minimum and maximum (smin,smax). These
   * values are used to map scalars into lookup table. Has no effect when
   * UseLookupTableScalarRange is true.
   */
  svtkSetVector2Macro(ScalarRange, double);
  svtkGetVectorMacro(ScalarRange, double, 2);
  //@}

  //@{
  /**
   * Control how the filter works with scalar point data and cell attribute
   * data.  By default (ScalarModeToDefault), the filter will use point data,
   * and if no point data is available, then cell data is used. Alternatively
   * you can explicitly set the filter to use point data
   * (ScalarModeToUsePointData) or cell data (ScalarModeToUseCellData).
   * You can also choose to get the scalars from an array in point field
   * data (ScalarModeToUsePointFieldData) or cell field data
   * (ScalarModeToUseCellFieldData).  If scalars are coming from a field
   * data array, you must call ColorByArrayComponent before you call
   * GetColors.
   */
  svtkSetMacro(ScalarMode, int);
  svtkGetMacro(ScalarMode, int);
  void SetScalarModeToDefault() { this->SetScalarMode(SVTK_SCALAR_MODE_DEFAULT); }
  void SetScalarModeToUsePointData() { this->SetScalarMode(SVTK_SCALAR_MODE_USE_POINT_DATA); }
  void SetScalarModeToUseCellData() { this->SetScalarMode(SVTK_SCALAR_MODE_USE_CELL_DATA); }
  void SetScalarModeToUsePointFieldData()
  {
    this->SetScalarMode(SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
  }
  void SetScalarModeToUseCellFieldData()
  {
    this->SetScalarMode(SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
  }
  //@}

  //@{
  /**
   * Choose which component of which field data array to color by.
   */
  void ColorByArrayComponent(int arrayNum, int component);
  void ColorByArrayComponent(const char* arrayName, int component);
  //@}

  /**
   * Get the array name or number and component to color by.
   */
  const char* GetArrayName() { return this->ArrayName; }
  int GetArrayId() { return this->ArrayId; }
  int GetArrayAccessMode() { return this->ArrayAccessMode; }
  int GetArrayComponent() { return this->ArrayComponent; }

  /**
   * Overload standard modified time function. If lookup table is modified,
   * then this object is modified as well.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Specify a svtkCoordinate object to be used to transform the svtkPolyData
   * point coordinates. By default (no svtkCoordinate specified), the point
   * coordinates are taken as viewport coordinates (pixels in the viewport
   * into which the mapper is rendering).
   */
  virtual void SetTransformCoordinate(svtkCoordinate*);
  svtkGetObjectMacro(TransformCoordinate, svtkCoordinate);
  //@}

  //@{
  /**
   * Specify whether or not rounding to integers the transformed points when
   * TransformCoordinate is set. By default, it does not use double precision.
   */
  svtkGetMacro(TransformCoordinateUseDouble, bool);
  svtkSetMacro(TransformCoordinateUseDouble, bool);
  svtkBooleanMacro(TransformCoordinateUseDouble, bool);
  //@}

  /**
   * Map the scalars (if there are any scalars and ScalarVisibility is on)
   * through the lookup table, returning an unsigned char RGBA array. This is
   * typically done as part of the rendering process. The alpha parameter
   * allows the blending of the scalars with an additional alpha (typically
   * which comes from a svtkActor, etc.)
   */
  svtkUnsignedCharArray* MapScalars(double alpha);

  /**
   * Make a shallow copy of this mapper.
   */
  void ShallowCopy(svtkAbstractMapper* m) override;

protected:
  svtkPolyDataMapper2D();
  ~svtkPolyDataMapper2D() override;

  int FillInputPortInformation(int, svtkInformation*) override;

  svtkUnsignedCharArray* Colors;

  svtkScalarsToColors* LookupTable;
  svtkTypeBool ScalarVisibility;
  svtkTimeStamp BuildTime;
  double ScalarRange[2];
  svtkTypeBool UseLookupTableScalarRange;
  int ColorMode;
  int ScalarMode;

  svtkCoordinate* TransformCoordinate;
  bool TransformCoordinateUseDouble;

  // for coloring by a component of a field data array
  int ArrayId;
  char ArrayName[256];
  int ArrayComponent;
  int ArrayAccessMode;

private:
  svtkPolyDataMapper2D(const svtkPolyDataMapper2D&) = delete;
  void operator=(const svtkPolyDataMapper2D&) = delete;
};

#endif
