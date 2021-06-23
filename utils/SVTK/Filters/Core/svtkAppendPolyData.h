/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAppendPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAppendPolyData
 * @brief   appends one or more polygonal datasets together
 *
 *
 * svtkAppendPolyData is a filter that appends one of more polygonal datasets
 * into a single polygonal dataset. All geometry is extracted and appended,
 * but point and cell attributes (i.e., scalars, vectors, normals) are
 * extracted and appended only if all datasets have the point and/or cell
 * attributes available.  (For example, if one dataset has point scalars but
 * another does not, point scalars will not be appended.)
 *
 * @sa
 * svtkAppendFilter
 */

#ifndef svtkAppendPolyData_h
#define svtkAppendPolyData_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkCellArray;
class svtkDataArray;
class svtkPoints;
class svtkPolyData;

class SVTKFILTERSCORE_EXPORT svtkAppendPolyData : public svtkPolyDataAlgorithm
{
public:
  static svtkAppendPolyData* New();

  svtkTypeMacro(svtkAppendPolyData, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * UserManagedInputs allows the user to set inputs by number instead of
   * using the AddInput/RemoveInput functions. Calls to
   * SetNumberOfInputs/SetInputConnectionByNumber should not be mixed with calls
   * to AddInput/RemoveInput. By default, UserManagedInputs is false.
   */
  svtkSetMacro(UserManagedInputs, svtkTypeBool);
  svtkGetMacro(UserManagedInputs, svtkTypeBool);
  svtkBooleanMacro(UserManagedInputs, svtkTypeBool);
  //@}

  /**
   * Add a dataset to the list of data to append. Should not be
   * used when UserManagedInputs is true, use SetInputByNumber instead.
   */
  void AddInputData(svtkPolyData*);

  /**
   * Remove a dataset from the list of data to append. Should not be
   * used when UserManagedInputs is true, use SetInputByNumber (nullptr) instead.
   */
  void RemoveInputData(svtkPolyData*);

  //@{
  /**
   * Get any input of this filter.
   */
  svtkPolyData* GetInput(int idx);
  svtkPolyData* GetInput() { return this->GetInput(0); }
  //@}

  /**
   * Directly set(allocate) number of inputs, should only be used
   * when UserManagedInputs is true.
   */
  void SetNumberOfInputs(int num);

  // Set Nth input, should only be used when UserManagedInputs is true.
  void SetInputConnectionByNumber(int num, svtkAlgorithmOutput* input);
  void SetInputDataByNumber(int num, svtkPolyData* ds);

  //@{
  /**
   * ParallelStreaming is for a particular application.
   * It causes this filter to ask for a different piece
   * from each of its inputs.  If all the inputs are the same,
   * then the output of this append filter is the whole dataset
   * pieced back together.  Duplicate points are create
   * along the seams.  The purpose of this feature is to get
   * data parallelism at a course scale.  Each of the inputs
   * can be generated in a different process at the same time.
   */
  svtkSetMacro(ParallelStreaming, svtkTypeBool);
  svtkGetMacro(ParallelStreaming, svtkTypeBool);
  svtkBooleanMacro(ParallelStreaming, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

  int ExecuteAppend(svtkPolyData* output, svtkPolyData* inputs[], int numInputs)
    SVTK_SIZEHINT(inputs, numInputs);

protected:
  svtkAppendPolyData();
  ~svtkAppendPolyData() override;

  // Flag for selecting parallel streaming behavior
  svtkTypeBool ParallelStreaming;
  int OutputPointsPrecision;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

  // An efficient templated way to append data.
  void AppendData(svtkDataArray* dest, svtkDataArray* src, svtkIdType offset);

  // An efficient way to append cells.
  void AppendCells(svtkCellArray* dest, svtkCellArray* src, svtkIdType offset);

private:
  // hide the superclass' AddInput() from the user and the compiler
  void AddInputData(svtkDataObject*)
  {
    svtkErrorMacro(<< "AddInput() must be called with a svtkPolyData not a svtkDataObject.");
  }

  svtkTypeBool UserManagedInputs;

private:
  svtkAppendPolyData(const svtkAppendPolyData&) = delete;
  void operator=(const svtkAppendPolyData&) = delete;
};

#endif
