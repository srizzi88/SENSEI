/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOutlineSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOutlineSource
 * @brief   create wireframe outline around bounding box
 *
 * svtkOutlineSource creates a wireframe outline around a
 * user-specified bounding box.  The outline may be created aligned
 * with the {x,y,z} axis - in which case it is defined by the 6 bounds
 * {xmin,xmax,ymin,ymax,zmin,zmax} via SetBounds(). Alternatively, the
 * box may be arbitrarily aligned, in which case it should be set via
 * the SetCorners() member.
 */

#ifndef svtkOutlineSource_h
#define svtkOutlineSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_BOX_TYPE_AXIS_ALIGNED 0
#define SVTK_BOX_TYPE_ORIENTED 1

class SVTKFILTERSSOURCES_EXPORT svtkOutlineSource : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation. type information, and printing.
   */
  static svtkOutlineSource* New();
  svtkTypeMacro(svtkOutlineSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Set box type to AxisAligned (default) or Oriented.
   * Use the method SetBounds() with AxisAligned mode, and SetCorners()
   * with Oriented mode.
   */
  svtkSetMacro(BoxType, int);
  svtkGetMacro(BoxType, int);
  void SetBoxTypeToAxisAligned() { this->SetBoxType(SVTK_BOX_TYPE_AXIS_ALIGNED); }
  void SetBoxTypeToOriented() { this->SetBoxType(SVTK_BOX_TYPE_ORIENTED); }
  //@}

  //@{
  /**
   * Specify the bounds of the box to be used in Axis Aligned mode.
   */
  svtkSetVector6Macro(Bounds, double);
  svtkGetVectorMacro(Bounds, double, 6);
  //@}

  //@{
  /**
   * Specify the corners of the outline when in Oriented mode, the
   * values are supplied as 8*3 double values The correct corner
   * ordering is using {x,y,z} convention for the unit cube as follows:
   * {0,0,0},{1,0,0},{0,1,0},{1,1,0},{0,0,1},{1,0,1},{0,1,1},{1,1,1}.
   */
  svtkSetVectorMacro(Corners, double, 24);
  svtkGetVectorMacro(Corners, double, 24);
  //@}

  //@{
  /**
   * Generate solid faces for the box. This is off by default.
   */
  svtkSetMacro(GenerateFaces, svtkTypeBool);
  svtkBooleanMacro(GenerateFaces, svtkTypeBool);
  svtkGetMacro(GenerateFaces, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * svtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * svtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkOutlineSource();
  ~svtkOutlineSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int BoxType;
  svtkTypeBool GenerateFaces;
  int OutputPointsPrecision;
  double Bounds[6];
  double Corners[24];

private:
  svtkOutlineSource(const svtkOutlineSource&) = delete;
  void operator=(const svtkOutlineSource&) = delete;
};

#endif
