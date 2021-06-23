/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBooleanOperationPolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBooleanOperationPolyDataFilter
 *
 *
 * Computes the boundary of the union, intersection, or difference
 * volume computed from the volumes defined by two input surfaces. The
 * two surfaces do not need to be manifold, but if they are not,
 * unexpected results may be obtained. The resulting surface is
 * available in the first output of the filter. The second output
 * contains a set of polylines that represent the intersection between
 * the two input surfaces.
 *
 * @warning This filter is not designed to perform 2D boolean operations,
 * and in fact relies on the inputs having no co-planar, overlapping cells.
 *
 * This code was contributed in the SVTK Journal paper:
 * "Boolean Operations on Surfaces in SVTK Without External Libraries"
 * by Cory Quammen, Chris Weigle C., Russ Taylor
 * http://hdl.handle.net/10380/3262
 * http://www.midasjournal.org/browse/publication/797
 */

#ifndef svtkBooleanOperationPolyDataFilter_h
#define svtkBooleanOperationPolyDataFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#include "svtkDataSetAttributes.h" // Needed for CopyCells() method

class svtkIdList;

class SVTKFILTERSGENERAL_EXPORT svtkBooleanOperationPolyDataFilter : public svtkPolyDataAlgorithm
{
public:
  /**
   * Construct object that computes the boolean surface.
   */
  static svtkBooleanOperationPolyDataFilter* New();

  svtkTypeMacro(svtkBooleanOperationPolyDataFilter, svtkPolyDataAlgorithm);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum OperationType
  {
    SVTK_UNION = 0,
    SVTK_INTERSECTION,
    SVTK_DIFFERENCE
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
   * Turn on/off cell reorientation of the intersection portion of the
   * surface when the operation is set to DIFFERENCE. Defaults to on.
   */
  svtkSetMacro(ReorientDifferenceCells, svtkTypeBool);
  svtkGetMacro(ReorientDifferenceCells, svtkTypeBool);
  svtkBooleanMacro(ReorientDifferenceCells, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get the tolerance used to determine when a point's absolute
   * distance is considered to be zero. Defaults to 1e-6.
   */
  svtkSetMacro(Tolerance, double);
  svtkGetMacro(Tolerance, double);
  //@}

protected:
  svtkBooleanOperationPolyDataFilter();
  ~svtkBooleanOperationPolyDataFilter() override;

  /**
   * Labels triangles in mesh as part of the intersection or union surface.
   */
  void SortPolyData(svtkPolyData* input, svtkIdList* intersectionList, svtkIdList* unionList);

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkBooleanOperationPolyDataFilter(const svtkBooleanOperationPolyDataFilter&) = delete;
  void operator=(const svtkBooleanOperationPolyDataFilter&) = delete;

  /**
   * Copies cells with indices given by from one svtkPolyData to
   * another. The point and cell field lists are used to determine
   * which fields should be copied.
   */
  void CopyCells(svtkPolyData* in, svtkPolyData* out, int idx,
    svtkDataSetAttributes::FieldList& pointFieldList, svtkDataSetAttributes::FieldList& cellFieldList,
    svtkIdList* cellIds, bool reverseCells);

  /**
   * Tolerance used to determine when a point's absolute
   * distance is considered to be zero.
   */
  double Tolerance;

  /**
   * Which operation to perform.
   * Can be SVTK_UNION, SVTK_INTERSECTION, or SVTK_DIFFERENCE.
   */
  int Operation;

  //@{
  /**
   * Determines if cells from the intersection surface should be
   * reversed in the difference surface.
   */
  svtkTypeBool ReorientDifferenceCells;
  //@}
};

#endif
