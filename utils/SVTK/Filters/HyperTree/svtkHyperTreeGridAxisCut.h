/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridAxisCut.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHyperTreeGridAxisCut
 * @brief   Axis aligned hyper tree grid cut
 *
 *
 * Cut an hyper tree grid along an axis aligned plane and output a hyper
 * tree grid lower dimensionality. Only works for 3D grids as inputs
 *
 * NB: This new (2014-16) version of the class is not to be confused with
 * earlier (2012-13) version that produced a svtkPolyData output composed of
 * disjoint (no point sharing) quadrilaterals, with possibly superimposed
 * faces when cut plane contained inter-cell boundaries.
 *
 * @sa
 * svtkHyperTreeGrid svtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was modified by Philippe Pebay, 2016
 * This class was modified by Jacques-Bernard Lekien, 2018
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkHyperTreeGridAxisCut_h
#define svtkHyperTreeGridAxisCut_h

#include "svtkFiltersHyperTreeModule.h" // For export macro
#include "svtkHyperTreeGridAlgorithm.h"

class svtkBitArray;
class svtkHyperTreeGrid;
class svtkHyperTreeGridNonOrientedCursor;
class svtkHyperTreeGridNonOrientedGeometryCursor;

class SVTKFILTERSHYPERTREE_EXPORT svtkHyperTreeGridAxisCut : public svtkHyperTreeGridAlgorithm
{
public:
  static svtkHyperTreeGridAxisCut* New();
  svtkTypeMacro(svtkHyperTreeGridAxisCut, svtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream&, svtkIndent) override;

  //@{
  /**
   * Normal axis: 0=X, 1=Y, 2=Z. Default is 0
   */
  svtkSetClampMacro(PlaneNormalAxis, int, 0, 2);
  svtkGetMacro(PlaneNormalAxis, int);
  //@}

  //@{
  /**
   * Position of plane: Axis constant. Default is 0.0
   */
  svtkSetMacro(PlanePosition, double);
  svtkGetMacro(PlanePosition, double);
  //@}

protected:
  svtkHyperTreeGridAxisCut();
  ~svtkHyperTreeGridAxisCut() override;

  // For this algorithm the output is a svtkHyperTreeGrid instance
  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * Main routine to generate hyper tree grid cut
   */
  int ProcessTrees(svtkHyperTreeGrid*, svtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree(svtkHyperTreeGridNonOrientedGeometryCursor* inCursor,
    svtkHyperTreeGridNonOrientedCursor* outCursor);

  /**
   * Direction of plane normal
   */
  int PlaneNormalAxis;

  /**
   * Intercept of plane along normal
   */
  double PlanePosition;
  double PlanePositionRealUse;

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
  svtkHyperTreeGridAxisCut(const svtkHyperTreeGridAxisCut&) = delete;
  void operator=(const svtkHyperTreeGridAxisCut&) = delete;
};

#endif // svtkHyperTreeGridAxisCut_h
