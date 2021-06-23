/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyPointSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyPointSource
 * @brief   create points from a list of input points
 *
 * svtkPolyPointSource is a source object that creates a vert from
 * user-specified points. The output is a svtkPolyData.
 */

#ifndef svtkPolyPointSource_h
#define svtkPolyPointSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkPoints;

class SVTKFILTERSSOURCES_EXPORT svtkPolyPointSource : public svtkPolyDataAlgorithm
{
public:
  static svtkPolyPointSource* New();
  svtkTypeMacro(svtkPolyPointSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the number of points in the poly line.
   */
  void SetNumberOfPoints(svtkIdType numPoints);
  svtkIdType GetNumberOfPoints();
  //@}

  /**
   * Resize while preserving data.
   */
  void Resize(svtkIdType numPoints);

  /**
   * Set a point location.
   */
  void SetPoint(svtkIdType id, double x, double y, double z);

  //@{
  /**
   * Get the points.
   */
  void SetPoints(svtkPoints* points);
  svtkGetObjectMacro(Points, svtkPoints);
  //@}

  /**
   * Get the mtime plus consider its Points
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkPolyPointSource();
  ~svtkPolyPointSource() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkPoints* Points;

private:
  svtkPolyPointSource(const svtkPolyPointSource&) = delete;
  void operator=(const svtkPolyPointSource&) = delete;
};

#endif
