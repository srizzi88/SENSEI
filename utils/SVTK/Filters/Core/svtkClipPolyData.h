/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClipPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkClipPolyData
 * @brief   clip polygonal data with user-specified implicit function or input scalar data
 *
 * svtkClipPolyData is a filter that clips polygonal data using either
 * any subclass of svtkImplicitFunction, or the input scalar
 * data. Clipping means that it actually "cuts" through the cells of
 * the dataset, returning everything inside of the specified implicit
 * function (or greater than the scalar value) including "pieces" of
 * a cell. (Compare this with svtkExtractGeometry, which pulls out
 * entire, uncut cells.) The output of this filter is polygonal data.
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
 * You can also specify a scalar value, which is used to
 * decide what is inside and outside of the implicit function. You can
 * also reverse the sense of what inside/outside is by setting the
 * InsideOut instance variable. (The cutting algorithm proceeds by
 * computing an implicit function value or using the input scalar data
 * for each point in the dataset. This is compared to the scalar value
 * to determine inside/outside.)
 *
 * This filter can be configured to compute a second output. The
 * second output is the polygonal data that is clipped away. Set the
 * GenerateClippedData boolean on if you wish to access this output data.
 *
 * @warning
 * In order to cut all types of cells in polygonal data, svtkClipPolyData
 * triangulates some cells, and then cuts the resulting simplices
 * (i.e., points, lines, and triangles). This means that the resulting
 * output may consist of different cell types than the input data.
 *
 * @sa
 * svtkImplicitFunction svtkCutter svtkClipVolume svtkExtractGeometry
 */

#ifndef svtkClipPolyData_h
#define svtkClipPolyData_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkImplicitFunction;
class svtkIncrementalPointLocator;

class SVTKFILTERSCORE_EXPORT svtkClipPolyData : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkClipPolyData, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with user-specified implicit function; InsideOut turned off;
   * value set to 0.0; GenerateClipScalars turned off; GenerateClippedOutput
   * turned off.
   */
  static svtkClipPolyData* New();

  //@{
  /**
   * Set the clipping value of the implicit function (if clipping with
   * implicit function) or scalar value (if clipping with
   * scalars). The default value is 0.0.
   */
  svtkSetMacro(Value, double);
  svtkGetMacro(Value, double);
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
   * clipping. If you do not define an implicit function, then the input
   * scalar data will be used for clipping.
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
   * GenerateClipScalars is off by default.
   */
  svtkSetMacro(GenerateClipScalars, svtkTypeBool);
  svtkGetMacro(GenerateClipScalars, svtkTypeBool);
  svtkBooleanMacro(GenerateClipScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * Control whether a second output is generated. The second output
   * contains the polygonal data that's been clipped away.
   * GenerateClippedOutput is off by default.
   */
  svtkSetMacro(GenerateClippedOutput, svtkTypeBool);
  svtkGetMacro(GenerateClippedOutput, svtkTypeBool);
  svtkBooleanMacro(GenerateClippedOutput, svtkTypeBool);
  //@}

  /**
   * Return the Clipped output.
   */
  svtkPolyData* GetClippedOutput();

  /**
   * Return the output port (a svtkAlgorithmOutput) of the clipped output.
   */
  svtkAlgorithmOutput* GetClippedOutputPort() { return this->GetOutputPort(1); }

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
   * the available precision settings. OutputPointsPrecision is DEFAULT_PRECISION
   * by default.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkClipPolyData(svtkImplicitFunction* cf = nullptr);
  ~svtkClipPolyData() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  svtkImplicitFunction* ClipFunction;

  svtkIncrementalPointLocator* Locator;
  svtkTypeBool InsideOut;
  double Value;
  svtkTypeBool GenerateClipScalars;
  svtkTypeBool GenerateClippedOutput;
  int OutputPointsPrecision;

private:
  svtkClipPolyData(const svtkClipPolyData&) = delete;
  void operator=(const svtkClipPolyData&) = delete;
};

#endif
