/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkReebGraphVolumeSkeletonFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkReebGraphVolumeSkeletonFilter
 * @brief   compute a skeletal embedding of the
 * Reeb graph of a scalar field defined on a tetrahedral mesh
 * (svtkUnstructuredGrid).
 *
 * The filter takes a svtkUnstructuredGrid as an input (port 0), along with a
 * svtkReebGraph (port 1).
 * The filter samples each arc of the Reeb graph and embeds the samples on the
 * barycenter of the corresponding field contour.
 * The number of (evenly distributed) arc samples can be defined with
 * SetNumberOfSamples() (default value: 10).
 * The skeleton can be optionally smoothed with SetNumberOfSmoothingIterations()
 * (default value: 10).
 * The filter will first try to pull as a scalar field the svtkDataArray with Id
 * 'FieldId' of the svtkPolyData, see SetFieldId() (default: 0). The filter will
 * abort if this field does not exist.
 *
 * The filter outputs a svtkTable of points (double[3]). Each column contains the
 * samples (sorted by function value) of the corresponding arc. The first and
 * the last entry of the column corresponds to the critical nodes at the
 * extremity of the arc (each column has NumberOfSamples + 2 entries).
 *
 * The skeleton can be rendered by linking the samples with geometrical
 * primitives (for instance, spheres at critical nodes and cylinders between
 * intermediary samples, see Graphics/Testing/Cxx/TestReebGraph.cxx).
 *
 */

#ifndef svtkReebGraphVolumeSkeletonFilter_h
#define svtkReebGraphVolumeSkeletonFilter_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkFiltersReebGraphModule.h" // For export macro

class svtkReebGraph;
class svtkTable;

class SVTKFILTERSREEBGRAPH_EXPORT svtkReebGraphVolumeSkeletonFilter : public svtkDataObjectAlgorithm
{
public:
  static svtkReebGraphVolumeSkeletonFilter* New();
  svtkTypeMacro(svtkReebGraphVolumeSkeletonFilter, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the number of samples along each arc of the Reeb graph
   * Default value: 5
   */
  svtkSetMacro(NumberOfSamples, int);
  svtkGetMacro(NumberOfSamples, int);
  //@}

  //@{
  /**
   * Set the number of optional smoothing iterations
   * Default value: 30
   */
  svtkSetMacro(NumberOfSmoothingIterations, int);
  svtkGetMacro(NumberOfSmoothingIterations, int);
  //@}

  //@{
  /**
   * Set the scalar field Id
   * Default value: 0
   */
  svtkSetMacro(FieldId, svtkIdType);
  svtkGetMacro(FieldId, svtkIdType);
  //@}

  svtkTable* GetOutput();

protected:
  svtkReebGraphVolumeSkeletonFilter();
  ~svtkReebGraphVolumeSkeletonFilter() override;

  svtkIdType FieldId;
  int NumberOfSamples, NumberOfSmoothingIterations;

  int FillInputPortInformation(int portNumber, svtkInformation*) override;
  int FillOutputPortInformation(int portNumber, svtkInformation* info) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkReebGraphVolumeSkeletonFilter(const svtkReebGraphVolumeSkeletonFilter&) = delete;
  void operator=(const svtkReebGraphVolumeSkeletonFilter&) = delete;
};

#endif
