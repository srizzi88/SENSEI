/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContourHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkContourHelper
 * @brief   A utility class used by various contour filters
 *
 *  This is a simple utility class that can be used by various contour filters to
 *  produce either triangles and/or polygons based on the outputTriangles parameter
 *  When working with multidimensional dataset, it is needed to process cells
 *  from low to high dimensions.
 * @sa
 * svtkContourGrid svtkCutter svtkContourFilter
 */

#ifndef svtkContourHelper_h
#define svtkContourHelper_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolygonBuilder.h"    //for a member variable
#include "svtkSmartPointer.h"      //for a member variable

class svtkIncrementalPointLocator;
class svtkCellArray;
class svtkPointData;
class svtkCellData;
class svtkCell;
class svtkDataArray;
class svtkIdListCollection;

class SVTKFILTERSCORE_EXPORT svtkContourHelper
{
public:
  svtkContourHelper(svtkIncrementalPointLocator* locator, svtkCellArray* verts, svtkCellArray* lines,
    svtkCellArray* polys, svtkPointData* inPd, svtkCellData* inCd, svtkPointData* outPd,
    svtkCellData* outCd, int estimatedSize, bool outputTriangles);
  ~svtkContourHelper();
  void Contour(svtkCell* cell, double value, svtkDataArray* cellScalars, svtkIdType cellId);

private:
  svtkContourHelper(const svtkContourHelper&) = delete;
  svtkContourHelper& operator=(const svtkContourHelper&) = delete;

  svtkIncrementalPointLocator* Locator;
  svtkCellArray* Verts;
  svtkCellArray* Lines;
  svtkCellArray* Polys;
  svtkPointData* InPd;
  svtkCellData* InCd;
  svtkPointData* OutPd;
  svtkCellData* OutCd;
  svtkSmartPointer<svtkCellData> TriOutCd;

  svtkCellArray* Tris;
  svtkPolygonBuilder PolyBuilder;
  svtkIdListCollection* PolyCollection;
  bool GenerateTriangles;
};

#endif
// SVTK-HeaderTest-Exclude: svtkContourHelper.h
