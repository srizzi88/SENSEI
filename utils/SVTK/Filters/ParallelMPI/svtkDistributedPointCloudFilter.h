/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDistributedPointCloudFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class svtkDistributedPointCloudFilter
 * @brief Distributes points among MPI processors.
 *
 * This filter distributes points among processors into spatially
 * contiguous point set, containing an equivalent number of points.
 * Algorithm: point set is recursively splitted in two, among MPI groups.
 * Note: input cells are ignored. Output is a svtkPolyData.
 *
 * @par Thanks:
 * This class has been written by Kitware SAS from an initial work made by
 * Aymeric Pelle from Universite de Technologie de Compiegne, France,
 * and Laurent Colombet and Thierry Carrard from Commissariat a l'Energie
 * Atomique (CEA/DIF).
 */

#ifndef svtkDistributedPointCloudFilter_h
#define svtkDistributedPointCloudFilter_h

#include "svtkFiltersParallelMPIModule.h" // For export macro
#include "svtkPointSetAlgorithm.h"

#include <vector> // for vector

class svtkMPIController;
class svtkMultiProcessController;

class SVTKFILTERSPARALLELMPI_EXPORT svtkDistributedPointCloudFilter : public svtkPointSetAlgorithm
{
public:
  static svtkDistributedPointCloudFilter* New();
  svtkTypeMacro(svtkDistributedPointCloudFilter, svtkPointSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the communicator object
   */
  void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  /**
   * Get the points that are inside innerBounds and put them in output DataSet.
   * Ask other MPI ranks for their corresponding points.
   */
  static void GetPointsInsideBounds(
    svtkMPIController*, svtkPointSet* input, svtkPointSet* output, const double innerBounds[6]);

protected:
  svtkDistributedPointCloudFilter();
  ~svtkDistributedPointCloudFilter() override;

  int FillOutputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Optimize bounding box following this rules:
   * - no intersection of bounding box of different MPI nodes
   * - same amount of point inside bounding box of each MPI nodes.
   *
   * Return false if input pointSet is nullptr or if no communicator was found.
   * Return true otherwise.
   */
  bool OptimizeBoundingBox(std::vector<svtkMPIController*>&, svtkPointSet*, double bounds[6]);

  /**
   * Initialize KdTreeRound: creates subControllers from Controller.
   * Delete old values if any.
   * Return false if KdTree cannot be initialized.
   */
  bool InitializeKdTree(std::vector<svtkMPIController*>&);

private:
  svtkDistributedPointCloudFilter(const svtkDistributedPointCloudFilter&) = delete;
  void operator=(const svtkDistributedPointCloudFilter&) = delete;

  svtkMultiProcessController* Controller;
};

#endif
