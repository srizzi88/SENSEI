/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOBBDicer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOBBDicer
 * @brief   divide dataset into spatially aggregated pieces
 *
 * svtkOBBDicer separates the cells of a dataset into spatially
 * aggregated pieces using a Oriented Bounding Box (OBB). These pieces
 * can then be operated on by other filters (e.g., svtkThreshold). One
 * application is to break very large polygonal models into pieces and
 * performing viewing and occlusion culling on the pieces.
 *
 * Refer to the superclass documentation (svtkDicer) for more information.
 *
 * @sa
 * svtkDicer svtkConnectedDicer
 */

#ifndef svtkOBBDicer_h
#define svtkOBBDicer_h

#include "svtkDicer.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class svtkOBBNode;
class svtkShortArray;
class svtkIdList;
class svtkPoints;

class SVTKFILTERSGENERAL_EXPORT svtkOBBDicer : public svtkDicer
{
public:
  svtkTypeMacro(svtkOBBDicer, svtkDicer);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate an object.
   */
  static svtkOBBDicer* New();

protected:
  svtkOBBDicer() {}
  ~svtkOBBDicer() override {}

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // implementation ivars and methods
  void BuildTree(svtkIdList* ptIds, svtkOBBNode* OBBptr, svtkDataSet* input);
  void MarkPoints(svtkOBBNode* OBBptr, svtkShortArray* groupIds);
  void DeleteTree(svtkOBBNode* OBBptr);
  svtkPoints* PointsList;

private:
  svtkOBBDicer(const svtkOBBDicer&) = delete;
  void operator=(const svtkOBBDicer&) = delete;
};

#endif
