/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHierarchicalBinningFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHierarchicalBinningFilter
 * @brief   uniform binning of points into
 * a hierarchical structure
 *
 *
 * svtkHierarchicalBinningFilter creates a spatial, hierarchical ordering of
 * input points. This hierarchy is suitable for level-of-detail rendering, or
 * multiresolution processing. Each level of the hierarchy is based on
 * uniform binning of space, where deeper levels (and its bins) are
 * repeatedly subdivided by a given branching factor. Points are associated
 * with bins at different levels, with the number of points in each level
 * proportional to the number of bins in that level. The output points are
 * sorted according to a bin number, where the bin number is unique,
 * monotonically increasing number representing the breadth first ordering of
 * all of the levels and their bins. Thus all points in a bin (or even a level)
 * are segmented into contiguous runs.
 *
 * Note that points are associated with different bins using a pseudo random
 * process. No points are repeated, and no new points are created, thus the
 * effect of executing this filter is simply to reorder the input points.
 *
 * The algorithm proceeds as follows: Given an initial bounding box, the
 * space is uniformally subdivided into bins of (M x N x O) dimensions; in
 * turn each subsequent level in the tree is further divided into (M x N x O)
 * bins (note that level 0 is a single, root bin). Thus the number of bins at
 * level L of the hierarchical tree is: Nbins=(M^L x N^L x O^L). Once the
 * binning is created to a specified depth, then points are placed in the
 * bins using a pseudo-random sampling proportional to the number of bins in each
 * level. All input points are sorted in the order described above, with no
 * points repeated.
 *
 * The output of this filter are sorted points and associated point
 * attributes represented by a svtkPolyData. In addition, an offset integral
 * array is associated with the field data of the output, providing offsets
 * into the points list via a breadth-first traversal order. Metadata
 * describing the output is provided in the field data. Convenience functions
 * are also provided here to access the data in a particular bin or across a
 * level. (Using the offset array directly may result in higher performance.)
 *
 * While any svtkPointSet type can be provided as input, the output is
 * represented by an explicit representation of points via a
 * svtkPolyData. This output polydata will populate its instance of svtkPoints,
 * but no cells will be defined (i.e., no svtkVertex or svtkPolyVertex are
 * contained in the output).
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkPointCloudFilter svtkQuadricClustering svtkStaticPointLocator
 */

#ifndef svtkHierarchicalBinningFilter_h
#define svtkHierarchicalBinningFilter_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_MAX_LEVEL 12

struct svtkBinTree;

class SVTKFILTERSPOINTS_EXPORT svtkHierarchicalBinningFilter : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static svtkHierarchicalBinningFilter* New();
  svtkTypeMacro(svtkHierarchicalBinningFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the number of levels in the spatial hierarchy. By default, the
   * number of levels is three.
   */
  svtkSetClampMacro(NumberOfLevels, int, 1, SVTK_MAX_LEVEL);
  svtkGetMacro(NumberOfLevels, int);
  //@}

  //@{
  /**
   * Specify whether to determine the determine the level divisions, and the bounding
   * box automatically (by default this is on). If off, then the user must specify both
   * the bounding box and bin divisions. (Computing the bounding box can be slow for
   * large point clouds, manual specification can save time.)
   */
  svtkSetMacro(Automatic, bool);
  svtkGetMacro(Automatic, bool);
  svtkBooleanMacro(Automatic, bool);
  //@}

  //@{
  /**
   * Set the number of branching divisions in each binning direction. Each
   * level of the tree is subdivided by this factor. The Divisions[i] must be
   * >= 1. Note: if Automatic subdivision is specified, the Divisions are
   * set by the filter.
   */
  svtkSetVector3Macro(Divisions, int);
  svtkGetVectorMacro(Divisions, int, 3);
  //@}

  //@{
  /**
   * Set the bounding box of the point cloud. If Automatic is enabled, then
   * this is computed during filter execution. If manually specified
   * (Automatic is off) then make sure the bounds is represented as
   * (xmin,xmax, ymin,ymax, zmin,zmax). If the bounds specified is does not
   * enclose the points, then points are clamped to lie in the bounding box.
   */
  svtkSetVector6Macro(Bounds, double);
  svtkGetVectorMacro(Bounds, double, 6);
  //@}

  /**
   * Convenience methods for extracting useful information about this bin
   * tree.  Return the number of total bins across all levels (i.e., the
   * total global bins). Invoke this method after the bin tree has been
   * built.
   */
  int GetNumberOfGlobalBins();

  /**
   * Convenience methods for extracting useful information about this bin
   * tree.  Return the number of bins in a particular level of the
   * tree. Invoke this method after the bin tree has been built.
   */
  int GetNumberOfBins(int level);

  /**
   * Convenience methods for extracting useful information about this bin
   * tree.  Given a level, return the beginning point id and number of points
   * that level. Invoke this method after the bin tree has been built.
   */
  svtkIdType GetLevelOffset(int level, svtkIdType& npts);

  /**
   * Convenience methods for extracting useful information about this bin
   * tree.  Given a global bin number, return the point id and number of
   * points for that bin. Invoke this method after the bin tree has been
   * built.
   */
  svtkIdType GetBinOffset(int globalBin, svtkIdType& npts);

  /**
   * Convenience methods for extracting useful information about this bin
   * tree.  Given a level, and the bin number in that level, return the
   * offset point id and number of points for that bin. Invoke this method
   * after the bin tree has been built.
   */
  svtkIdType GetLocalBinOffset(int level, int localBin, svtkIdType& npts);

  /**
   * Convenience methods for extracting useful information about a bin tree.
   * Given a global bin number, return the bounds (xmin,xmax,ymin,ymax,zmin,zmax)
   * for that bin. Invoke this method after the bin tree has been built.
   */
  void GetBinBounds(int globalBin, double bounds[6]);

  /**
   * Convenience methods for extracting useful information about a bin tree.
   * Given a level, and a local bin number, return the bounds
   * (xmin,xmax,ymin,ymax,zmin,zmax) for that bin. Invoke this method after
   * the bin tree has been built.
   */
  void GetLocalBinBounds(int level, int localBin, double bounds[6]);

protected:
  svtkHierarchicalBinningFilter();
  ~svtkHierarchicalBinningFilter() override;

  // IVars
  int NumberOfLevels;
  bool Automatic;
  int Divisions[3];
  double Bounds[6];

  // Handle to the underlying implementation. The representation is maintained so
  // that the convenience functions can be invoked on the bin tree.
  svtkBinTree* Tree;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkHierarchicalBinningFilter(const svtkHierarchicalBinningFilter&) = delete;
  void operator=(const svtkHierarchicalBinningFilter&) = delete;
};

#endif
