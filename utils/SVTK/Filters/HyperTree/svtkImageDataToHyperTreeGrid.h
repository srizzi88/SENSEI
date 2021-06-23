/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDataToHyperTreeGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageDataToHyperTreeGrid
 * @brief
 *
 *
 * @sa
 * svtkHyperTreeGrid svtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkImageDataToHyperTreeGrid_h
#define svtkImageDataToHyperTreeGrid_h

#include "svtkFiltersHyperTreeModule.h" // For export macro
#include "svtkHyperTreeGridAlgorithm.h"

class svtkBitArray;
class svtkIntArray;
class svtkUnsignedCharArray;
class svtkDoubleArray;
class svtkHyperTreeGrid;
class svtkHyperTreeGridNonOrientedCursor;

class SVTKFILTERSHYPERTREE_EXPORT svtkImageDataToHyperTreeGrid : public svtkHyperTreeGridAlgorithm
{
public:
  static svtkImageDataToHyperTreeGrid* New();
  svtkTypeMacro(svtkImageDataToHyperTreeGrid, svtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream&, svtkIndent) override;

  svtkSetMacro(DepthMax, int);
  svtkGetMacro(DepthMax, int);

  svtkSetMacro(NbColors, int);
  svtkGetMacro(NbColors, int);

protected:
  svtkImageDataToHyperTreeGrid();
  ~svtkImageDataToHyperTreeGrid() override;

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  virtual int ProcessTrees(svtkHyperTreeGrid*, svtkDataObject*) override;

  void ProcessPixels(svtkIntArray*, svtkHyperTreeGridNonOrientedCursor*);

  int FillInputPortInformation(int, svtkInformation*) override;
  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkImageDataToHyperTreeGrid(const svtkImageDataToHyperTreeGrid&) = delete;
  void operator=(const svtkImageDataToHyperTreeGrid&) = delete;

  int DepthMax;
  int NbColors;

  svtkDataArray* InScalars;

  svtkUnsignedCharArray* Color;
  svtkDoubleArray* Depth;
  svtkBitArray* Mask;
  int GlobalId;
};

#endif // svtkImageDataToHyperTreeGrid_h
