/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMeshQuality.h
  Language:  C++

  Copyright 2003-2006 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

  Contact: dcthomp@sandia.gov,pppebay@sandia.gov

=========================================================================*/
/**
 * @class   svtkMeshQuality
 * @brief   Calculate functions of quality of the elements
 *  of a mesh
 *
 *
 * svtkMeshQuality computes one or more functions of (geometric)
 * quality for each 2-D and 3-D cell (triangle, quadrilateral, tetrahedron,
 * or hexahedron) of a mesh. These functions of quality are then averaged
 * over the entire mesh. The minimum, average, maximum, and unbiased variance
 * of quality for each type of cell is stored in the output mesh's FieldData.
 * The FieldData arrays are named "Mesh Triangle Quality,"
 * "Mesh Quadrilateral Quality," "Mesh Tetrahedron Quality,"
 * and "Mesh Hexahedron Quality." Each array has a single tuple
 * with 5 components. The first 4 components are the quality statistics
 * mentioned above; the final value is the number of cells of the given type.
 * This final component makes aggregation of statistics for distributed
 * mesh data possible.
 *
 * By default, the per-cell quality is added to the mesh's cell data, in
 * an array named "Quality." Cell types not supported by
 * this filter will have an entry of 0. Use SaveCellQualityOff() to
 * store only the final statistics.
 *
 * This version of the filter written by Philippe Pebay and David Thompson
 * overtakes an older version written by Leila Baghdadi, Hanif Ladak, and
 * David Steinman at the Imaging Research Labs, Robarts Research Institute.
 * That version only supported tetrahedral radius ratio. See the
 * CompatibilityModeOn() member for information on how to make this filter
 * behave like the previous implementation.
 * For more information on the triangle quality functions of this class, cf.
 * Pebay & Baker 2003, Analysis of triangle quality measures, Math Comp 72:244.
 * For more information on the quadrangle quality functions of this class, cf.
 * Pebay 2004, Planar Quadrangle Quality Measures, Eng Comp 20:2.
 *
 * @warning
 * While more general than before, this class does not address many
 * cell types, including wedges and pyramids in 3D and triangle strips
 * and fans in 2D (among others).
 * Most quadrilateral quality functions are intended for planar quadrilaterals
 * only.
 * The minimal angle is not, strictly speaking, a quality function, but it is
 * provided because of its usage by many authors.
 */

#ifndef svtkMeshQuality_h
#define svtkMeshQuality_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersVerdictModule.h" // For export macro

class svtkCell;
class svtkDataArray;

#define SVTK_QUALITY_EDGE_RATIO 0
#define SVTK_QUALITY_ASPECT_RATIO 1
#define SVTK_QUALITY_RADIUS_RATIO 2
#define SVTK_QUALITY_ASPECT_FROBENIUS 3
#define SVTK_QUALITY_MED_ASPECT_FROBENIUS 4
#define SVTK_QUALITY_MAX_ASPECT_FROBENIUS 5
#define SVTK_QUALITY_MIN_ANGLE 6
#define SVTK_QUALITY_COLLAPSE_RATIO 7
#define SVTK_QUALITY_MAX_ANGLE 8
#define SVTK_QUALITY_CONDITION 9
#define SVTK_QUALITY_SCALED_JACOBIAN 10
#define SVTK_QUALITY_SHEAR 11
#define SVTK_QUALITY_RELATIVE_SIZE_SQUARED 12
#define SVTK_QUALITY_SHAPE 13
#define SVTK_QUALITY_SHAPE_AND_SIZE 14
#define SVTK_QUALITY_DISTORTION 15
#define SVTK_QUALITY_MAX_EDGE_RATIO 16
#define SVTK_QUALITY_SKEW 17
#define SVTK_QUALITY_TAPER 18
#define SVTK_QUALITY_VOLUME 19
#define SVTK_QUALITY_STRETCH 20
#define SVTK_QUALITY_DIAGONAL 21
#define SVTK_QUALITY_DIMENSION 22
#define SVTK_QUALITY_ODDY 23
#define SVTK_QUALITY_SHEAR_AND_SIZE 24
#define SVTK_QUALITY_JACOBIAN 25
#define SVTK_QUALITY_WARPAGE 26
#define SVTK_QUALITY_ASPECT_GAMMA 27
#define SVTK_QUALITY_AREA 28
#define SVTK_QUALITY_ASPECT_BETA 29

class SVTKFILTERSVERDICT_EXPORT svtkMeshQuality : public svtkDataSetAlgorithm
{
public:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkMeshQuality, svtkDataSetAlgorithm);
  static svtkMeshQuality* New();

  //@{
  /**
   * This variable controls whether or not cell quality is stored as
   * cell data in the resulting mesh or discarded (leaving only the
   * aggregate quality average of the entire mesh, recorded in the
   * FieldData).
   */
  svtkSetMacro(SaveCellQuality, svtkTypeBool);
  svtkGetMacro(SaveCellQuality, svtkTypeBool);
  svtkBooleanMacro(SaveCellQuality, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the particular estimator used to function the quality of triangles.
   * The default is SVTK_QUALITY_RADIUS_RATIO and valid values also include
   * SVTK_QUALITY_ASPECT_RATIO, SVTK_QUALITY_ASPECT_FROBENIUS, and SVTK_QUALITY_EDGE_RATIO,
   * SVTK_QUALITY_MIN_ANGLE, SVTK_QUALITY_MAX_ANGLE, SVTK_QUALITY_CONDITION,
   * SVTK_QUALITY_SCALED_JACOBIAN, SVTK_QUALITY_RELATIVE_SIZE_SQUARED,
   * SVTK_QUALITY_SHAPE, SVTK_QUALITY_SHAPE_AND_SIZE, and SVTK_QUALITY_DISTORTION.
   */
  svtkSetMacro(TriangleQualityMeasure, int);
  svtkGetMacro(TriangleQualityMeasure, int);
  void SetTriangleQualityMeasureToArea() { this->SetTriangleQualityMeasure(SVTK_QUALITY_AREA); }
  void SetTriangleQualityMeasureToEdgeRatio()
  {
    this->SetTriangleQualityMeasure(SVTK_QUALITY_EDGE_RATIO);
  }
  void SetTriangleQualityMeasureToAspectRatio()
  {
    this->SetTriangleQualityMeasure(SVTK_QUALITY_ASPECT_RATIO);
  }
  void SetTriangleQualityMeasureToRadiusRatio()
  {
    this->SetTriangleQualityMeasure(SVTK_QUALITY_RADIUS_RATIO);
  }
  void SetTriangleQualityMeasureToAspectFrobenius()
  {
    this->SetTriangleQualityMeasure(SVTK_QUALITY_ASPECT_FROBENIUS);
  }
  void SetTriangleQualityMeasureToMinAngle()
  {
    this->SetTriangleQualityMeasure(SVTK_QUALITY_MIN_ANGLE);
  }
  void SetTriangleQualityMeasureToMaxAngle()
  {
    this->SetTriangleQualityMeasure(SVTK_QUALITY_MAX_ANGLE);
  }
  void SetTriangleQualityMeasureToCondition()
  {
    this->SetTriangleQualityMeasure(SVTK_QUALITY_CONDITION);
  }
  void SetTriangleQualityMeasureToScaledJacobian()
  {
    this->SetTriangleQualityMeasure(SVTK_QUALITY_SCALED_JACOBIAN);
  }
  void SetTriangleQualityMeasureToRelativeSizeSquared()
  {
    this->SetTriangleQualityMeasure(SVTK_QUALITY_RELATIVE_SIZE_SQUARED);
  }
  void SetTriangleQualityMeasureToShape() { this->SetTriangleQualityMeasure(SVTK_QUALITY_SHAPE); }
  void SetTriangleQualityMeasureToShapeAndSize()
  {
    this->SetTriangleQualityMeasure(SVTK_QUALITY_SHAPE_AND_SIZE);
  }
  void SetTriangleQualityMeasureToDistortion()
  {
    this->SetTriangleQualityMeasure(SVTK_QUALITY_DISTORTION);
  }
  //@}

  //@{
  /**
   * Set/Get the particular estimator used to measure the quality of quadrilaterals.
   * The default is SVTK_QUALITY_EDGE_RATIO and valid values also include
   * SVTK_QUALITY_RADIUS_RATIO, SVTK_QUALITY_ASPECT_RATIO, SVTK_QUALITY_MAX_EDGE_RATIO
   * SVTK_QUALITY_SKEW, SVTK_QUALITY_TAPER, SVTK_QUALITY_WARPAGE, SVTK_QUALITY_AREA,
   * SVTK_QUALITY_STRETCH, SVTK_QUALITY_MIN_ANGLE, SVTK_QUALITY_MAX_ANGLE,
   * SVTK_QUALITY_ODDY, SVTK_QUALITY_CONDITION, SVTK_QUALITY_JACOBIAN,
   * SVTK_QUALITY_SCALED_JACOBIAN, SVTK_QUALITY_SHEAR, SVTK_QUALITY_SHAPE,
   * SVTK_QUALITY_RELATIVE_SIZE_SQUARED, SVTK_QUALITY_SHAPE_AND_SIZE,
   * SVTK_QUALITY_SHEAR_AND_SIZE, and SVTK_QUALITY_DISTORTION.

   * Scope: Except for SVTK_QUALITY_EDGE_RATIO, these estimators are intended for planar
   * quadrilaterals only; use at your own risk if you really want to assess non-planar
   * quadrilateral quality with those.
   */
  svtkSetMacro(QuadQualityMeasure, int);
  svtkGetMacro(QuadQualityMeasure, int);
  void SetQuadQualityMeasureToEdgeRatio() { this->SetQuadQualityMeasure(SVTK_QUALITY_EDGE_RATIO); }
  void SetQuadQualityMeasureToAspectRatio()
  {
    this->SetQuadQualityMeasure(SVTK_QUALITY_ASPECT_RATIO);
  }
  void SetQuadQualityMeasureToRadiusRatio()
  {
    this->SetQuadQualityMeasure(SVTK_QUALITY_RADIUS_RATIO);
  }
  void SetQuadQualityMeasureToMedAspectFrobenius()
  {
    this->SetQuadQualityMeasure(SVTK_QUALITY_MED_ASPECT_FROBENIUS);
  }
  void SetQuadQualityMeasureToMaxAspectFrobenius()
  {
    this->SetQuadQualityMeasure(SVTK_QUALITY_MAX_ASPECT_FROBENIUS);
  }
  void SetQuadQualityMeasureToMaxEdgeRatios()
  {
    this->SetQuadQualityMeasure(SVTK_QUALITY_MAX_EDGE_RATIO);
  }
  void SetQuadQualityMeasureToSkew() { this->SetQuadQualityMeasure(SVTK_QUALITY_SKEW); }
  void SetQuadQualityMeasureToTaper() { this->SetQuadQualityMeasure(SVTK_QUALITY_TAPER); }
  void SetQuadQualityMeasureToWarpage() { this->SetQuadQualityMeasure(SVTK_QUALITY_WARPAGE); }
  void SetQuadQualityMeasureToArea() { this->SetQuadQualityMeasure(SVTK_QUALITY_AREA); }
  void SetQuadQualityMeasureToStretch() { this->SetQuadQualityMeasure(SVTK_QUALITY_STRETCH); }
  void SetQuadQualityMeasureToMinAngle() { this->SetQuadQualityMeasure(SVTK_QUALITY_MIN_ANGLE); }
  void SetQuadQualityMeasureToMaxAngle() { this->SetQuadQualityMeasure(SVTK_QUALITY_MAX_ANGLE); }
  void SetQuadQualityMeasureToOddy() { this->SetQuadQualityMeasure(SVTK_QUALITY_ODDY); }
  void SetQuadQualityMeasureToCondition() { this->SetQuadQualityMeasure(SVTK_QUALITY_CONDITION); }
  void SetQuadQualityMeasureToJacobian() { this->SetQuadQualityMeasure(SVTK_QUALITY_JACOBIAN); }
  void SetQuadQualityMeasureToScaledJacobian()
  {
    this->SetQuadQualityMeasure(SVTK_QUALITY_SCALED_JACOBIAN);
  }
  void SetQuadQualityMeasureToShear() { this->SetQuadQualityMeasure(SVTK_QUALITY_SHEAR); }
  void SetQuadQualityMeasureToShape() { this->SetQuadQualityMeasure(SVTK_QUALITY_SHAPE); }
  void SetQuadQualityMeasureToRelativeSizeSquared()
  {
    this->SetQuadQualityMeasure(SVTK_QUALITY_RELATIVE_SIZE_SQUARED);
  }
  void SetQuadQualityMeasureToShapeAndSize()
  {
    this->SetQuadQualityMeasure(SVTK_QUALITY_SHAPE_AND_SIZE);
  }
  void SetQuadQualityMeasureToShearAndSize()
  {
    this->SetQuadQualityMeasure(SVTK_QUALITY_SHEAR_AND_SIZE);
  }
  void SetQuadQualityMeasureToDistortion() { this->SetQuadQualityMeasure(SVTK_QUALITY_DISTORTION); }
  //@}

  //@{
  /**
   * Set/Get the particular estimator used to measure the quality of tetrahedra.
   * The default is SVTK_QUALITY_RADIUS_RATIO (identical to Verdict's aspect
   * ratio beta) and valid values also include
   * SVTK_QUALITY_ASPECT_RATIO, SVTK_QUALITY_ASPECT_FROBENIUS, SVTK_QUALITY_EDGE_RATIO,
   * SVTK_QUALITY_COLLAPSE_RATIO, SVTK_QUALITY_ASPECT_BETA, SVTK_QUALITY_ASPECT_GAMMA,
   * SVTK_QUALITY_VOLUME, SVTK_QUALITY_CONDITION, SVTK_QUALITY_JACOBIAN,
   * SVTK_QUALITY_SCALED_JACOBIAN, SVTK_QUALITY_SHAPE, SVTK_QUALITY_RELATIVE_SIZE_SQUARED,
   * SVTK_QUALITY_SHAPE_AND_SIZE, and SVTK_QUALITY_DISTORTION.
   */
  svtkSetMacro(TetQualityMeasure, int);
  svtkGetMacro(TetQualityMeasure, int);
  void SetTetQualityMeasureToEdgeRatio() { this->SetTetQualityMeasure(SVTK_QUALITY_EDGE_RATIO); }
  void SetTetQualityMeasureToAspectRatio() { this->SetTetQualityMeasure(SVTK_QUALITY_ASPECT_RATIO); }
  void SetTetQualityMeasureToRadiusRatio() { this->SetTetQualityMeasure(SVTK_QUALITY_RADIUS_RATIO); }
  void SetTetQualityMeasureToAspectFrobenius()
  {
    this->SetTetQualityMeasure(SVTK_QUALITY_ASPECT_FROBENIUS);
  }
  void SetTetQualityMeasureToMinAngle() { this->SetTetQualityMeasure(SVTK_QUALITY_MIN_ANGLE); }
  void SetTetQualityMeasureToCollapseRatio()
  {
    this->SetTetQualityMeasure(SVTK_QUALITY_COLLAPSE_RATIO);
  }
  void SetTetQualityMeasureToAspectBeta() { this->SetTetQualityMeasure(SVTK_QUALITY_ASPECT_BETA); }
  void SetTetQualityMeasureToAspectGamma() { this->SetTetQualityMeasure(SVTK_QUALITY_ASPECT_GAMMA); }
  void SetTetQualityMeasureToVolume() { this->SetTetQualityMeasure(SVTK_QUALITY_VOLUME); }
  void SetTetQualityMeasureToCondition() { this->SetTetQualityMeasure(SVTK_QUALITY_CONDITION); }
  void SetTetQualityMeasureToJacobian() { this->SetTetQualityMeasure(SVTK_QUALITY_JACOBIAN); }
  void SetTetQualityMeasureToScaledJacobian()
  {
    this->SetTetQualityMeasure(SVTK_QUALITY_SCALED_JACOBIAN);
  }
  void SetTetQualityMeasureToShape() { this->SetTetQualityMeasure(SVTK_QUALITY_SHAPE); }
  void SetTetQualityMeasureToRelativeSizeSquared()
  {
    this->SetTetQualityMeasure(SVTK_QUALITY_RELATIVE_SIZE_SQUARED);
  }
  void SetTetQualityMeasureToShapeAndSize()
  {
    this->SetTetQualityMeasure(SVTK_QUALITY_SHAPE_AND_SIZE);
  }
  void SetTetQualityMeasureToDistortion() { this->SetTetQualityMeasure(SVTK_QUALITY_DISTORTION); }
  //@}

  //@{
  /**
   * Set/Get the particular estimator used to measure the quality of hexahedra.
   * The default is SVTK_QUALITY_MAX_ASPECT_FROBENIUS and valid values also include
   * SVTK_QUALITY_EDGE_RATIO, SVTK_QUALITY_MAX_ASPECT_FROBENIUS,
   * SVTK_QUALITY_MAX_EDGE_RATIO, SVTK_QUALITY_SKEW, SVTK_QUALITY_TAPER, SVTK_QUALITY_VOLUME,
   * SVTK_QUALITY_STRETCH, SVTK_QUALITY_DIAGONAL, SVTK_QUALITY_DIMENSION,
   * SVTK_QUALITY_ODDY, SVTK_QUALITY_CONDITION, SVTK_QUALITY_JACOBIAN,
   * SVTK_QUALITY_SCALED_JACOBIAN, SVTK_QUALITY_SHEAR, SVTK_QUALITY_SHAPE,
   * SVTK_QUALITY_RELATIVE_SIZE_SQUARED, SVTK_QUALITY_SHAPE_AND_SIZE,
   * SVTK_QUALITY_SHEAR_AND_SIZE, and SVTK_QUALITY_DISTORTION.
   */
  svtkSetMacro(HexQualityMeasure, int);
  svtkGetMacro(HexQualityMeasure, int);
  void SetHexQualityMeasureToEdgeRatio() { this->SetHexQualityMeasure(SVTK_QUALITY_EDGE_RATIO); }
  void SetHexQualityMeasureToMedAspectFrobenius()
  {
    this->SetHexQualityMeasure(SVTK_QUALITY_MED_ASPECT_FROBENIUS);
  }
  void SetHexQualityMeasureToMaxAspectFrobenius()
  {
    this->SetHexQualityMeasure(SVTK_QUALITY_MAX_ASPECT_FROBENIUS);
  }
  void SetHexQualityMeasureToMaxEdgeRatios()
  {
    this->SetHexQualityMeasure(SVTK_QUALITY_MAX_EDGE_RATIO);
  }
  void SetHexQualityMeasureToSkew() { this->SetHexQualityMeasure(SVTK_QUALITY_SKEW); }
  void SetHexQualityMeasureToTaper() { this->SetHexQualityMeasure(SVTK_QUALITY_TAPER); }
  void SetHexQualityMeasureToVolume() { this->SetHexQualityMeasure(SVTK_QUALITY_VOLUME); }
  void SetHexQualityMeasureToStretch() { this->SetHexQualityMeasure(SVTK_QUALITY_STRETCH); }
  void SetHexQualityMeasureToDiagonal() { this->SetHexQualityMeasure(SVTK_QUALITY_DIAGONAL); }
  void SetHexQualityMeasureToDimension() { this->SetHexQualityMeasure(SVTK_QUALITY_DIMENSION); }
  void SetHexQualityMeasureToOddy() { this->SetHexQualityMeasure(SVTK_QUALITY_ODDY); }
  void SetHexQualityMeasureToCondition() { this->SetHexQualityMeasure(SVTK_QUALITY_CONDITION); }
  void SetHexQualityMeasureToJacobian() { this->SetHexQualityMeasure(SVTK_QUALITY_JACOBIAN); }
  void SetHexQualityMeasureToScaledJacobian()
  {
    this->SetHexQualityMeasure(SVTK_QUALITY_SCALED_JACOBIAN);
  }
  void SetHexQualityMeasureToShear() { this->SetHexQualityMeasure(SVTK_QUALITY_SHEAR); }
  void SetHexQualityMeasureToShape() { this->SetHexQualityMeasure(SVTK_QUALITY_SHAPE); }
  void SetHexQualityMeasureToRelativeSizeSquared()
  {
    this->SetHexQualityMeasure(SVTK_QUALITY_RELATIVE_SIZE_SQUARED);
  }
  void SetHexQualityMeasureToShapeAndSize()
  {
    this->SetHexQualityMeasure(SVTK_QUALITY_SHAPE_AND_SIZE);
  }
  void SetHexQualityMeasureToShearAndSize()
  {
    this->SetHexQualityMeasure(SVTK_QUALITY_SHEAR_AND_SIZE);
  }
  void SetHexQualityMeasureToDistortion() { this->SetHexQualityMeasure(SVTK_QUALITY_DISTORTION); }
  //@}

  /**
   * This is a static function used to calculate the area of a triangle.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   */
  static double TriangleArea(svtkCell* cell);

  /**
   * This is a static function used to calculate the edge ratio of a triangle.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   * The edge ratio of a triangle \f$t\f$ is:
   * \f$\frac{|t|_\infty}{|t|_0}\f$,
   * where \f$|t|_\infty\f$ and \f$|t|_0\f$ respectively denote the greatest and
   * the smallest edge lengths of \f$t\f$.
   */
  static double TriangleEdgeRatio(svtkCell* cell);

  /**
   * This is a static function used to calculate the aspect ratio of a triangle.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   * The aspect ratio of a triangle \f$t\f$ is:
   * \f$\frac{|t|_\infty}{2\sqrt{3}r}\f$,
   * where \f$|t|_\infty\f$ and \f$r\f$ respectively denote the greatest edge
   * length and the inradius of \f$t\f$.
   */
  static double TriangleAspectRatio(svtkCell* cell);

  /**
   * This is a static function used to calculate the radius ratio of a triangle.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   * The radius ratio of a triangle \f$t\f$ is:
   * \f$\frac{R}{2r}\f$,
   * where \f$R\f$ and \f$r\f$ respectively denote the circumradius and
   * the inradius of \f$t\f$.
   */
  static double TriangleRadiusRatio(svtkCell* cell);

  /**
   * This is a static function used to calculate the Frobenius condition number
   * of the transformation matrix from an equilateral triangle to a triangle.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   * The Frobenius aspect of a triangle \f$t\f$, when the reference element is
   * equilateral, is:
   * \f$\frac{|t|^2_2}{2\sqrt{3}{\cal A}}\f$,
   * where \f$|t|^2_2\f$ and \f$\cal A\f$ respectively denote the sum of the
   * squared edge lengths and the area of \f$t\f$.
   */
  static double TriangleAspectFrobenius(svtkCell* cell);

  /**
   * This is a static function used to calculate the minimal (nonoriented) angle
   * of a triangle, expressed in degrees.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   */
  static double TriangleMinAngle(svtkCell* cell);

  /**
   * This is a static function used to calculate the maximal (nonoriented) angle
   * of a triangle, expressed in degrees.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   */
  static double TriangleMaxAngle(svtkCell* cell);

  /**
   * This is a static function used to calculate the condition number
   * of a triangle.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   */
  static double TriangleCondition(svtkCell* cell);

  /**
   * This is a static function used to calculate the scaled Jacobian of a triangle.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   */
  static double TriangleScaledJacobian(svtkCell* cell);

  /**
   * This is a static function used to calculate the square of the relative size of a triangle.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   */
  static double TriangleRelativeSizeSquared(svtkCell* cell);

  /**
   * This is a static function used to calculate the shape of a triangle.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   */
  static double TriangleShape(svtkCell* cell);

  /**
   * This is a static function used to calculate the product of shape and relative size of a
   * triangle. It assumes that you pass the correct type of cell -- no type checking is performed
   * because this method is called from the inner loop of the Execute() member function.
   */
  static double TriangleShapeAndSize(svtkCell* cell);

  /**
   * This is a static function used to calculate the distortion of a triangle.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   */
  static double TriangleDistortion(svtkCell* cell);

  /**
   * This is a static function used to calculate the edge ratio of a quadrilateral.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   * The edge ratio of a quadrilateral \f$q\f$ is:
   * \f$\frac{|q|_\infty}{|q|_0}\f$,
   * where \f$|q|_\infty\f$ and \f$|q|_0\f$ respectively denote the greatest and
   * the smallest edge lengths of \f$q\f$.
   */
  static double QuadEdgeRatio(svtkCell* cell);

  /**
   * This is a static function used to calculate the aspect ratio of a planar
   * quadrilateral.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function. Use at your own risk with nonplanar quadrilaterals.
   * The aspect ratio of a planar quadrilateral \f$q\f$ is:
   * \f$\frac{|q|_1|q|_\infty}{4{\cal A}}\f$,
   * where \f$|q|_1\f$, \f$|q|_\infty\f$ and \f${\cal A}\f$ respectively denote the
   * perimeter, the greatest edge length and the area of \f$q\f$.
   */
  static double QuadAspectRatio(svtkCell* cell);

  /**
   * This is a static function used to calculate the radius ratio of a planar
   * quadrilateral. The name is only used by analogy with the triangle radius
   * ratio, because in general a quadrilateral does not have a circumcircle nor
   * an incircle.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function. Use at your own risk with nonplanar quadrilaterals.
   * The radius ratio of a planar quadrilateral \f$q\f$ is:
   * \f$\frac{|q|_2h_{\max}}{\min_i{\cal A}_i}\f$,
   * where \f$|q|_2\f$, \f$h_{\max}\f$ and \f$\min{\cal A}_i\f$ respectively denote
   * the sum of the squared edge lengths, the greatest amongst diagonal and edge
   * lengths and the smallest area of the 4 triangles extractable from \f$q\f$.
   */
  static double QuadRadiusRatio(svtkCell* cell);

  /**
   * This is a static function used to calculate the average Frobenius aspect of
   * the 4 corner triangles of a planar quadrilateral, when the reference
   * triangle elements are right isosceles at the quadrangle vertices.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function. Use at your own risk with nonplanar quadrilaterals.
   * The Frobenius aspect of a triangle \f$t\f$, when the reference element is
   * right isosceles at vertex \f$V\f$, is:
   * \f$\frac{f^2+g^2}{4{\cal A}}\f$,
   * where \f$f^2+g^2\f$ and \f$\cal A\f$ respectively denote the sum of the
   * squared lengths of the edges attached to \f$V\f$ and the area of \f$t\f$.
   */
  static double QuadMedAspectFrobenius(svtkCell* cell);

  /**
   * This is a static function used to calculate the maximal Frobenius aspect of
   * the 4 corner triangles of a planar quadrilateral, when the reference
   * triangle elements are right isosceles at the quadrangle vertices.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function. Use at your own risk with nonplanar quadrilaterals.
   * The Frobenius aspect of a triangle \f$t\f$, when the reference element is
   * right isosceles at vertex \f$V\f$, is:
   * \f$\frac{f^2+g^2}{4{\cal A}}\f$,
   * where \f$f^2+g^2\f$ and \f$\cal A\f$ respectively denote the sum of the
   * squared lengths of the edges attached to \f$V\f$ and the area of \f$t\f$.
   */
  static double QuadMaxAspectFrobenius(svtkCell* cell);

  /**
   * This is a static function used to calculate the minimal (nonoriented) angle
   * of a quadrilateral, expressed in degrees.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   */
  static double QuadMinAngle(svtkCell* cell);

  static double QuadMaxEdgeRatios(svtkCell* cell);
  static double QuadSkew(svtkCell* cell);
  static double QuadTaper(svtkCell* cell);
  static double QuadWarpage(svtkCell* cell);
  static double QuadArea(svtkCell* cell);
  static double QuadStretch(svtkCell* cell);
  static double QuadMaxAngle(svtkCell* cell);
  static double QuadOddy(svtkCell* cell);
  static double QuadCondition(svtkCell* cell);
  static double QuadJacobian(svtkCell* cell);
  static double QuadScaledJacobian(svtkCell* cell);
  static double QuadShear(svtkCell* cell);
  static double QuadShape(svtkCell* cell);
  static double QuadRelativeSizeSquared(svtkCell* cell);
  static double QuadShapeAndSize(svtkCell* cell);
  static double QuadShearAndSize(svtkCell* cell);
  static double QuadDistortion(svtkCell* cell);

  /**
   * This is a static function used to calculate the edge ratio of a tetrahedron.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   * The edge ratio of a tetrahedron \f$K\f$ is:
   * \f$\frac{|K|_\infty}{|K|_0}\f$,
   * where \f$|K|_\infty\f$ and \f$|K|_0\f$ respectively denote the greatest and
   * the smallest edge lengths of \f$K\f$.
   */
  static double TetEdgeRatio(svtkCell* cell);

  /**
   * This is a static function used to calculate the aspect ratio of a tetrahedron.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   * The aspect ratio of a tetrahedron \f$K\f$ is:
   * \f$\frac{|K|_\infty}{2\sqrt{6}r}\f$,
   * where \f$|K|_\infty\f$ and \f$r\f$ respectively denote the greatest edge
   * length and the inradius of \f$K\f$.
   */
  static double TetAspectRatio(svtkCell* cell);

  /**
   * This is a static function used to calculate the radius ratio of a tetrahedron.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   * The radius ratio of a tetrahedron \f$K\f$ is:
   * \f$\frac{R}{3r}\f$,
   * where \f$R\f$ and \f$r\f$ respectively denote the circumradius and
   * the inradius of \f$K\f$.
   */
  static double TetRadiusRatio(svtkCell* cell);

  /**
   * This is a static function used to calculate the Frobenius condition number
   * of the transformation matrix from a regular tetrahedron to a tetrahedron.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   * The Frobenius aspect of a tetrahedron \f$K\f$, when the reference element is
   * regular, is:
   * \f$\frac{\frac{3}{2}(l_{11}+l_{22}+l_{33}) - (l_{12}+l_{13}+l_{23})}
   * {3(\sqrt{2}\det{T})^\frac{2}{3}}\f$,
   * where \f$T\f$ and \f$l_{ij}\f$ respectively denote the edge matrix of \f$K\f$
   * and the entries of \f$L=T^t\,T\f$.
   */
  static double TetAspectFrobenius(svtkCell* cell);

  /**
   * This is a static function used to calculate the minimal (nonoriented) dihedral
   * angle of a tetrahedron, expressed in degrees.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   */
  static double TetMinAngle(svtkCell* cell);

  //@{
  /**
   * This is a static function used to calculate the collapse ratio of a tetrahedron.
   * The collapse ratio is a dimensionless number defined as the smallest ratio of the
   * height of a vertex above its opposing triangle to the longest edge of that opposing
   * triangle across all vertices of the tetrahedron.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   */
  static double TetCollapseRatio(svtkCell* cell);
  static double TetAspectBeta(svtkCell* cell);
  static double TetAspectGamma(svtkCell* cell);
  static double TetVolume(svtkCell* cell);
  static double TetCondition(svtkCell* cell);
  static double TetJacobian(svtkCell* cell);
  static double TetScaledJacobian(svtkCell* cell);
  static double TetShape(svtkCell* cell);
  static double TetRelativeSizeSquared(svtkCell* cell);
  static double TetShapeandSize(svtkCell* cell);
  static double TetDistortion(svtkCell* cell);
  //@}

  /**
   * This is a static function used to calculate the edge ratio of a hexahedron.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   * The edge ratio of a hexahedron \f$H\f$ is:
   * \f$\frac{|H|_\infty}{|H|_0}\f$,
   * where \f$|H|_\infty\f$ and \f$|H|_0\f$ respectively denote the greatest and
   * the smallest edge lengths of \f$H\f$.
   */
  static double HexEdgeRatio(svtkCell* cell);

  /**
   * This is a static function used to calculate the average Frobenius aspect of
   * the 8 corner tetrahedra of a hexahedron, when the reference
   * tetrahedral elements are right isosceles at the hexahedron vertices.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   */
  static double HexMedAspectFrobenius(svtkCell* cell);

  //@{
  /**
   * This is a static function used to calculate the maximal Frobenius aspect of
   * the 8 corner tetrahedra of a hexahedron, when the reference
   * tetrahedral elements are right isosceles at the hexahedron vertices.
   * It assumes that you pass the correct type of cell -- no type checking is
   * performed because this method is called from the inner loop of the Execute()
   * member function.
   */
  static double HexMaxAspectFrobenius(svtkCell* cell);
  static double HexMaxEdgeRatio(svtkCell* cell);
  static double HexSkew(svtkCell* cell);
  static double HexTaper(svtkCell* cell);
  static double HexVolume(svtkCell* cell);
  static double HexStretch(svtkCell* cell);
  static double HexDiagonal(svtkCell* cell);
  static double HexDimension(svtkCell* cell);
  static double HexOddy(svtkCell* cell);
  static double HexCondition(svtkCell* cell);
  static double HexJacobian(svtkCell* cell);
  static double HexScaledJacobian(svtkCell* cell);
  static double HexShear(svtkCell* cell);
  static double HexShape(svtkCell* cell);
  static double HexRelativeSizeSquared(svtkCell* cell);
  static double HexShapeAndSize(svtkCell* cell);
  static double HexShearAndSize(svtkCell* cell);
  static double HexDistortion(svtkCell* cell);
  //@}

  /**
   * These methods are deprecated. Use Get/SetSaveCellQuality() instead.

   * Formerly, SetRatio could be used to disable computation
   * of the tetrahedral radius ratio so that volume alone could be computed.
   * Now, cell quality is always computed, but you may decide not
   * to store the result for each cell.
   * This allows average cell quality of a mesh to be
   * calculated without requiring per-cell storage.
   */
  virtual void SetRatio(svtkTypeBool r) { this->SetSaveCellQuality(r); }
  svtkTypeBool GetRatio() { return this->GetSaveCellQuality(); }
  svtkBooleanMacro(Ratio, svtkTypeBool);

  //@{
  /**
   * These methods are deprecated. The functionality of computing cell
   * volume is being removed until it can be computed for any 3D cell.
   * (The previous implementation only worked for tetrahedra.)

   * For now, turning on the volume computation will put this
   * filter into "compatibility mode," where tetrahedral cell
   * volume is stored in first component of each output tuple and
   * the radius ratio is stored in the second component. You may
   * also use CompatibilityModeOn()/Off() to enter this mode.
   * In this mode, cells other than tetrahedra will have report
   * a volume of 0.0 (if volume computation is enabled).

   * By default, volume computation is disabled and compatibility
   * mode is off, since it does not make a lot of sense for
   * meshes with non-tetrahedral cells.
   */
  virtual void SetVolume(svtkTypeBool cv)
  {
    if (!((cv != 0) ^ (this->Volume != 0)))
    {
      return;
    }
    this->Modified();
    this->Volume = cv;
    if (this->Volume)
    {
      this->CompatibilityModeOn();
    }
  }
  svtkTypeBool GetVolume() { return this->Volume; }
  svtkBooleanMacro(Volume, svtkTypeBool);
  //@}

  //@{
  /**
   * CompatibilityMode governs whether, when both a quality function
   * and cell volume are to be stored as cell data, the two values
   * are stored in a single array. When compatibility mode is off
   * (the default), two separate arrays are used -- one labeled
   * "Quality" and the other labeled "Volume".
   * When compatibility mode is on, both values are stored in a
   * single array, with volume as the first component and quality
   * as the second component.

   * Enabling CompatibilityMode changes the default tetrahedral
   * quality function to SVTK_QUALITY_RADIUS_RATIO and turns volume
   * computation on. (This matches the default behavior of the
   * initial implementation of svtkMeshQuality.) You may change
   * quality function and volume computation without leaving
   * compatibility mode.

   * Disabling compatibility mode does not affect the current
   * volume computation or tetrahedral quality function settings.

   * The final caveat to CompatibilityMode is that regardless of
   * its setting, the resulting array will be of type svtkDoubleArray
   * rather than the original svtkFloatArray.
   * This is a safety function to keep the authors from
   * diving off of the Combinatorial Coding Cliff into
   * Certain Insanity.
   */
  virtual void SetCompatibilityMode(svtkTypeBool cm)
  {
    if (!((cm != 0) ^ (this->CompatibilityMode != 0)))
    {
      return;
    }
    this->CompatibilityMode = cm;
    this->Modified();
    if (this->CompatibilityMode)
    {
      this->Volume = 1;
      this->TetQualityMeasure = SVTK_QUALITY_RADIUS_RATIO;
    }
  }
  svtkGetMacro(CompatibilityMode, svtkTypeBool);
  svtkBooleanMacro(CompatibilityMode, svtkTypeBool);
  //@}

protected:
  svtkMeshQuality();
  ~svtkMeshQuality() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * A function called by some VERDICT triangle quality functions to test for inverted triangles.
   */
  static int GetCurrentTriangleNormal(double point[3], double normal[3]);

  svtkTypeBool SaveCellQuality;
  int TriangleQualityMeasure;
  int QuadQualityMeasure;
  int TetQualityMeasure;
  int HexQualityMeasure;

  svtkTypeBool CompatibilityMode;
  svtkTypeBool Volume;

  svtkDataArray* CellNormals;
  static double CurrentTriNormal[3];

private:
  svtkMeshQuality(const svtkMeshQuality&) = delete;
  void operator=(const svtkMeshQuality&) = delete;
};

#endif // svtkMeshQuality_h
