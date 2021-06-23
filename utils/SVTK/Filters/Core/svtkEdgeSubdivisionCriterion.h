/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEdgeSubdivisionCriterion.h
  Language:  C++

  Copyright 2003 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
#ifndef svtkEdgeSubdivisionCriterion_h
#define svtkEdgeSubdivisionCriterion_h
/**
 * @class   svtkEdgeSubdivisionCriterion
 * @brief   how to decide whether a linear approximation to nonlinear geometry or field should be
 * subdivided
 *
 *
 * Descendants of this abstract class are used to decide whether a
 * piecewise linear approximation (triangles, lines, ... ) to some
 * nonlinear geometry should be subdivided. This decision may be
 * based on an absolute error metric (chord error) or on some
 * view-dependent metric (chord error compared to device resolution)
 * or on some abstract metric (color error). Or anything else, really.
 * Just so long as you implement the EvaluateLocationAndFields member, all will
 * be well.
 *
 * @sa
 * svtkDataSetSubdivisionAlgorithm svtkStreamingTessellator
 */

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkObject.h"

class svtkDataSetAttributes;
class svtkMatrix4x4;
class svtkStreamingTessellator;

class SVTKFILTERSCORE_EXPORT svtkEdgeSubdivisionCriterion : public svtkObject
{
public:
  svtkTypeMacro(svtkEdgeSubdivisionCriterion, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * You must implement this member function in a subclass.
   * It will be called by \p svtkStreamingTessellator for each
   * edge in each primitive that svtkStreamingTessellator generates.
   */
  virtual bool EvaluateLocationAndFields(double* p1, int field_start) = 0;

  /**
   * This is a helper routine called by \p PassFields() which
   * you may also call directly; it adds \a sourceSize to the size of
   * the output vertex field values. The offset of the \a sourceId
   * field in the output vertex array is returned.
   * -1 is returned if \a sourceSize would force the output to have more
   * than \a svtkStreamingTessellator::MaxFieldSize field values per vertex.
   */
  virtual int PassField(int sourceId, int sourceSize, svtkStreamingTessellator* t);

  /**
   * Don't pass any field values in the vertex pointer.
   * This is used to reset the list of fields to pass after a
   * successful run of svtkStreamingTessellator.
   */
  virtual void ResetFieldList();

  /**
   * This does the opposite of \p PassField(); it removes a field from
   * the output (assuming the field was set to be passed).
   * Returns true if any action was taken, false otherwise.
   */
  virtual bool DontPassField(int sourceId, svtkStreamingTessellator* t);

  /**
   * Return the map from output field id to input field ids.
   * That is, field \a i of any output vertex from svtkStreamingTessellator
   * will be associated with \p GetFieldIds()[\a i] on the input mesh.
   */
  const int* GetFieldIds() const;

  /**
   * Return the offset into an output vertex array of all fields.
   * That is, field \a i of any output vertex, \a p, from svtkStreamingTessellator
   * will have its first entry at \a p[\p GetFieldOffsets()[\a i] ] .
   */
  const int* GetFieldOffsets() const;

  /**
   * Return the output ID of an input field.
   * Returns -1 if \a fieldId is not set to be passed to the output.
   */
  int GetOutputField(int fieldId) const;

  /**
   * Return the number of fields being evaluated at each output vertex.
   * This is the length of the arrays returned by \p GetFieldIds() and
   * \p GetFieldOffsets().
   */
  int GetNumberOfFields() const;

protected:
  svtkEdgeSubdivisionCriterion();
  ~svtkEdgeSubdivisionCriterion() override;

  int* FieldIds;
  int* FieldOffsets;
  int NumberOfFields;

  /**
   * Perform the core logic for a view-dependent subdivision.
   * Returns true if subdivision should occur, false otherwise.
   * This is to be used by subclasses once the mesh-specific
   * evaluation routines have been called to get the actual
   * (as opposed to linearly interpolated) midpoint coordinates.
   * Currently, this handles only geometry, but could conceivably
   * test scalar fields as well.
   * @param p0 is the first endpoint of the edge
   * @param p1 is the linearly interpolated midpoint of the edge
   * @param p1_actual is the actual midpoint of the edge
   * @param p2 is the second endpoint of the edge
   * @param field_start is the offset into the above arrays
   * indicating where the scalar field values start (when
   * isosurfacing, the embedding dimension may be smaller
   * than the number of parametric coordinates).
   * @param viewtrans is the viewing transform (from model to
   * screen coordinates). Applying this transform to p0, p1, etc.,
   * should yield screen-space coordinates.
   * @param pixelSize is the width and height of a pixel in
   * screen space coordinates.
   * @param allowableChordErr is the maximum allowable distance
   * between \a p1 and \a p1_actual, in multiples of pixelSize,
   * before subdivision will occur.
   */
  bool ViewDependentEval(const double* p0, double* p1, double* p1_actual, const double* p2,
    int field_start, svtkMatrix4x4* viewtrans, const double* pixelSize,
    double allowableChordErr) const;

  /**
   * Perform the core logic for a fixed multi-criterion,
   * scalar-field based subdivision.
   * Returns true if subdivision should occur, false otherwise.
   * This is to be used by subclasses once the mesh-specific
   * evaluation routines have been called to get the actual
   * (as opposed to linearly interpolated) midpoint geometry
   * and field values.
   * Only field values
   * are tested (not geometry) because you can save yourself
   * field evaluations if you check the geometry yourself
   * and it fails the test.
   * @param p0 is the first endpoint of the edge
   * @param p1 is the linearly interpolated midpoint of the edge
   * @param p1_actual is the actual midpoint of the edge
   * @param p2 is the second endpoint of the edge
   * @param field_start is the offset into the above arrays
   * indicating where the scalar field values start (when
   * isosurfacing, the embedding dimension may be smaller
   * than the number of parametric coordinates).
   * @param field_criteria is a bitfield specifying which
   * fields (of the fields specified by PassField or
   * PassFields) are to be considered for subdivision.
   * Thus, you may pass fields to the output mesh without
   * using them as subdivision criteria. In than case,
   * the allowableFieldErr will have an empty entry for
   * those fields.
   * @param allowableFieldErr is an array of tolerances,
   * one for each field passed to the output. If the
   * linearly interpolated and actual midpoint values
   * for any field are greater than the value specified
   * here, the member will return true.
   */
  bool FixedFieldErrorEval(double* p1, double* p1_actual, int field_start, int field_criteria,
    double* allowableFieldErr) const;

private:
  svtkEdgeSubdivisionCriterion(const svtkEdgeSubdivisionCriterion&) = delete;
  void operator=(const svtkEdgeSubdivisionCriterion&) = delete;
};

inline const int* svtkEdgeSubdivisionCriterion::GetFieldIds() const
{
  return this->FieldIds;
}
inline const int* svtkEdgeSubdivisionCriterion::GetFieldOffsets() const
{
  return this->FieldOffsets;
}
inline int svtkEdgeSubdivisionCriterion::GetNumberOfFields() const
{
  return this->NumberOfFields;
}

#endif // svtkEdgeSubdivisionCriterion_h
