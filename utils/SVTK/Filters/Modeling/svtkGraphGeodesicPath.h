/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphGeodesicPath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGraphGeodesicPath
 * @brief   Abstract base for classes that generate a geodesic path on a graph (mesh).
 *
 * Serves as a base class for algorithms that trace a geodesic on a
 * polygonal dataset treating it as a graph. ie points connecting the
 * vertices of the graph
 */

#ifndef svtkGraphGeodesicPath_h
#define svtkGraphGeodesicPath_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkGeodesicPath.h"

class svtkIdList;

class SVTKFILTERSMODELING_EXPORT svtkGraphGeodesicPath : public svtkGeodesicPath
{
public:
  //@{
  /**
   * Standard methods for printing and determining type information.
   */
  svtkTypeMacro(svtkGraphGeodesicPath, svtkGeodesicPath);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * The vertex at the start of the shortest path
   */
  svtkGetMacro(StartVertex, svtkIdType);
  svtkSetMacro(StartVertex, svtkIdType);
  //@}

  //@{
  /**
   * The vertex at the end of the shortest path
   */
  svtkGetMacro(EndVertex, svtkIdType);
  svtkSetMacro(EndVertex, svtkIdType);
  //@}

protected:
  svtkGraphGeodesicPath();
  ~svtkGraphGeodesicPath() override;

  svtkIdType StartVertex;
  svtkIdType EndVertex;

private:
  svtkGraphGeodesicPath(const svtkGraphGeodesicPath&) = delete;
  void operator=(const svtkGraphGeodesicPath&) = delete;
};

#endif
