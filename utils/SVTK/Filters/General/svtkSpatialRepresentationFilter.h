/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSpatialRepresentationFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSpatialRepresentationFilter
 * @brief   generate polygonal model of spatial search object (i.e., a svtkLocator)
 *
 * svtkSpatialRepresentationFilter generates an polygonal representation of a
 * spatial search (svtkLocator) object. The representation varies depending
 * upon the nature of the spatial search object. For example, the
 * representation for svtkOBBTree is a collection of oriented bounding
 * boxes. This input to this filter is a dataset of any type, and the output
 * is polygonal data. You must also specify the spatial search object to
 * use.
 *
 * Generally spatial search objects are used for collision detection and
 * other geometric operations, but in this filter one or more levels of
 * spatial searchers can be generated to form a geometric approximation to
 * the input data. This is a form of data simplification, generally used to
 * accelerate the rendering process. Or, this filter can be used as a
 * debugging/ visualization aid for spatial search objects.
 *
 * This filter can generate one or more  svtkPolyData blocks corresponding to
 * different levels in the spatial search tree. The block ids range from 0
 * (root level) to MaximumLevel. Note that the block for level "id" is not computed
 * unless a AddLevel(id) method is issued. Thus, if you desire three levels of output
 * (say 2,4,7), you would have to invoke AddLevel(2), AddLevel(4), and
 * AddLevel(7). If GenerateLeaves is set to true (off by default), all leaf nodes
 * of the locator (which may be at different levels) are computed and stored in
 * block with id MaximumLevel + 1.
 *
 * @sa
 * svtkLocator svtkPointLocator svtkCellLocator svtkOBBTree
 */

#ifndef svtkSpatialRepresentationFilter_h
#define svtkSpatialRepresentationFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class svtkLocator;
class svtkDataSet;
class svtkSpatialRepresentationFilterInternal;

class SVTKFILTERSGENERAL_EXPORT svtkSpatialRepresentationFilter : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkSpatialRepresentationFilter* New();
  svtkTypeMacro(svtkSpatialRepresentationFilter, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the locator that will be used to generate the representation.
   */
  virtual void SetSpatialRepresentation(svtkLocator*);
  svtkGetObjectMacro(SpatialRepresentation, svtkLocator);
  //@}

  //@{
  /**
   * Get the maximum level that is available. Populated during
   * RequestData().
   */
  svtkGetMacro(MaximumLevel, int);
  //@}

  /**
   * Add a level to be computed.
   */
  void AddLevel(int level);

  /**
   * Remove all levels.
   */
  void ResetLevels();

  //@{
  /**
   * Turn on/off the generation of leaf nodes. Off by default.
   */
  svtkSetMacro(GenerateLeaves, bool);
  svtkGetMacro(GenerateLeaves, bool);
  svtkBooleanMacro(GenerateLeaves, bool);
  //@}

protected:
  svtkSpatialRepresentationFilter();
  ~svtkSpatialRepresentationFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int MaximumLevel;
  bool GenerateLeaves;

  svtkLocator* SpatialRepresentation;

  void ReportReferences(svtkGarbageCollector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkSpatialRepresentationFilter(const svtkSpatialRepresentationFilter&) = delete;
  void operator=(const svtkSpatialRepresentationFilter&) = delete;

  svtkSpatialRepresentationFilterInternal* Internal;
};

#endif
