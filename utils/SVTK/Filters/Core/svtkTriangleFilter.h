/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTriangleFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTriangleFilter
 * @brief   convert input polygons and strips to triangles
 *
 * svtkTriangleFilter generates triangles from input polygons and triangle
 * strips.  It also generates line segments from polylines unless PassLines
 * is off, and generates individual vertex cells from svtkVertex point lists
 * unless PassVerts is off.
 */

#ifndef svtkTriangleFilter_h
#define svtkTriangleFilter_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSCORE_EXPORT svtkTriangleFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkTriangleFilter* New();
  svtkTypeMacro(svtkTriangleFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Turn on/off passing vertices through filter (default: on).
   * If this is on, then the input vertex cells will be broken
   * into individual vertex cells (one point per cell).  If it
   * is off, the input vertex cells will be ignored.
   */
  svtkBooleanMacro(PassVerts, svtkTypeBool);
  svtkSetMacro(PassVerts, svtkTypeBool);
  svtkGetMacro(PassVerts, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off passing lines through filter (default: on).
   * If this is on, then the input polylines will be broken
   * into line segments.  If it is off, then the input lines
   * will be ignored and the output will have no lines.
   */
  svtkBooleanMacro(PassLines, svtkTypeBool);
  svtkSetMacro(PassLines, svtkTypeBool);
  svtkGetMacro(PassLines, svtkTypeBool);
  //@}

protected:
  svtkTriangleFilter()
    : PassVerts(1)
    , PassLines(1)
  {
  }
  ~svtkTriangleFilter() override {}

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkTypeBool PassVerts;
  svtkTypeBool PassLines;

private:
  svtkTriangleFilter(const svtkTriangleFilter&) = delete;
  void operator=(const svtkTriangleFilter&) = delete;
};

#endif
