/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetEdgeSubdivisionCriterion.h
  Language:  C++

  Copyright 2003 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
#ifndef svtkDataSetEdgeSubdivisionCriterion_h
#define svtkDataSetEdgeSubdivisionCriterion_h
/**
 * @class   svtkDataSetEdgeSubdivisionCriterion
 * @brief   a subclass of svtkEdgeSubdivisionCriterion for svtkDataSet objects.
 *
 *
 * This is a subclass of svtkEdgeSubdivisionCriterion that is used for
 * tessellating cells of a svtkDataSet, particularly nonlinear
 * cells.
 *
 * It provides functions for setting the current cell being tessellated and a
 * convenience routine, \a EvaluateFields() to evaluate field values at a
 * point. You should call \a EvaluateFields() from inside \a EvaluateLocationAndFields()
 * whenever the result of \a EvaluateLocationAndFields() will be true. Otherwise, do
 * not call \a EvaluateFields() as the midpoint is about to be discarded.
 * (<i>Implementor's note</i>: This isn't true if UGLY_ASPECT_RATIO_HACK
 * has been defined. But in that case, we don't want the exact field values;
 * we need the linearly interpolated ones at the midpoint for continuity.)
 *
 * @sa
 * svtkEdgeSubdivisionCriterion
 */

#include "svtkEdgeSubdivisionCriterion.h"
#include "svtkFiltersCoreModule.h" // For export macro

class svtkCell;
class svtkDataSet;

class SVTKFILTERSCORE_EXPORT svtkDataSetEdgeSubdivisionCriterion : public svtkEdgeSubdivisionCriterion
{
public:
  svtkTypeMacro(svtkDataSetEdgeSubdivisionCriterion, svtkEdgeSubdivisionCriterion);
  static svtkDataSetEdgeSubdivisionCriterion* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  virtual void SetMesh(svtkDataSet*);
  svtkDataSet* GetMesh();

  const svtkDataSet* GetMesh() const;

  virtual void SetCellId(svtkIdType cell);
  svtkIdType GetCellId() const;

  svtkIdType& GetCellId();

  svtkCell* GetCell();

  const svtkCell* GetCell() const;

  bool EvaluateLocationAndFields(double* midpt, int field_start) override;

  /**
   * Evaluate all of the fields that should be output with the
   * given \a vertex and store them just past the parametric coordinates
   * of \a vertex, at the offsets given by
   * \p svtkEdgeSubdivisionCriterion::GetFieldOffsets() plus \a field_start.
   * \a field_start contains the number of world-space coordinates (always 3)
   * plus the embedding dimension (the size of the parameter-space in which
   * the cell is embedded). It will range between 3 and 6, inclusive.

   * You must have called SetCellId() before calling this routine or there
   * will not be a mesh over which to evaluate the fields.

   * You must have called \p svtkEdgeSubdivisionCriterion::PassDefaultFields()
   * or \p svtkEdgeSubdivisionCriterion::PassField() or there will be no fields
   * defined for the output vertex.

   * This routine is public and returns its input argument so that it
   * may be used as an argument to
   * \p svtkStreamingTessellator::AdaptivelySamplekFacet():
   * @verbatim
   * svtkStreamingTessellator* t = svtkStreamingTessellator::New();
   * svtkEdgeSubdivisionCriterion* s;
   * ...
   * t->AdaptivelySample1Facet( s->EvaluateFields( p0 ), s->EvaluateFields( p1 ) );
   * ...
   * @endverbatim
   * Although this will work, using \p EvaluateFields() in this manner
   * should be avoided. It's much more efficient to fetch the corner values
   * for each attribute and copy them into \a p0, \a p1, ... as opposed to
   * performing shape function evaluations. The only case where you wouldn't
   * want to do this is when the field you are interpolating is discontinuous
   * at cell borders, such as with a discontinuous galerkin method or when
   * all the Gauss points for quadrature are interior to the cell.

   * The final argument, \a weights, is the array of weights to apply to each
   * point's data when interpolating the field. This is returned by
   * \a svtkCell::EvaluateLocation() when evaluating the geometry.
   */
  double* EvaluateFields(double* vertex, double* weights, int field_start);

  //@{
  /**
   * Evaluate either a cell or nodal field.
   * This exists because of the funky way that Exodus data will be handled.
   * Sure, it's a hack, but what are ya gonna do?
   */
  void EvaluatePointDataField(double* result, double* weights, int field);
  void EvaluateCellDataField(double* result, double* weights, int field);
  //@}

  //@{
  /**
   * Get/Set the square of the allowable chord error at any edge's midpoint.
   * This value is used by EvaluateLocationAndFields.
   */
  svtkSetMacro(ChordError2, double);
  svtkGetMacro(ChordError2, double);
  //@}

  //@{
  /**
   * Get/Set the square of the allowable error magnitude for the
   * scalar field \a s at any edge's midpoint.
   * A value less than or equal to 0 indicates that the field
   * should not be used as a criterion for subdivision.
   */
  virtual void SetFieldError2(int s, double err);
  double GetFieldError2(int s) const;
  //@}

  /**
   * Tell the subdivider not to use any field values as subdivision criteria.
   * Effectively calls SetFieldError2( a, -1. ) for all fields.
   */
  virtual void ResetFieldError2();

  //@{
  /**
   * Return a bitfield specifying which FieldError2 criteria are positive (i.e., actively
   * used to decide edge subdivisions).
   * This is stored as separate state to make subdivisions go faster.
   */
  svtkGetMacro(ActiveFieldCriteria, int);
  int GetActiveFieldCriteria() const { return this->ActiveFieldCriteria; }
  //@}

protected:
  svtkDataSetEdgeSubdivisionCriterion();
  ~svtkDataSetEdgeSubdivisionCriterion() override;

  svtkDataSet* CurrentMesh;
  svtkIdType CurrentCellId;
  svtkCell* CurrentCellData;

  double ChordError2;
  double* FieldError2;
  int FieldError2Length;
  int FieldError2Capacity;
  int ActiveFieldCriteria;

private:
  svtkDataSetEdgeSubdivisionCriterion(const svtkDataSetEdgeSubdivisionCriterion&) = delete;
  void operator=(const svtkDataSetEdgeSubdivisionCriterion&) = delete;
};

inline svtkIdType& svtkDataSetEdgeSubdivisionCriterion::GetCellId()
{
  return this->CurrentCellId;
}
inline svtkIdType svtkDataSetEdgeSubdivisionCriterion::GetCellId() const
{
  return this->CurrentCellId;
}

inline svtkDataSet* svtkDataSetEdgeSubdivisionCriterion::GetMesh()
{
  return this->CurrentMesh;
}
inline const svtkDataSet* svtkDataSetEdgeSubdivisionCriterion::GetMesh() const
{
  return this->CurrentMesh;
}

inline svtkCell* svtkDataSetEdgeSubdivisionCriterion::GetCell()
{
  return this->CurrentCellData;
}
inline const svtkCell* svtkDataSetEdgeSubdivisionCriterion::GetCell() const
{
  return this->CurrentCellData;
}

#endif // svtkDataSetEdgeSubdivisionCriterion_h
