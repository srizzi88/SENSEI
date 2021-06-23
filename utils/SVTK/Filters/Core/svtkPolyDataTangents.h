/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataTangents.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyDataTangents
 * @brief   compute tangents for triangulated polydata
 *
 * svtkPolyDataTangents is a filter that computes point and/or cell tangents for a triangulated
 * polydata.
 * This filter requires an input with both normals and tcoords on points.
 */

#ifndef svtkPolyDataTangents_h
#define svtkPolyDataTangents_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkFloatArray;
class svtkIdList;
class svtkPolyData;

class SVTKFILTERSCORE_EXPORT svtkPolyDataTangents : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkPolyDataTangents, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPolyDataTangents* New();

  //@{
  /**
   * Turn on/off the computation of point tangents.
   * Default is true.
   */
  svtkSetMacro(ComputePointTangents, bool);
  svtkGetMacro(ComputePointTangents, bool);
  svtkBooleanMacro(ComputePointTangents, bool);
  //@}

  //@{
  /**
   * Turn on/off the computation of cell tangents.
   * Default is false.
   */
  svtkSetMacro(ComputeCellTangents, bool);
  svtkGetMacro(ComputeCellTangents, bool);
  svtkBooleanMacro(ComputeCellTangents, bool);
  //@}

protected:
  svtkPolyDataTangents() = default;
  ~svtkPolyDataTangents() override = default;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  bool ComputePointTangents = true;
  bool ComputeCellTangents = false;

private:
  svtkPolyDataTangents(const svtkPolyDataTangents&) = delete;
  void operator=(const svtkPolyDataTangents&) = delete;
};

#endif
