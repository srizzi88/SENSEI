/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSphereTreeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkSphereTreeFilter
 * @brief represent a sphere tree as svtkPolyData
 *
 * svtkSphereTreeFilter is a filter that produces a svtkPolyData representation
 * of a sphere tree (svtkSphereTree). Basically it generates a point, a scalar
 * radius, and tree level number for the cell spheres and/or the different levels
 * in the tree hierarchy (assuming that the hierarchy is built). The output
 * can be glyphed using a filter like svtkGlyph3D to actually visualize the
 * sphere tree. The primary use of this class is for visualization of sphere
 * trees, and debugging the construction and use of sphere trees.
 *
 * Additional capabilities include production of candidate spheres based on
 * geometric queries. For example, queries based on a point, infinite line,
 * and infinite plane are possible.
 *
 * Note that this class may create a sphere tree, and then build it, for the
 * input dataset to this filter (if no sphere tree is provided). If the user
 * specifies a sphere tree, then the specified sphere tree is used. Thus the
 * input to the filter is optional. Consequently this filter can act like a source,
 * or as a filter in a pipeline.
 *
 * @sa
 * svtkSphereTree svtkPlaneCutter
 */

#ifndef svtkSphereTreeFilter_h
#define svtkSphereTreeFilter_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_SPHERE_TREE_LEVELS 0
#define SVTK_SPHERE_TREE_POINT 1
#define SVTK_SPHERE_TREE_LINE 2
#define SVTK_SPHERE_TREE_PLANE 3

class svtkSphereTree;

class SVTKFILTERSCORE_EXPORT svtkSphereTreeFilter : public svtkPolyDataAlgorithm
{
public:
  /**
   * Instantiate the sphere tree filter.
   */
  static svtkSphereTreeFilter* New();

  //@{
  /**
   * Standard type related macros and PrintSelf() method.
   */
  svtkTypeMacro(svtkSphereTreeFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify and retrieve the sphere tree.
   */
  virtual void SetSphereTree(svtkSphereTree*);
  svtkGetObjectMacro(SphereTree, svtkSphereTree);
  //@}

  //@{
  /**
   * Specify what information this filter is to extract from the sphere
   * tree. Options include: spheres that make up one or more levels; spheres
   * that intersect a specified plane; spheres that intersect a specified line;
   * and spheres that intersect a specified point. What is extracted are sphere
   * centers, a radius, and an optional level. By default the specified levels
   * are extracted.
   */
  svtkSetMacro(ExtractionMode, int);
  svtkGetMacro(ExtractionMode, int);
  void SetExtractionModeToLevels() { this->SetExtractionMode(SVTK_SPHERE_TREE_LEVELS); }
  void SetExtractionModeToPoint() { this->SetExtractionMode(SVTK_SPHERE_TREE_POINT); }
  void SetExtractionModeToLine() { this->SetExtractionMode(SVTK_SPHERE_TREE_LINE); }
  void SetExtractionModeToPlane() { this->SetExtractionMode(SVTK_SPHERE_TREE_PLANE); }
  const char* GetExtractionModeAsString();
  //@}

  //@{
  /**
   * Enable or disable the building and generation of the sphere tree
   * hierarchy. The hierarchy represents different levels in the tree
   * and enables rapid traversal of the tree.
   */
  svtkSetMacro(TreeHierarchy, bool);
  svtkGetMacro(TreeHierarchy, bool);
  svtkBooleanMacro(TreeHierarchy, bool);
  //@}

  //@{
  /**
   * Specify the level of the tree to extract (used when ExtractionMode is
   * set to Levels). A value of (-1) means all levels. Note that level 0 is
   * the root of the sphere tree. By default all levels are extracted. Note
   * that if TreeHierarchy is off, then it is only possible to extract leaf
   * spheres (i.e., spheres for each cell of the associated dataset).
   */
  svtkSetClampMacro(Level, int, -1, SVTK_SHORT_MAX);
  svtkGetMacro(Level, int);
  //@}

  //@{
  /**
   * Specify a point used to extract one or more leaf spheres. This method is
   * used when extracting spheres using a point, line, or plane.
   */
  svtkSetVector3Macro(Point, double);
  svtkGetVectorMacro(Point, double, 3);
  //@}

  //@{
  /**
   * Specify a line used to extract spheres (used when ExtractionMode is set
   * to Line). The Ray plus Point define an infinite line. The ray is a
   * vector defining the direction of the line.
   */
  svtkSetVector3Macro(Ray, double);
  svtkGetVectorMacro(Ray, double, 3);
  //@}

  //@{
  /**
   * Specify a plane used to extract spheres (used when ExtractionMode is set
   * to Plane). The plane Normal plus Point define an infinite plane.
   */
  svtkSetVector3Macro(Normal, double);
  svtkGetVectorMacro(Normal, double, 3);
  //@}

  /**
   * Modified GetMTime because the sphere tree may have changed.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkSphereTreeFilter();
  ~svtkSphereTreeFilter() override;

  svtkSphereTree* SphereTree;
  bool TreeHierarchy;
  int ExtractionMode;
  int Level;
  double Point[3];
  double Ray[3];
  double Normal[3];

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkSphereTreeFilter(const svtkSphereTreeFilter&) = delete;
  void operator=(const svtkSphereTreeFilter&) = delete;
};

#endif
