/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeRingToPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkTreeRingToPolyData
 * @brief   converts a tree to a polygonal data
 * representing radial space filling tree.
 *
 *
 * This algorithm requires that the svtkTreeRingLayout filter has already
 * been applied to the data in order to create the quadruple array
 * (start angle, end angle, inner radius, outer radius) of bounds
 * for each vertex of the tree.
 */

#ifndef svtkTreeRingToPolyData_h
#define svtkTreeRingToPolyData_h

#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKINFOVISLAYOUT_EXPORT svtkTreeRingToPolyData : public svtkPolyDataAlgorithm
{
public:
  static svtkTreeRingToPolyData* New();

  svtkTypeMacro(svtkTreeRingToPolyData, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * The field containing quadruples of the form (start angle, end angle,
   * inner radius, outer radius)
   * representing the bounds of the rectangles for each vertex.
   * This field may be added to the tree using svtkTreeRingLayout.
   * This array must be set.
   */
  virtual void SetSectorsArrayName(const char* name)
  {
    this->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  }

  //@{
  /**
   * Define a shrink percentage for each of the sectors.
   */
  svtkSetMacro(ShrinkPercentage, double);
  svtkGetMacro(ShrinkPercentage, double);
  //@}

  int FillInputPortInformation(int port, svtkInformation* info) override;

protected:
  svtkTreeRingToPolyData();
  ~svtkTreeRingToPolyData() override;

  double ShrinkPercentage;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkTreeRingToPolyData(const svtkTreeRingToPolyData&) = delete;
  void operator=(const svtkTreeRingToPolyData&) = delete;
};

#endif
