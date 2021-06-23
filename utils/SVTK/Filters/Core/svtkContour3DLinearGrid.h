/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContour3DLinearGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkContour3DLinearGrid
 * @brief   fast generation of isosurface from 3D linear cells
 *
 * svtkContour3DLinearGrid is a specialized filter that generates isocontours
 * from an input svtkUnstructuredGrid consisting of 3D linear cells:
 * tetrahedra, hexahedra, voxels, pyramids, and/or wedges. (The cells are
 * linear in the sense that each cell edge is a straight line.) The filter is
 * designed for high-speed, specialized operation. All other cell types are
 * skipped and produce no output. (Note: the filter will also process
 * input svtkCompositeDataSets containing svtkUnstructuredGrids.)
 *
 * To use this filter you must specify an input unstructured grid or
 * svtkCompositeDataSet, and one or more contour values.  You can either use
 * the method SetValue() to specify each contour value, or use
 * GenerateValues() to generate a series of evenly spaced contours.
 *
 * The filter performance varies depending on optional output
 * information. Basically if point merging is required (when PointMerging,
 * InterpolateAttributes, and/or ComputeNormals is enabled), a sorting
 * process is required to eliminate duplicate output points in the
 * isosurface. Otherwise when point merging is not required, a fast path
 * process produces independent triangles representing the isosurface. In
 * many situations the results of the fast path are quite good and do not
 * require additional processing.
 *
 * Note that another performance option exists, using a svtkScalarTree, which
 * is an object that accelerates isosurface extraction, at the initial cost
 * of building the scalar tree. (This feature is useful for exploratory
 * isosurface extraction when the isovalue is frequently changed.) In some
 * cases this can improve performance, however this algorithm is so highly
 * tuned that random memory jumps (due to random access of cells provided by
 * the scalar tree) can actually negatively impact performance, especially if
 * the input dataset type consists of homogeneous cell types.
 *
 * @warning
 * When the input is of type svtkCompositeDataSet the filter will process the
 * unstructured grid(s) contained in the composite data set. As a result the
 * output of this filter is then a svtkMultiBlockDataSet containing multiple
 * svtkPolyData. When a svtkUnstructuredGrid is provided as input the
 * output is a single svtkPolyData.
 *
 * @warning
 * The fast path simply produces output points and triangles (the fast path
 * executes when MergePoints if off; InterpolateAttributes is off; and
 * ComputeNormals is off). Since the fast path does not merge points, it
 * produces many more output points, typically on the order of 5-6x more than
 * when MergePoints is enabled. Adding in the other options point merging,
 * field interpolation, and normal generation results in additional
 * performance impacts. By default the fast path is enabled.
 *
 * @warning
 * When a svtkCompositeDataSet is provided as input, and UseScalarTree is
 * enabled and a ScalarTree specified, then the specified scalar tree is
 * cloned to create new ones for each dataset in the composite
 * dataset. Otherwise (i.e., when svtkUnstructuredGrid input) the specified
 * scalar tree is directly used (no cloning required).
 *
 * @warning
 * Internal to this filter, a caching iterator is used to traverse the cells
 * that compose the svtkUnstructuredGrid. Maximum performance is obtained if
 * the cells are all of one type (i.e., input grid of homogeneous cell
 * types); repeated switching from different types may have detrimental
 * effects on performance.
 *
 * @warning
 * For unstructured data, gradients are not computed. Normals are computed if
 * requested; they are "pseudo-normals" in that the normals of output
 * triangles that use a common point are averaged at the point. Alternatively
 * use svtkPolyDataNormals to compute the surface normals.
 *
 * @warning
 * The output of this filter is subtly different than the more general filter
 * svtkContourGrid. svtkContourGrid eliminates small, degenerate triangles with
 * concident points which are consequently not sent to the output. In
 * practice this makes little impact on visual appearance but may have
 * repercussions if the output is used for modelling and/or analysis.
 *
 * @warning
 * Input cells that are not of 3D linear type (tetrahedron, hexahedron,
 * wedge, pyramid, and voxel) are simply skipped and not processed.
 *
 * @warning
 * The filter is templated on types of input and output points, and input
 * scalar type. To reduce object file bloat, only real points (float,double) are
 * processed, and a limited subset of scalar types.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkContourGrid svtkContourFilter svtkFlyingEdges3D svtkMarchingCubes
 * svtkPolyDataNormals svtkStaticEdgeLocatorTemplate.h svtkScalarTree
 * svtkSpanSpace
 */

#ifndef svtkContour3DLinearGrid_h
#define svtkContour3DLinearGrid_h

#include "svtkContourValues.h" // Needed for inline methods
#include "svtkDataObjectAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro

class svtkPolyData;
class svtkUnstructuredGrid;
class svtkScalarTree;
struct svtkScalarTreeMap;

class SVTKFILTERSCORE_EXPORT svtkContour3DLinearGrid : public svtkDataObjectAlgorithm
{
public:
  //@{
  /**
   * Standard methods for construction, type info, and printing.
   */
  static svtkContour3DLinearGrid* New();
  svtkTypeMacro(svtkContour3DLinearGrid, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Methods to set / get contour values.
   */
  void SetValue(int i, double value);
  double GetValue(int i);
  double* GetValues();
  void GetValues(double* contourValues);
  void SetNumberOfContours(int number);
  svtkIdType GetNumberOfContours();
  void GenerateValues(int numContours, double range[2]);
  void GenerateValues(int numContours, double rangeStart, double rangeEnd);
  //@}

  //@{
  /**
   * Indicate whether to merge coincident points. This takes extra time and
   * produces fewer output points, creating a "watertight" contour
   * surface. By default this is off.
   */
  svtkSetMacro(MergePoints, svtkTypeBool);
  svtkGetMacro(MergePoints, svtkTypeBool);
  svtkBooleanMacro(MergePoints, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate whether to interpolate input attributes onto the isosurface. By
   * default this option is off.
   */
  svtkSetMacro(InterpolateAttributes, svtkTypeBool);
  svtkGetMacro(InterpolateAttributes, svtkTypeBool);
  svtkBooleanMacro(InterpolateAttributes, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate whether to compute output point normals. An averaging method is
   * used to average shared triangle normals. By default this if off. This is
   * a relatively expensive option so use judiciously.
   */
  svtkSetMacro(ComputeNormals, svtkTypeBool);
  svtkGetMacro(ComputeNormals, svtkTypeBool);
  svtkBooleanMacro(ComputeNormals, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::Precision enum for an explanation of the available
   * precision settings.
   */
  void SetOutputPointsPrecision(int precision);
  int GetOutputPointsPrecision() const;
  //@}

  /**
   * Overloaded GetMTime() because of delegation to the internal
   * svtkContourValues class.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Enable the use of a scalar tree to accelerate contour extraction. By
   * default this is off. If enabled, and a scalar tree is not specified, then
   * a svtkSpanSpace instance will be constructed and used.
   */
  svtkSetMacro(UseScalarTree, svtkTypeBool);
  svtkGetMacro(UseScalarTree, svtkTypeBool);
  svtkBooleanMacro(UseScalarTree, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the scalar tree to use. By default a svtkSpanSpace scalar tree is
   * used.
   */
  virtual void SetScalarTree(svtkScalarTree*);
  svtkGetObjectMacro(ScalarTree, svtkScalarTree);
  //@}

  //@{
  /**
   * Force sequential processing (i.e. single thread) of the contouring
   * process. By default, sequential processing is off. Note this flag only
   * applies if the class has been compiled with SVTK_SMP_IMPLEMENTATION_TYPE
   * set to something other than Sequential. (If set to Sequential, then the
   * filter always runs in serial mode.) This flag is typically used for
   * benchmarking purposes.
   */
  svtkSetMacro(SequentialProcessing, svtkTypeBool);
  svtkGetMacro(SequentialProcessing, svtkTypeBool);
  svtkBooleanMacro(SequentialProcessing, svtkTypeBool);
  //@}

  /**
   *  Return the number of threads actually used during execution. This is
   *  valid only after algorithm execution.
   */
  int GetNumberOfThreadsUsed() { return this->NumberOfThreadsUsed; }

  /**
   * Inform the user as to whether large ids were used during filter
   * execution. This flag only has meaning after the filter has executed.
   * Large ids are used when the id of the larges cell or point is greater
   * than signed 32-bit precision. (Smaller ids reduce memory usage and speed
   * computation. Note that LargeIds are only available on 64-bit
   * architectures.)
   */
  bool GetLargeIds() { return this->LargeIds; }

  /**
   * Returns true if the data object passed in is fully supported by this
   * filter, i.e., all cell types are linear. For composite datasets, this
   * means all dataset leaves have only linear cell types that can be processed
   * by this filter. The second array is the name of the array to process.
   */
  static bool CanFullyProcessDataObject(svtkDataObject* object, const char* scalarArrayName);

protected:
  svtkContour3DLinearGrid();
  ~svtkContour3DLinearGrid() override;

  svtkContourValues* ContourValues;
  int OutputPointsPrecision;
  svtkTypeBool MergePoints;
  svtkTypeBool InterpolateAttributes;
  svtkTypeBool ComputeNormals;
  svtkTypeBool SequentialProcessing;
  int NumberOfThreadsUsed;
  bool LargeIds; // indicate whether integral ids are large(==true) or not

  // Manage scalar trees, including mapping scalar tree to input dataset
  svtkTypeBool UseScalarTree;
  svtkScalarTree* ScalarTree;
  struct svtkScalarTreeMap* ScalarTreeMap;

  // Process the data: input unstructured grid and output polydata
  void ProcessPiece(svtkUnstructuredGrid* input, svtkDataArray* inScalars, svtkPolyData* output);

  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkContour3DLinearGrid(const svtkContour3DLinearGrid&) = delete;
  void operator=(const svtkContour3DLinearGrid&) = delete;
};

/**
 * Set a particular contour value at contour number i. The index i ranges
 * between 0<=i<NumberOfContours.
 */
inline void svtkContour3DLinearGrid::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i, value);
}

/**
 * Get the ith contour value.
 */
inline double svtkContour3DLinearGrid::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

/**
 * Get a pointer to an array of contour values. There will be
 * GetNumberOfContours() values in the list.
 */
inline double* svtkContour3DLinearGrid::GetValues()
{
  return this->ContourValues->GetValues();
}

/**
 * Fill a supplied list with contour values. There will be
 * GetNumberOfContours() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void svtkContour3DLinearGrid::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

/**
 * Set the number of contours to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void svtkContour3DLinearGrid::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

/**
 * Get the number of contours in the list of contour values.
 */
inline svtkIdType svtkContour3DLinearGrid::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkContour3DLinearGrid::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkContour3DLinearGrid::GenerateValues(
  int numContours, double rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

#endif
