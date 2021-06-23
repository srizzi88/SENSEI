/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridThreshold.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHyperTreeGridThreshold
 * @brief   Extract cells from a hyper tree grid
 * where selected scalar value is within given range.
 *
 *
 * This filter extracts cells from a hyper tree grid that satisfy the
 * following threshold: a cell is considered to be within range if its
 * value for the active scalar is within a specified range (inclusive).
 * The output remains a hyper tree grid.
 * JB Un parametre (JustCreateNewMask=true) permet de ne pas faire
 * le choix de la creation d'un nouveau HTG mais
 * de redefinir juste le masque.
 *
 * @sa
 * svtkHyperTreeGrid svtkHyperTreeGridAlgorithm svtkThreshold
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was revised by Philippe Pebay, 2016
 * This class was optimized by Jacques-Bernard Lekien, 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkHyperTreeGridThreshold_h
#define svtkHyperTreeGridThreshold_h

#include "svtkFiltersHyperTreeModule.h" // For export macro
#include "svtkHyperTreeGridAlgorithm.h"

class svtkBitArray;
class svtkHyperTreeGrid;

class svtkHyperTreeGridNonOrientedCursor;

class SVTKFILTERSHYPERTREE_EXPORT svtkHyperTreeGridThreshold : public svtkHyperTreeGridAlgorithm
{
public:
  static svtkHyperTreeGridThreshold* New();
  svtkTypeMacro(svtkHyperTreeGridThreshold, svtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream&, svtkIndent) override;

  //@{
  /**
   * Set/Get True, create a new mask ; false, create a new HTG.
   */
  svtkSetMacro(JustCreateNewMask, bool);
  svtkGetMacro(JustCreateNewMask, bool);
  //@}

  //@{
  /**
   * Set/Get minimum scalar value of threshold
   */
  svtkSetMacro(LowerThreshold, double);
  svtkGetMacro(LowerThreshold, double);
  //@}

  //@{
  /**
   * Set/Get maximum scalar value of threshold
   */
  svtkSetMacro(UpperThreshold, double);
  svtkGetMacro(UpperThreshold, double);
  //@}

  /**
   * Convenience method to set both threshold values at once
   */
  void ThresholdBetween(double, double);

protected:
  svtkHyperTreeGridThreshold();
  ~svtkHyperTreeGridThreshold() override;

  /**
   * For this algorithm the output is a svtkHyperTreeGrid instance
   */
  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * Main routine to extract cells based on thresholded value
   */
  int ProcessTrees(svtkHyperTreeGrid*, svtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves
   */
  bool RecursivelyProcessTree(
    svtkHyperTreeGridNonOrientedCursor*, svtkHyperTreeGridNonOrientedCursor*);
  bool RecursivelyProcessTreeWithCreateNewMask(svtkHyperTreeGridNonOrientedCursor*);

  /**
   * LowerThreshold scalar value to be accepted
   */
  double LowerThreshold;

  /**
   * UpperThreshold scalar value to be accepted
   */
  double UpperThreshold;

  /**
   * Input material mask
   */
  svtkBitArray* InMask;

  /**
   * Output material mask constructed by this filter
   */
  svtkBitArray* OutMask;

  /**
   * Keep track of current index in output hyper tree grid
   */
  svtkIdType CurrentId;

  /**
   * Keep track of selected input scalars
   */
  svtkDataArray* InScalars;

  /**
   * With or without copy
   */
  bool JustCreateNewMask;

private:
  svtkHyperTreeGridThreshold(const svtkHyperTreeGridThreshold&) = delete;
  void operator=(const svtkHyperTreeGridThreshold&) = delete;
};

#endif /* svtkHyperTreeGridThreshold */
