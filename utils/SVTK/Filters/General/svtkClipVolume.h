/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClipVolume.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkClipVolume
 * @brief   clip volume data with user-specified implicit function or input scalar data
 *
 * svtkClipVolume is a filter that clips volume data (i.e., svtkImageData)
 * using either: any subclass of svtkImplicitFunction or the input scalar
 * data. The clipping operation cuts through the cells of the
 * dataset--converting 3D image data into a 3D unstructured grid--returning
 * everything inside of the specified implicit function (or greater than the
 * scalar value). During the clipping the filter will produce pieces of a
 * cell. (Compare this with svtkExtractGeometry or svtkGeometryFilter, which
 * produces entire, uncut cells.) The output of this filter is a 3D
 * unstructured grid (e.g., tetrahedra or other 3D cell types).
 *
 * To use this filter, you must decide if you will be clipping with an
 * implicit function, or whether you will be using the input scalar data.  If
 * you want to clip with an implicit function, you must first define and then
 * set the implicit function with the SetClipFunction() method. Otherwise,
 * you must make sure input scalar data is available. You can also specify a
 * scalar value, which is used to decide what is inside and outside of the
 * implicit function. You can also reverse the sense of what inside/outside
 * is by setting the InsideOut instance variable. (The cutting algorithm
 * proceeds by computing an implicit function value or using the input scalar
 * data for each point in the dataset. This is compared to the scalar value
 * to determine inside/outside.)
 *
 * This filter can be configured to compute a second output. The
 * second output is the portion of the volume that is clipped away. Set the
 * GenerateClippedData boolean on if you wish to access this output data.
 *
 * The filter will produce an unstructured grid of entirely tetrahedra or a
 * mixed grid of tetrahedra and other 3D cell types (e.g., wedges). Control
 * this behavior by setting the Mixed3DCellGeneration. By default the
 * Mixed3DCellGeneration is on and a combination of cell types will be
 * produced. Note that producing mixed cell types is a faster than producing
 * only tetrahedra.
 *
 * @warning
 * This filter is designed to function with 3D structured points. Clipping
 * 2D images should be done by converting the image to polygonal data
 * and using svtkClipPolyData,
 *
 * @sa
 * svtkImplicitFunction svtkClipPolyData svtkGeometryFilter svtkExtractGeometry
 */

#ifndef svtkClipVolume_h
#define svtkClipVolume_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkCellData;
class svtkDataArray;
class svtkIdList;
class svtkImplicitFunction;
class svtkMergePoints;
class svtkOrderedTriangulator;
class svtkPointData;
class svtkIncrementalPointLocator;
class svtkPoints;
class svtkUnstructuredGrid;
class svtkCell;
class svtkTetra;
class svtkCellArray;
class svtkIdTypeArray;
class svtkUnsignedCharArray;

class SVTKFILTERSGENERAL_EXPORT svtkClipVolume : public svtkUnstructuredGridAlgorithm
{
public:
  svtkTypeMacro(svtkClipVolume, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with user-specified implicit function; InsideOut turned off;
   * value set to 0.0; and generate clip scalars turned off.
   */
  static svtkClipVolume* New();

  //@{
  /**
   * Set the clipping value of the implicit function (if clipping with
   * implicit function) or scalar value (if clipping with scalars). The
   * default value is 0.0.
   */
  svtkSetMacro(Value, double);
  svtkGetMacro(Value, double);
  //@}

  //@{
  /**
   * Set/Get the InsideOut flag. When off, a vertex is considered inside the
   * implicit function if its value is greater than the Value ivar. When
   * InsideOutside is turned on, a vertex is considered inside the implicit
   * function if its implicit function value is less than or equal to the
   * Value ivar.  InsideOut is off by default.
   */
  svtkSetMacro(InsideOut, svtkTypeBool);
  svtkGetMacro(InsideOut, svtkTypeBool);
  svtkBooleanMacro(InsideOut, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the implicit function with which to perform the clipping. If you
   * do not define an implicit function, then the input scalar data will be
   * used for clipping.
   */
  virtual void SetClipFunction(svtkImplicitFunction*);
  svtkGetObjectMacro(ClipFunction, svtkImplicitFunction);
  //@}

  //@{
  /**
   * If this flag is enabled, then the output scalar values will be
   * interpolated from the implicit function values, and not the
   * input scalar data. If you enable this flag but do not provide an
   * implicit function an error will be reported.
   */
  svtkSetMacro(GenerateClipScalars, svtkTypeBool);
  svtkGetMacro(GenerateClipScalars, svtkTypeBool);
  svtkBooleanMacro(GenerateClipScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * Control whether a second output is generated. The second output
   * contains the unstructured grid that's been clipped away.
   */
  svtkSetMacro(GenerateClippedOutput, svtkTypeBool);
  svtkGetMacro(GenerateClippedOutput, svtkTypeBool);
  svtkBooleanMacro(GenerateClippedOutput, svtkTypeBool);
  //@}

  /**
   * Return the clipped output.
   */
  svtkUnstructuredGrid* GetClippedOutput();

  //@{
  /**
   * Control whether the filter produces a mix of 3D cell types on output, or
   * whether the output cells are all tetrahedra. By default, a mixed set of
   * cells (e.g., tetrahedra and wedges) is produced. (Note: mixed type
   * generation is faster and less overall data is generated.)
   */
  svtkSetMacro(Mixed3DCellGeneration, svtkTypeBool);
  svtkGetMacro(Mixed3DCellGeneration, svtkTypeBool);
  svtkBooleanMacro(Mixed3DCellGeneration, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the tolerance for merging clip intersection points that are near
   * the corners of voxels. This tolerance is used to prevent the generation
   * of degenerate tetrahedra.
   */
  svtkSetClampMacro(MergeTolerance, double, 0.0001, 0.25);
  svtkGetMacro(MergeTolerance, double);
  //@}

  //@{
  /**
   * Set / Get a spatial locator for merging points. By default,
   * an instance of svtkMergePoints is used.
   */
  void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified. The
   * locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

  /**
   * Return the mtime also considering the locator and clip function.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkClipVolume(svtkImplicitFunction* cf = nullptr);
  ~svtkClipVolume() override;

  void ReportReferences(svtkGarbageCollector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  void ClipTets(double value, svtkTetra* clipTetra, svtkDataArray* clipScalars,
    svtkDataArray* cellScalars, svtkIdList* tetraIds, svtkPoints* tetraPts, svtkPointData* inPD,
    svtkPointData* outPD, svtkCellData* inCD, svtkIdType cellId, svtkCellData* outCD,
    svtkCellData* clippedCD, int insideOut);
  void ClipVoxel(double value, svtkDataArray* cellScalars, int flip, double origin[3],
    double spacing[3], svtkIdList* cellIds, svtkPoints* cellPts, svtkPointData* inPD,
    svtkPointData* outPD, svtkCellData* inCD, svtkIdType cellId, svtkCellData* outCD,
    svtkCellData* clippedCD);

  svtkImplicitFunction* ClipFunction;
  svtkIncrementalPointLocator* Locator;
  svtkTypeBool InsideOut;
  double Value;
  svtkTypeBool GenerateClipScalars;
  double MergeTolerance;
  svtkTypeBool Mixed3DCellGeneration;
  svtkTypeBool GenerateClippedOutput;
  svtkUnstructuredGrid* ClippedOutput;

private:
  svtkOrderedTriangulator* Triangulator;

  // Used temporarily to pass data around
  svtkIdType NumberOfCells;
  svtkCellArray* Connectivity;
  svtkUnsignedCharArray* Types;
  svtkIdType NumberOfClippedCells;
  svtkCellArray* ClippedConnectivity;
  svtkUnsignedCharArray* ClippedTypes;

private:
  svtkClipVolume(const svtkClipVolume&) = delete;
  void operator=(const svtkClipVolume&) = delete;
};

#endif
