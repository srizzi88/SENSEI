/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLoopBooleanPolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLoopBooleanPolyDataFilter
 *
 *
 * Computes the boundary of the union, intersection, or difference
 * volume computed from the volumes defined by two input surfaces. The
 * two surfaces do not need to be manifold, but if they are not,
 * unexpected results may be obtained. The resulting surface is
 * available in the first output of the filter. The second output
 * contains a set of polylines that represent the intersection between
 * the two input surfaces.
 * The filter uses svtkIntersectionPolyDataFilter. Must have information
 * about the cells on mesh that the intersection lines touch. Filter assumes
 * this information is given.
 * The output result will have data about the Original Surface,
 * BoundaryPoints, Boundary Cells,
 * Free Edges, and Bad Triangles
 */
#ifndef svtkLoopBooleanPolyDataFilter_h
#define svtkLoopBooleanPolyDataFilter_h

#include "svtkDataSetAttributes.h"    // Needed for CopyCells() method
#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkIdList;

/*!
 *  \brief Filter to perform boolean operations
 *  \author Adam Updegrove
 */
class SVTKFILTERSGENERAL_EXPORT svtkLoopBooleanPolyDataFilter : public svtkPolyDataAlgorithm
{
public:
  /**
   * Construct object that computes the boolean surface.
   */
  static svtkLoopBooleanPolyDataFilter* New();

  svtkTypeMacro(svtkLoopBooleanPolyDataFilter, svtkPolyDataAlgorithm);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Integer describing the number of intersection points and lines
   */
  svtkGetMacro(NumberOfIntersectionPoints, int);
  svtkGetMacro(NumberOfIntersectionLines, int);
  //@}

  //@{
  /**
   * ONLY USED IF NO INTERSECTION BETWEEN SURFACES
   * Variable to determine what is output if no intersection occurs.
   * 0 = neither (default), 1 = first, 2 = second, 3 = both
   */
  svtkGetMacro(NoIntersectionOutput, int);
  svtkSetMacro(NoIntersectionOutput, int);
  svtkBooleanMacro(NoIntersectionOutput, int);
  //@}

  // Union intersection, or difference
  enum OperationType
  {
    SVTK_UNION = 0,
    SVTK_INTERSECTION,
    SVTK_DIFFERENCE
  };
  // Output if no intersection
  enum NoIntersectionOutputType
  {
    SVTK_NEITHER = 0,
    SVTK_FIRST,
    SVTK_SECOND,
    SVTK_BOTH,
  };

  //@{
  /**
   * Set the boolean operation to perform. Defaults to union.
   */
  svtkSetClampMacro(Operation, int, SVTK_UNION, SVTK_DIFFERENCE);
  svtkGetMacro(Operation, int);
  void SetOperationToUnion() { this->SetOperation(SVTK_UNION); }
  void SetOperationToIntersection() { this->SetOperation(SVTK_INTERSECTION); }
  void SetOperationToDifference() { this->SetOperation(SVTK_DIFFERENCE); }
  //@}

  //@{
  /**
   * Check the status of the filter after update. If the status is zero,
   * there was an error in the operation. If status is one, everything
   * went smoothly
   */
  svtkGetMacro(Status, int);
  //@}

  //@{
  /**
   * Set the tolerance for geometric tests
   */
  svtkGetMacro(Tolerance, double);
  svtkSetMacro(Tolerance, double);
  //@}

protected:
  svtkLoopBooleanPolyDataFilter();
  ~svtkLoopBooleanPolyDataFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkLoopBooleanPolyDataFilter(const svtkLoopBooleanPolyDataFilter&) = delete;
  void operator=(const svtkLoopBooleanPolyDataFilter&) = delete;

  //@{
  /**
   * Which operation to perform.
   * Can be SVTK_UNION, SVTK_INTERSECTION, or SVTK_DIFFERENCE.
   */
  int Operation;
  int NoIntersectionOutput;
  int NumberOfIntersectionPoints;
  int NumberOfIntersectionLines;
  //@}

  int Status;
  double Tolerance;

  class Impl;
};

#endif
