/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostGraphAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkBoostBetweennessClustering
 * @brief   Implements graph clustering based on
 * edge betweenness centrality.
 *
 *
 *
 * This svtk class uses the Boost centrality clustering
 * generic algorithm to compute edge betweenness centrality on
 * the input graph (a svtkGraph).
 *
 * @sa
 * svtkGraph svtkBoostGraphAdapter
 */

#ifndef svtkBoostBetweennessClustering_h
#define svtkBoostBetweennessClustering_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisBoostGraphAlgorithmsModule.h" // For export macro

class SVTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT svtkBoostBetweennessClustering : public svtkGraphAlgorithm
{
public:
  static svtkBoostBetweennessClustering* New();
  svtkTypeMacro(svtkBoostBetweennessClustering, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkBoostBetweennessClustering();
  ~svtkBoostBetweennessClustering() override;

  //@{
  /**
   * Get/Set the threshold value. Algorithm terminats when the maximum edge
   * centrality is below this threshold.
   */
  svtkSetMacro(Threshold, double);
  svtkGetMacro(Threshold, double);
  //@}

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

  //@{
  /**
   * Set the edge centrality array name. If no output array name is
   * set then the name "edge_centrality" is used.
   */
  svtkSetStringMacro(EdgeCentralityArrayName);
  //@}

protected:
  virtual int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  virtual int FillOutputPortInformation(int port, svtkInformation* info) override;

private:
  double Threshold;
  bool UseEdgeWeightArray;
  bool InvertEdgeWeightArray;
  char* EdgeWeightArrayName;
  char* EdgeCentralityArrayName;

  svtkBoostBetweennessClustering(const svtkBoostBetweennessClustering&) = delete;
  void operator=(const svtkBoostBetweennessClustering&) = delete;
};

#endif // svtkBoostBetweennessClustering_h
