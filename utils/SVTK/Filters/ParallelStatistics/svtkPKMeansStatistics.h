/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPKMeansStatistics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
/**
 * @class   svtkPKMeansStatisitcs
 * @brief   A class for parallel k means clustering
 *
 * svtkPKMeansStatistics is svtkKMeansStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * @par Thanks:
 * Thanks to Janine Bennett, Philippe Pebay and David Thompson from Sandia National Laboratories for
 * implementing this class.
 */

#ifndef svtkPKMeansStatistics_h
#define svtkPKMeansStatistics_h

#include "svtkFiltersParallelStatisticsModule.h" // For export macro
#include "svtkKMeansStatistics.h"

class svtkMultiProcessController;
class svtkCommunicator;

class SVTKFILTERSPARALLELSTATISTICS_EXPORT svtkPKMeansStatistics : public svtkKMeansStatistics
{
public:
  static svtkPKMeansStatistics* New();
  svtkTypeMacro(svtkPKMeansStatistics, svtkKMeansStatistics);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  /**
   * Subroutine to update new cluster centers from the old centers.
   */
  void UpdateClusterCenters(svtkTable* newClusterElements, svtkTable* curClusterElements,
    svtkIdTypeArray* numMembershipChanges, svtkIdTypeArray* numElementsInCluster,
    svtkDoubleArray* error, svtkIdTypeArray* startRunID, svtkIdTypeArray* endRunID,
    svtkIntArray* computeRun) override;

  /**
   * Subroutine to get the total number of data objects.
   */
  svtkIdType GetTotalNumberOfObservations(svtkIdType numObservations) override;

  /**
   * Subroutine to initialize cluster centerss if not provided by the user.
   */
  void CreateInitialClusterCenters(svtkIdType numToAllocate, svtkIdTypeArray* numberOfClusters,
    svtkTable* inData, svtkTable* curClusterElements, svtkTable* newClusterElements) override;

protected:
  svtkPKMeansStatistics();
  ~svtkPKMeansStatistics() override;

  svtkMultiProcessController* Controller;

private:
  svtkPKMeansStatistics(const svtkPKMeansStatistics&) = delete;
  void operator=(const svtkPKMeansStatistics&) = delete;
};

#endif
