/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSpherePuzzle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSpherePuzzle
 * @brief   create a polygonal sphere centered at the origin
 *
 * svtkSpherePuzzle creates
 */

#ifndef svtkSpherePuzzle_h
#define svtkSpherePuzzle_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_MAX_SPHERE_RESOLUTION 1024

class svtkTransform;

class SVTKFILTERSMODELING_EXPORT svtkSpherePuzzle : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkSpherePuzzle, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkSpherePuzzle* New();

  /**
   * Reset the state of this puzzle back to its original state.
   */
  void Reset();

  /**
   * Move the top/bottom half one segment either direction.
   */
  void MoveHorizontal(int section, int percentage, int rightFlag);

  /**
   * Rotate vertical half of sphere along one of the longitude lines.
   */
  void MoveVertical(int section, int percentage, int rightFlag);

  /**
   * SetPoint will be called as the mouse moves over the screen.
   * The output will change to indicate the pending move.
   * SetPoint returns zero if move is not activated by point.
   * Otherwise it encodes the move into a unique integer so that
   * the caller can determine if the move state has changed.
   * This will answer the question, "Should I render."
   */
  int SetPoint(double x, double y, double z);

  /**
   * Move actually implements the pending move. When percentage
   * is 100, the pending move becomes inactive, and SetPoint
   * will have to be called again to setup another move.
   */
  void MovePoint(int percentage);

  /**
   * For drawing state as arrows.
   */
  int* GetState() { return this->State; }

protected:
  svtkSpherePuzzle();
  ~svtkSpherePuzzle() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void MarkVertical(int section);
  void MarkHorizontal(int section);

  int State[32];

  // Stuff for storing a partial move.
  int PieceMask[32];
  svtkTransform* Transform;

  // Colors for faces.
  unsigned char Colors[96];

  // State for potential move.
  int Active;
  int VerticalFlag;
  int RightFlag;
  int Section;

private:
  svtkSpherePuzzle(const svtkSpherePuzzle&) = delete;
  void operator=(const svtkSpherePuzzle&) = delete;
};

#endif
