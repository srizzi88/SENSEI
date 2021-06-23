/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlaneCutter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPlaneCutter
 * @brief   cut any dataset with a plane and generate a
 * polygonal cut surface
 *
 * svtkPlaneCutter is a specialization of the svtkCutter algorithm to cut a
 * dataset grid with a single plane. It is designed for performance and an
 * exploratory, fast workflow. It produces output polygons that result from
 * cutting the icnput dataset with the specified plane.
 *
 * This algorithm is fast because it is threaded, and may build (in a
 * preprocessing step) a spatial search structure that accelerates the plane
 * cuts. The search structure, which is typically a sphere tree, is used to
 * quickly cull candidate cells.  (Note that certain types of input data are
 * delegated to other, internal classes; for example image data is delegated
 * to svtkFlyingEdgesPlaneCutter.)
 *
 * Because this filter may build an initial data structure during a
 * preprocessing step, the first execution of the filter may take longer than
 * subsequent operations. Typically the first execution is still faster than
 * svtkCutter (especially with threading enabled), but for certain types of
 * data this may not be true. However if you are using the filter to cut a
 * dataset multiple times (as in an exploratory or interactive workflow) this
 * filter typically works well.
 *
 * @warning
 * This filter outputs a svtkMultiBlockeDataSet. Each piece in the multiblock
 * output corresponds to the output from one thread.
 *
 * @warning
 * This filter produces non-merged, potentially coincident points for all
 * input dataset types except svtkImageData (which uses
 * svtkFlyingEdgesPlaneCutter under the hood - which does merge points).
 *
 * @warning
 * This filter delegates to svtkFlyingEdgesPlaneCutter to process image
 * data, but output and input have been standardized when possible.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkFlyingEdgesPlaneCutter svtk3DLinearGridPlaneCutter svtkCutter svtkPlane
 */

#ifndef svtkPlaneCutter_h
#define svtkPlaneCutter_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkSmartPointer.h"      // For SmartPointer
#include <vector>                 // For vector

class svtkCellArray;
class svtkCellData;
class svtkImageData;
class svtkMultiPieceDataSet;
class svtkPlane;
class svtkPointData;
class svtkPoints;
class svtkSphereTree;
class svtkStructuredGrid;
class svtkUnstructuredGrid;

class SVTKFILTERSCORE_EXPORT svtkPlaneCutter : public svtkDataSetAlgorithm
{
public:
  //@{
  /**
   * Standard construction and print methods.
   */
  static svtkPlaneCutter* New();
  svtkTypeMacro(svtkPlaneCutter, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * The modified time depends on the delegated cut plane.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Specify the plane (an implicit function) to perform the cutting. The
   * definition of the plane (its origin and normal) is controlled via this
   * instance of svtkPlane.
   */
  virtual void SetPlane(svtkPlane*);
  svtkGetObjectMacro(Plane, svtkPlane);
  //@}

  //@{
  /**
   * Set/Get the computation of normals. The normal generated is simply the
   * cut plane normal. The normal, if generated, is defined by cell data
   * associated with the output polygons. By default computing of normals is
   * disabled.
   */
  svtkSetMacro(ComputeNormals, bool);
  svtkGetMacro(ComputeNormals, bool);
  svtkBooleanMacro(ComputeNormals, bool);
  //@}

  //@{
  /**
   * Indicate whether to interpolate attribute data. By default this is
   * enabled. Note that both cell data and point data is interpolated and
   * outputted, except for image data input where only point data are outputted.
   */
  svtkSetMacro(InterpolateAttributes, bool);
  svtkGetMacro(InterpolateAttributes, bool);
  svtkBooleanMacro(InterpolateAttributes, bool);
  //@}

  //@{
  /**
   * Indicate whether to generate polygons instead of triangles when cutting
   * structured and rectilinear grid.
   * No effect with other kinds of inputs, enabled by default.
   */
  svtkSetMacro(GeneratePolygons, bool);
  svtkGetMacro(GeneratePolygons, bool);
  svtkBooleanMacro(GeneratePolygons, bool);
  //@}

  //@{
  /**
   * Indicate whether to build the sphere tree. Computing the sphere
   * will take some time on the first computation
   * but if the input does not change, the computation of all further
   * slice will be much faster. Default is on.
   */
  svtkSetMacro(BuildTree, bool);
  svtkGetMacro(BuildTree, bool);
  svtkBooleanMacro(BuildTree, bool);
  //@}

  //@{
  /**
   * Indicate whether to build tree hierarchy. Computing the tree
   * hierarchy can take some time on the first computation but if
   * the input does not change, the computation of all further
   * slice will be faster. Default is on.
   */
  svtkSetMacro(BuildHierarchy, bool);
  svtkGetMacro(BuildHierarchy, bool);
  svtkBooleanMacro(BuildHierarchy, bool);
  //@}

  /**
   * See svtkAlgorithm for details.
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkPlaneCutter();
  ~svtkPlaneCutter() override;

  svtkPlane* Plane;
  bool ComputeNormals;
  bool InterpolateAttributes;
  bool GeneratePolygons;
  bool BuildTree;
  bool BuildHierarchy;

  // Helpers
  std::vector<svtkSmartPointer<svtkSphereTree> > SphereTrees;

  // Pipeline-related methods
  int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  virtual int ExecuteDataSet(svtkDataSet* input, svtkSphereTree* tree, svtkMultiPieceDataSet* output);

  static void AddNormalArray(double* planeNormal, svtkDataSet* ds);
  static void InitializeOutput(svtkMultiPieceDataSet* output);

private:
  svtkPlaneCutter(const svtkPlaneCutter&) = delete;
  void operator=(const svtkPlaneCutter&) = delete;
};

#endif
