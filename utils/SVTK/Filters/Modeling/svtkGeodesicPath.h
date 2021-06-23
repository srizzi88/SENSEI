/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGeodesicPath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGeodesicPath
 * @brief   Abstract base for classes that generate a geodesic path
 *
 * Serves as a base class for algorithms that trace a geodesic path on a
 * polygonal dataset.
 */

#ifndef svtkGeodesicPath_h
#define svtkGeodesicPath_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkPolyData;

class SVTKFILTERSMODELING_EXPORT svtkGeodesicPath : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for printing and determining type information.
   */
  svtkTypeMacro(svtkGeodesicPath, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

protected:
  svtkGeodesicPath();
  ~svtkGeodesicPath() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkGeodesicPath(const svtkGeodesicPath&) = delete;
  void operator=(const svtkGeodesicPath&) = delete;
};

#endif
