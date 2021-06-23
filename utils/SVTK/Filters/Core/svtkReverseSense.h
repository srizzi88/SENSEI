/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkReverseSense.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkReverseSense
 * @brief   reverse the ordering of polygonal cells and/or vertex normals
 *
 *
 * svtkReverseSense is a filter that reverses the order of polygonal cells
 * and/or reverses the direction of point and cell normals. Two flags are
 * used to control these operations. Cell reversal means reversing the order
 * of indices in the cell connectivity list. Normal reversal means
 * multiplying the normal vector by -1 (both point and cell normals,
 * if present).
 *
 * @warning
 * Normals can be operated on only if they are present in the data.
 */

#ifndef svtkReverseSense_h
#define svtkReverseSense_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSCORE_EXPORT svtkReverseSense : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkReverseSense, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object so that behavior is to reverse cell ordering and
   * leave normal orientation as is.
   */
  static svtkReverseSense* New();

  //@{
  /**
   * Flag controls whether to reverse cell ordering.
   */
  svtkSetMacro(ReverseCells, svtkTypeBool);
  svtkGetMacro(ReverseCells, svtkTypeBool);
  svtkBooleanMacro(ReverseCells, svtkTypeBool);
  //@}

  //@{
  /**
   * Flag controls whether to reverse normal orientation.
   */
  svtkSetMacro(ReverseNormals, svtkTypeBool);
  svtkGetMacro(ReverseNormals, svtkTypeBool);
  svtkBooleanMacro(ReverseNormals, svtkTypeBool);
  //@}

protected:
  svtkReverseSense();
  ~svtkReverseSense() override {}

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkTypeBool ReverseCells;
  svtkTypeBool ReverseNormals;

private:
  svtkReverseSense(const svtkReverseSense&) = delete;
  void operator=(const svtkReverseSense&) = delete;
};

#endif
