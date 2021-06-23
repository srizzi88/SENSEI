/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPOutlineFilterInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPOutlineFilterInternals
 * @brief   create wireframe outline (or corners) for arbitrary data set
 *
 * svtkPOutlineFilterInternals has common code for svtkOutlineFilter and
 * svtkOutlineCornerFilter. It assumes the filter is operated in a data parallel
 * pipeline.
 */

#ifndef svtkPOutlineFilterInternals_h
#define svtkPOutlineFilterInternals_h

#include "svtkBoundingBox.h"           //  needed for svtkBoundingBox.
#include "svtkFiltersParallelModule.h" // For export macro
#include <vector>                     // needed for std::vector

class svtkBoundingBox;
class svtkDataObject;
class svtkDataObjectTree;
class svtkDataSet;
class svtkGraph;
class svtkInformation;
class svtkInformationVector;
class svtkMultiProcessController;
class svtkOverlappingAMR;
class svtkPolyData;
class svtkUniformGridAMR;

class SVTKFILTERSPARALLEL_EXPORT svtkPOutlineFilterInternals
{
public:
  svtkPOutlineFilterInternals();
  virtual ~svtkPOutlineFilterInternals();
  void SetController(svtkMultiProcessController*);
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);
  void SetCornerFactor(double cornerFactor);
  void SetIsCornerSource(bool value);

private:
  svtkPOutlineFilterInternals(const svtkPOutlineFilterInternals&) = delete;
  svtkPOutlineFilterInternals& operator=(const svtkPOutlineFilterInternals&) = delete;

  int RequestData(svtkOverlappingAMR* amr, svtkPolyData* output);
  int RequestData(svtkUniformGridAMR* amr, svtkPolyData* output);
  int RequestData(svtkDataObjectTree* cd, svtkPolyData* output);
  int RequestData(svtkDataSet* ds, svtkPolyData* output);
  int RequestData(svtkGraph* graph, svtkPolyData* output);

  void CollectCompositeBounds(svtkDataObject* input);

  std::vector<svtkBoundingBox> BoundsList;
  svtkMultiProcessController* Controller;

  bool IsCornerSource;
  double CornerFactor;
};

#endif
// SVTK-HeaderTest-Exclude: svtkPOutlineFilterInternals.h
