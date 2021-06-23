/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStrahlerMetric.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//-------------------------------------------------------------------------
// Copyright 2008 Sandia Corporation.
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//-------------------------------------------------------------------------
//
/**
 * @class   svtkStrahlerMetric
 * @brief   compute Strahler metric for a tree
 *
 * The Strahler metric is a value assigned to each vertex of a
 * tree that characterizes the structural complexity of the
 * sub-tree rooted at that node.  The metric originated in the
 * study of river systems, but has been applied to other tree-
 * structured systes,  Details of the metric and the rationale
 * for using it in infovis can be found in:
 *
 * Tree Visualization and Navigation Clues for Information
 * Visualization, I. Herman, M. Delest, and G. Melancon,
 * Computer Graphics Forum, Vol 17(2), Blackwell, 1998.
 *
 * The input tree is copied to the output, but with a new array
 * added to the output vertex data.
 *
 * @par Thanks:
 * Thanks to David Duke from the University of Leeds for providing this
 * implementation.
 */

#ifndef svtkStrahlerMetric_h
#define svtkStrahlerMetric_h

#include "svtkFiltersStatisticsModule.h" // For export macro
#include "svtkTreeAlgorithm.h"

class svtkFloatArray;

class SVTKFILTERSSTATISTICS_EXPORT svtkStrahlerMetric : public svtkTreeAlgorithm
{
public:
  static svtkStrahlerMetric* New();
  svtkTypeMacro(svtkStrahlerMetric, svtkTreeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the name of the array in which the Strahler values will
   * be stored within the output vertex data.
   * Default is "Strahler"
   */
  svtkSetStringMacro(MetricArrayName);
  //@}

  //@{
  /**
   * Set/get setting of normalize flag.  If this is set, the
   * Strahler values are scaled into the range [0..1].
   * Default is for normalization to be OFF.
   */
  svtkSetMacro(Normalize, svtkTypeBool);
  svtkGetMacro(Normalize, svtkTypeBool);
  svtkBooleanMacro(Normalize, svtkTypeBool);
  //@}

  //@{
  /**
   * Get the maximum strahler value for the tree.
   */
  svtkGetMacro(MaxStrahler, float);
  //@}

protected:
  svtkStrahlerMetric();
  ~svtkStrahlerMetric() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkTypeBool Normalize;
  float MaxStrahler;
  char* MetricArrayName;

  float CalculateStrahler(svtkIdType root, svtkFloatArray* metric, svtkTree* graph);

private:
  svtkStrahlerMetric(const svtkStrahlerMetric&) = delete;
  void operator=(const svtkStrahlerMetric&) = delete;
};

#endif
