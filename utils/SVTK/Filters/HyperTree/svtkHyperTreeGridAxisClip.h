/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridAxisClip.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHyperTreeGridAxisClip
 * @brief   Axis aligned hyper tree grid clip
 *
 *
 * Clip an hyper tree grid along an axis aligned plane or box and output
 * a hyper tree grid with same dimensionality.
 * This filter also allows for reversal of the direction of what is inside
 * versus what is outside by setting the InsideOut instance variable.
 *
 * @sa
 * svtkHyperTreeGrid svtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Philippe Pebay on a idea of Guenole Harel and Jacques-Bernard Lekien,
 * 2016 This class was modified by Jacques-Bernard Lekien, 2018 This work was supported by
 * Commissariat a l'Energie Atomique CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkHyperTreeGridAxisClip_h
#define svtkHyperTreeGridAxisClip_h

#include "svtkFiltersHyperTreeModule.h" // For export macro
#include "svtkHyperTreeGridAlgorithm.h"

class svtkBitArray;
class svtkHyperTreeGrid;
class svtkQuadric;
class svtkHyperTreeGridNonOrientedCursor;
class svtkHyperTreeGridNonOrientedGeometryCursor;

class SVTKFILTERSHYPERTREE_EXPORT svtkHyperTreeGridAxisClip : public svtkHyperTreeGridAlgorithm
{
public:
  static svtkHyperTreeGridAxisClip* New();
  svtkTypeMacro(svtkHyperTreeGridAxisClip, svtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream&, svtkIndent) override;

  /**
   * Methods by which the hyper tree grid input may be clipped:
   * PLANE: Clip with an axis-aligned plane defined by normal and intercept.
   * BOX: Clip with an axis-aligned rectangular prism defined by its extremal coordinates.
   * QUADRIC: Clip with an axis-aligned quadric defined by its coefficients.
   */
  enum ClipType
  {
    PLANE = 0,
    BOX,
    QUADRIC,
  };

  //@{
  /**
   * Set/get type of clip.
   * Default value is 0 (plane clip).
   */
  svtkSetClampMacro(ClipType, int, 0, 2);
  svtkGetMacro(ClipType, int);
  void SetClipTypeToPlane() { this->SetClipType(svtkHyperTreeGridAxisClip::PLANE); }
  void SetClipTypeToBox() { this->SetClipType(svtkHyperTreeGridAxisClip::BOX); }
  void SetClipTypeToQuadric() { this->SetClipType(svtkHyperTreeGridAxisClip::QUADRIC); }
  //@}

  //@{
  /**
   * Set/get normal axis of clipping plane: 0=X, 1=Y, 2=Z.
   * Default value is 0 (X-axis normal).
   */
  svtkSetClampMacro(PlaneNormalAxis, int, 0, 2);
  svtkGetMacro(PlaneNormalAxis, int);
  //@}

  //@{
  /**
   * Set/get position of clipping plane: intercept along normal axis.
   * Default value is 0.0.
   */
  svtkSetMacro(PlanePosition, double);
  svtkGetMacro(PlanePosition, double);
  //@}

  //@{
  /**
   * Set/get bounds of clipping box.
   */
  svtkSetVector6Macro(Bounds, double);
  svtkGetVectorMacro(Bounds, double, 6);
  void GetMinimumBounds(double[3]);
  void GetMaximumBounds(double[3]);
  //@}

  //@{
  /**
   * Set/Get the InsideOut flag, in the case of clip by hyperplane.
   * When off, a cell is clipped out when its origin is above said plane
   * intercept along the considered direction, inside otherwise.
   * When on, a cell is clipped out when its origin + size is below said
   * said plane intercept along the considered direction.
   */
  svtkSetMacro(InsideOut, bool);
  svtkGetMacro(InsideOut, bool);
  svtkBooleanMacro(InsideOut, bool);
  //@}

  //@{
  /**
   * Set/Get the clipping quadric function.
   */
  virtual void SetQuadric(svtkQuadric*);
  svtkGetObjectMacro(Quadric, svtkQuadric);
  //@}

  //@{
  /**
   * Helpers to set/get the 10 coefficients of the quadric function
   */
  void SetQuadricCoefficients(double a, double b, double c, double d, double e, double f, double g,
    double h, double i, double j)
  {
    double array[10] = { a, b, c, d, e, f, g, h, i, j };
    this->SetQuadricCoefficients(array);
  }
  void SetQuadricCoefficients(double[10]);
  void GetQuadricCoefficients(double[10]);
  double* GetQuadricCoefficients();
  //@}

  /**
   * Override GetMTime because we delegate to a svtkQuadric.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkHyperTreeGridAxisClip();
  ~svtkHyperTreeGridAxisClip() override;

  // For this algorithm the output is a svtkHyperTreeGrid instance
  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * Decide whether the cell is clipped out
   */
  bool IsClipped(svtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Main routine to generate hyper tree grid clip
   */
  int ProcessTrees(svtkHyperTreeGrid*, svtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree(svtkHyperTreeGridNonOrientedGeometryCursor* inCursor,
    svtkHyperTreeGridNonOrientedCursor* outCursor);

  /**
   * Type of clip to be performed
   */
  int ClipType;

  /**
   * Direction of clipping plane normal
   */
  int PlaneNormalAxis;

  /**
   * Intercept of clipping plane along normal
   */
  double PlanePosition;
  double PlanePositionRealUse;

  /**
   * Bounds of axis-aligned clipping box
   */
  double Bounds[6];

  /**
   * Coefficients of axis-aligned quadric
   */
  svtkQuadric* Quadric;

  /**
   * Decide what is inside versus what is out
   */
  bool InsideOut;

  /**
   * Output material mask constructed by this filter
   */
  svtkBitArray* InMask;
  svtkBitArray* OutMask;

  /**
   * Keep track of current index in output hyper tree grid
   */
  svtkIdType CurrentId;

private:
  svtkHyperTreeGridAxisClip(const svtkHyperTreeGridAxisClip&) = delete;
  void operator=(const svtkHyperTreeGridAxisClip&) = delete;
};

#endif // svtkHyperTreeGridAxisClip_h
