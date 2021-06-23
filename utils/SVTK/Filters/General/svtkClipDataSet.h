/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClipDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkClipDataSet
 * @brief   clip any dataset with user-specified implicit function or input scalar data
 *
 * svtkClipDataSet is a filter that clips any type of dataset using either
 * any subclass of svtkImplicitFunction, or the input scalar
 * data. Clipping means that it actually "cuts" through the cells of
 * the dataset, returning everything inside of the specified implicit
 * function (or greater than the scalar value) including "pieces" of
 * a cell. (Compare this with svtkExtractGeometry, which pulls out
 * entire, uncut cells.) The output of this filter is an unstructured
 * grid.
 *
 * To use this filter, you must decide if you will be clipping with an
 * implicit function, or whether you will be using the input scalar
 * data.  If you want to clip with an implicit function, you must:
 * 1) define an implicit function
 * 2) set it with the SetClipFunction method
 * 3) apply the GenerateClipScalarsOn method
 * If a ClipFunction is not specified, or GenerateClipScalars is off
 * (the default), then the input's scalar data will be used to clip
 * the polydata.
 *
 * You can also specify a scalar value, which is used to decide what is
 * inside and outside of the implicit function. You can also reverse the
 * sense of what inside/outside is by setting the InsideOut instance
 * variable. (The clipping algorithm proceeds by computing an implicit
 * function value or using the input scalar data for each point in the
 * dataset. This is compared to the scalar value to determine
 * inside/outside.)
 *
 * This filter can be configured to compute a second output. The
 * second output is the part of the cell that is clipped away. Set the
 * GenerateClippedData boolean on if you wish to access this output data.
 *
 * @warning
 * svtkClipDataSet will triangulate all types of 3D cells (i.e., create
 * tetrahedra). This is true even if the cell is not actually cut. This
 * is necessary to preserve compatibility across face neighbors. 2D cells
 * will only be triangulated if the cutting function passes through them.
 *
 * @sa
 * svtkImplicitFunction svtkCutter svtkClipVolume svtkClipPolyData
 */

#ifndef svtkClipDataSet_h
#define svtkClipDataSet_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkCallbackCommand;
class svtkImplicitFunction;
class svtkIncrementalPointLocator;

class SVTKFILTERSGENERAL_EXPORT svtkClipDataSet : public svtkUnstructuredGridAlgorithm
{
public:
  svtkTypeMacro(svtkClipDataSet, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with user-specified implicit function; InsideOut turned off;
   * value set to 0.0; and generate clip scalars turned off.
   */
  static svtkClipDataSet* New();

  //@{
  /**
   * Set the clipping value of the implicit function (if clipping with
   * implicit function) or scalar value (if clipping with
   * scalars). The default value is 0.0. This value is ignored if
   * UseValueAsOffset is true and a clip function is defined.
   */
  svtkSetMacro(Value, double);
  svtkGetMacro(Value, double);
  //@}

  //@{
  /**
   * If UseValueAsOffset is true, Value is used as an offset parameter to
   * the implicit function. Otherwise, Value is used only when clipping
   * using a scalar array. Default is true.
   */
  svtkSetMacro(UseValueAsOffset, bool);
  svtkGetMacro(UseValueAsOffset, bool);
  svtkBooleanMacro(UseValueAsOffset, bool);
  //@}

  //@{
  /**
   * Set/Get the InsideOut flag. When off, a vertex is considered
   * inside the implicit function if its value is greater than the
   * Value ivar. When InsideOutside is turned on, a vertex is
   * considered inside the implicit function if its implicit function
   * value is less than or equal to the Value ivar.  InsideOut is off
   * by default.
   */
  svtkSetMacro(InsideOut, svtkTypeBool);
  svtkGetMacro(InsideOut, svtkTypeBool);
  svtkBooleanMacro(InsideOut, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the implicit function with which to perform the
   * clipping. If you do not define an implicit function,
   * then the selected input scalar data will be used for clipping.
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
   * contains the polygonal data that's been clipped away.
   */
  svtkSetMacro(GenerateClippedOutput, svtkTypeBool);
  svtkGetMacro(GenerateClippedOutput, svtkTypeBool);
  svtkBooleanMacro(GenerateClippedOutput, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the tolerance for merging clip intersection points that are near
   * the vertices of cells. This tolerance is used to prevent the generation
   * of degenerate primitives. Note that only 3D cells actually use this
   * instance variable.
   */
  svtkSetClampMacro(MergeTolerance, double, 0.0001, 0.25);
  svtkGetMacro(MergeTolerance, double);
  //@}

  /**
   * Return the Clipped output.
   */
  svtkUnstructuredGrid* GetClippedOutput();

  //@{
  /**
   * Specify a spatial locator for merging points. By default, an
   * instance of svtkMergePoints is used.
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

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  svtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkClipDataSet(svtkImplicitFunction* cf = nullptr);
  ~svtkClipDataSet() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  svtkImplicitFunction* ClipFunction;

  svtkIncrementalPointLocator* Locator;
  svtkTypeBool InsideOut;
  double Value;
  svtkTypeBool GenerateClipScalars;

  svtkTypeBool GenerateClippedOutput;
  double MergeTolerance;

  // Callback registered with the InternalProgressObserver.
  static void InternalProgressCallbackFunction(svtkObject*, unsigned long, void* clientdata, void*);
  void InternalProgressCallback(svtkAlgorithm* algorithm);
  // The observer to report progress from the internal readers.
  svtkCallbackCommand* InternalProgressObserver;

  // helper functions
  void ClipVolume(svtkDataSet* input, svtkUnstructuredGrid* output);

  int ClipPoints(
    svtkDataSet* input, svtkUnstructuredGrid* output, svtkInformationVector** inputVector);

  bool UseValueAsOffset;
  int OutputPointsPrecision;

private:
  svtkClipDataSet(const svtkClipDataSet&) = delete;
  void operator=(const svtkClipDataSet&) = delete;
};

#endif
