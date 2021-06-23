/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostBrandesCentrality.h

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
 * @class   svtkBoostBrandesCentrality
 * @brief   Compute Brandes betweenness centrality
 * on a svtkGraph
 *
 *
 *
 * This svtk class uses the Boost brandes_betweeness_centrality
 * generic algorithm to compute betweenness centrality on
 * the input graph (a svtkGraph).
 *
 * @sa
 * svtkGraph svtkBoostGraphAdapter
 */

#ifndef svtkBoostBrandesCentrality_h
#define svtkBoostBrandesCentrality_h

#include "svtkInfovisBoostGraphAlgorithmsModule.h" // For export macro
#include "svtkVariant.h"                           // For variant type

#include "svtkGraphAlgorithm.h"

class SVTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT svtkBoostBrandesCentrality : public svtkGraphAlgorithm
{
public:
  static svtkBoostBrandesCentrality* New();
  svtkTypeMacro(svtkBoostBrandesCentrality, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the flag that sets the rule whether or not to use the
   * edge weight array as set using \c SetEdgeWeightArrayName.
   */
  svtkSetMacro(UseEdgeWeightArray, bool);
  svtkBooleanMacro(UseEdgeWeightArray, bool);
  //@}

  svtkSetMacro(InvertEdgeWeightArray, bool);
  svtkBooleanMacro(InvertEdgeWeightArray, bool);

  //@{
  /**
   * Get/Set the name of the array that needs to be used as the edge weight.
   * The array should be a svtkDataArray.
   */
  svtkGetStringMacro(EdgeWeightArrayName);
  svtkSetStringMacro(EdgeWeightArrayName);
  //@}

protected:
  svtkBoostBrandesCentrality();
  ~svtkBoostBrandesCentrality() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  bool UseEdgeWeightArray;
  bool InvertEdgeWeightArray;
  char* EdgeWeightArrayName;

  svtkBoostBrandesCentrality(const svtkBoostBrandesCentrality&) = delete;
  void operator=(const svtkBoostBrandesCentrality&) = delete;
};

#endif
