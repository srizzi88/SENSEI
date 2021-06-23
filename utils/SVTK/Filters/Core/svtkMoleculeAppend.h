/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMoleculeAppend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

/**
 * @class svtkMoleculeAppend
 * @brief Appends one or more molecules into a single molecule
 *
 * svtkMoleculeAppend appends molecule into a single molecule. It also appends
 * the associated atom data and edge data.
 * Note that input data arrays should match (same number of arrays with same names in each input)
 *
 * Option MergeCoincidentAtoms specifies if coincident atoms should be merged or not.
 * This may be useful in Parallel mode to remove ghost atoms when gather molecule on a rank.
 * When merging, use the data of the non ghost atom. If none, use the data of the last coincident
 * atom. This option is active by default.
 */

#ifndef svtkMoleculeAppend_h
#define svtkMoleculeAppend_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkMoleculeAlgorithm.h"

class SVTKFILTERSCORE_EXPORT svtkMoleculeAppend : public svtkMoleculeAlgorithm
{
public:
  static svtkMoleculeAppend* New();
  svtkTypeMacro(svtkMoleculeAppend, svtkMoleculeAlgorithm);

  //@{
  /**
   * Get one input to this filter. This method is only for support of
   * old-style pipeline connections.  When writing new code you should
   * use svtkAlgorithm::GetInputConnection(0, num).
   */
  svtkDataObject* GetInput(int num);
  svtkDataObject* GetInput() { return this->GetInput(0); }
  //@}

  //@{
  /**
   * Specify if coincident atoms (atom with exactly the same position)
   * should be merged into one.
   * True by default.
   */
  svtkGetMacro(MergeCoincidentAtoms, bool);
  svtkSetMacro(MergeCoincidentAtoms, bool);
  svtkBooleanMacro(MergeCoincidentAtoms, bool);
  // @}

protected:
  svtkMoleculeAppend();
  ~svtkMoleculeAppend() override = default;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // see svtkAlgorithm for docs.
  int FillInputPortInformation(int, svtkInformation*) override;

  // Check arrays information : name, type and number of components.
  bool CheckArrays(svtkAbstractArray* array1, svtkAbstractArray* array2);

  bool MergeCoincidentAtoms;

private:
  svtkMoleculeAppend(const svtkMoleculeAppend&) = delete;
  void operator=(const svtkMoleculeAppend&) = delete;
};

#endif
