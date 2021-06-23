/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTableToPolyData
 * @brief   filter used to convert a svtkTable to a svtkPolyData
 * consisting of vertices.
 *
 * svtkTableToPolyData is a filter used to convert a svtkTable  to a svtkPolyData
 * consisting of vertices.
 */

#ifndef svtkTableToPolyData_h
#define svtkTableToPolyData_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkTableToPolyData : public svtkPolyDataAlgorithm
{
public:
  static svtkTableToPolyData* New();
  svtkTypeMacro(svtkTableToPolyData, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the name of the column to use as the X coordinate for the points.
   */
  svtkSetStringMacro(XColumn);
  svtkGetStringMacro(XColumn);
  //@}

  //@{
  /**
   * Set the index of the column to use as the X coordinate for the points.
   */
  svtkSetClampMacro(XColumnIndex, int, 0, SVTK_INT_MAX);
  svtkGetMacro(XColumnIndex, int);
  //@}

  //@{
  /**
   * Specify the component for the column specified using SetXColumn() to
   * use as the xcoordinate in case the column is a multi-component array.
   * Default is 0.
   */
  svtkSetClampMacro(XComponent, int, 0, SVTK_INT_MAX);
  svtkGetMacro(XComponent, int);
  //@}

  //@{
  /**
   * Set the name of the column to use as the Y coordinate for the points.
   * Default is 0.
   */
  svtkSetStringMacro(YColumn);
  svtkGetStringMacro(YColumn);
  //@}

  //@{
  /**
   * Set the index of the column to use as the Y coordinate for the points.
   */
  svtkSetClampMacro(YColumnIndex, int, 0, SVTK_INT_MAX);
  svtkGetMacro(YColumnIndex, int);
  //@}

  //@{
  /**
   * Specify the component for the column specified using SetYColumn() to
   * use as the Ycoordinate in case the column is a multi-component array.
   */
  svtkSetClampMacro(YComponent, int, 0, SVTK_INT_MAX);
  svtkGetMacro(YComponent, int);
  //@}

  //@{
  /**
   * Set the name of the column to use as the Z coordinate for the points.
   * Default is 0.
   */
  svtkSetStringMacro(ZColumn);
  svtkGetStringMacro(ZColumn);
  //@}

  //@{
  /**
   * Set the index of the column to use as the Z coordinate for the points.
   */
  svtkSetClampMacro(ZColumnIndex, int, 0, SVTK_INT_MAX);
  svtkGetMacro(ZColumnIndex, int);
  //@}

  //@{
  /**
   * Specify the component for the column specified using SetZColumn() to
   * use as the Zcoordinate in case the column is a multi-component array.
   */
  svtkSetClampMacro(ZComponent, int, 0, SVTK_INT_MAX);
  svtkGetMacro(ZComponent, int);
  //@}

  //@{
  /**
   * Specify whether the points of the polydata are 3D or 2D. If this is set to
   * true then the Z Column will be ignored and the z value of each point on the
   * polydata will be set to 0. By default this will be off.
   */
  svtkSetMacro(Create2DPoints, bool);
  svtkGetMacro(Create2DPoints, bool);
  svtkBooleanMacro(Create2DPoints, bool);
  //@}

  //@{
  /**
   * Allow user to keep columns specified as X,Y,Z as Data arrays.
   * By default this will be off.
   */
  svtkSetMacro(PreserveCoordinateColumnsAsDataArrays, bool);
  svtkGetMacro(PreserveCoordinateColumnsAsDataArrays, bool);
  svtkBooleanMacro(PreserveCoordinateColumnsAsDataArrays, bool);
  //@}

protected:
  svtkTableToPolyData();
  ~svtkTableToPolyData() override;

  /**
   * Overridden to specify that input must be a svtkTable.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * Convert input svtkTable to svtkPolyData.
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  char* XColumn;
  char* YColumn;
  char* ZColumn;
  int XColumnIndex;
  int YColumnIndex;
  int ZColumnIndex;
  int XComponent;
  int YComponent;
  int ZComponent;
  bool Create2DPoints;
  bool PreserveCoordinateColumnsAsDataArrays;

private:
  svtkTableToPolyData(const svtkTableToPolyData&) = delete;
  void operator=(const svtkTableToPolyData&) = delete;
};

#endif
