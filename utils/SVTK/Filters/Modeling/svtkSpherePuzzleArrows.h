/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSpherePuzzleArrows.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSpherePuzzleArrows
 * @brief   Visualize permutation of the sphere puzzle.
 *
 * svtkSpherePuzzleArrows creates
 */

#ifndef svtkSpherePuzzleArrows_h
#define svtkSpherePuzzleArrows_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkCellArray;
class svtkPoints;
class svtkSpherePuzzle;

class SVTKFILTERSMODELING_EXPORT svtkSpherePuzzleArrows : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkSpherePuzzleArrows, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkSpherePuzzleArrows* New();

  //@{
  /**
   * Permutation is an array of puzzle piece ids.
   * Arrows will be generated for any id that does not contain itself.
   * Permutation[3] = 3 will produce no arrow.
   * Permutation[3] = 10 will draw an arrow from location 3 to 10.
   */
  svtkSetVectorMacro(Permutation, int, 32);
  svtkGetVectorMacro(Permutation, int, 32);
  void SetPermutationComponent(int comp, int val);
  void SetPermutation(svtkSpherePuzzle* puz);
  //@}

protected:
  svtkSpherePuzzleArrows();
  ~svtkSpherePuzzleArrows() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void AppendArrow(int id1, int id2, svtkPoints* pts, svtkCellArray* polys);

  int Permutation[32];

  double Radius;

private:
  svtkSpherePuzzleArrows(const svtkSpherePuzzleArrows&) = delete;
  void operator=(const svtkSpherePuzzleArrows&) = delete;
};

#endif
