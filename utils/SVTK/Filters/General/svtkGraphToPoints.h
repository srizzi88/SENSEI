/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphToPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkGraphToPoints
 * @brief   convert a svtkGraph a set of points.
 *
 *
 * Converts a svtkGraph to a svtkPolyData containing a set of points.
 * This assumes that the points
 * of the graph have already been filled (perhaps by svtkGraphLayout).
 * The vertex data is passed along to the point data.
 */

#ifndef svtkGraphToPoints_h
#define svtkGraphToPoints_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkGraphToPoints : public svtkPolyDataAlgorithm
{
public:
  static svtkGraphToPoints* New();
  svtkTypeMacro(svtkGraphToPoints, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkGraphToPoints();
  ~svtkGraphToPoints() override {}

  /**
   * Convert the svtkGraph into svtkPolyData.
   */
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Set the input type of the algorithm to svtkGraph.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkGraphToPoints(const svtkGraphToPoints&) = delete;
  void operator=(const svtkGraphToPoints&) = delete;
};

#endif
