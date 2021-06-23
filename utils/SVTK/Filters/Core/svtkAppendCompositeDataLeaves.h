/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAppendCompositeDataLeaves.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAppendCompositeDataLeaves
 * @brief   appends one or more composite datasets with the same structure together into a single
 * output composite dataset
 *
 * svtkAppendCompositeDataLeaves is a filter that takes input composite datasets with the same
 * structure: (1) the same number of entries and -- if any children are composites -- the
 * same constraint holds from them; and (2) the same type of dataset at each position.
 * It then creates an output dataset with the same structure whose leaves contain all the
 * cells from the datasets at the corresponding leaves of the input datasets.
 *
 * Currently, this filter only supports "appending" of a few types for the leaf
 * nodes and the logic used for each supported data type is as follows:
 *
 * \li svtkUnstructuredGrid - appends all unstructured grids from the leaf
 *     location on all inputs into a single unstructured grid for the
 *     corresponding location in the output composite dataset. PointData and
 *     CellData arrays are extracted and appended only if they are available in
 *     all datasets.(For example, if one dataset has scalars but another does
 *     not, scalars will not be appended.)
 *
 * \li svtkPolyData - appends all polydatas from the leaf location on all inputs
 *     into a single polydata for the corresponding location in the output
 *     composite dataset. PointData and CellData arrays are extracted and
 *     appended only if they are available in all datasets.(For example, if one
 *     dataset has scalars but another does not, scalars will not be appended.)
 *
 * \li svtkImageData/svtkUniformGrid - simply passes the first non-null
 *     grid for a particular location to corresponding location in the output.
 *
 * \li svtkTable - simply passes the first non-null svtkTable for a particular
 *     location to the corresponding location in the output.
 *
 * Other types of leaf datasets will be ignored and their positions in the
 * output dataset will be nullptr pointers.
 *
 * @sa
 * svtkAppendPolyData svtkAppendFilter
 */

#ifndef svtkAppendCompositeDataLeaves_h
#define svtkAppendCompositeDataLeaves_h

#include "svtkCompositeDataSetAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro

class svtkCompositeDataIterator;
class svtkDataSet;

class SVTKFILTERSCORE_EXPORT svtkAppendCompositeDataLeaves : public svtkCompositeDataSetAlgorithm
{
public:
  static svtkAppendCompositeDataLeaves* New();

  svtkTypeMacro(svtkAppendCompositeDataLeaves, svtkCompositeDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/get whether the field data of each dataset in the composite dataset is copied to the
   * output. If AppendFieldData is non-zero, then field data arrays from all the inputs are added to
   * the output. If there are duplicates, the array on the first input encountered is taken.
   */
  svtkSetMacro(AppendFieldData, svtkTypeBool);
  svtkGetMacro(AppendFieldData, svtkTypeBool);
  svtkBooleanMacro(AppendFieldData, svtkTypeBool);
  //@}

protected:
  svtkAppendCompositeDataLeaves();
  ~svtkAppendCompositeDataLeaves() override;

  /**
   * Since svtkCompositeDataSet is an abstract class and we output the same types as the input,
   * we must override the default implementation.
   */
  int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Iterates over the datasets and appends corresponding notes.
   */
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * The input is repeatable, so we override the default implementation.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * When leaf nodes are unstructured grids, this uses a svtkAppendFilter to merge them.
   */
  virtual void AppendUnstructuredGrids(svtkInformationVector* inputVector, int i, int numInputs,
    svtkCompositeDataIterator* iter, svtkCompositeDataSet* output);

  /**
   * When leaf nodes are polydata, this uses a svtkAppendPolyData to merge them.
   */
  virtual void AppendPolyData(svtkInformationVector* inputVector, int i, int numInputs,
    svtkCompositeDataIterator* iter, svtkCompositeDataSet* output);

  /**
   * Both AppendUnstructuredGrids and AppendPolyData call AppendFieldDataArrays. If
   * AppendFieldData is non-zero, then field data arrays from all the inputs are added
   * to the output. If there are duplicates, the array on the first input encountered
   * is taken.
   */
  virtual void AppendFieldDataArrays(svtkInformationVector* inputVector, int i, int numInputs,
    svtkCompositeDataIterator* iter, svtkDataSet* dset);

  svtkTypeBool AppendFieldData;

private:
  svtkAppendCompositeDataLeaves(const svtkAppendCompositeDataLeaves&) = delete;
  void operator=(const svtkAppendCompositeDataLeaves&) = delete;
};

#endif // svtkAppendCompositeDataLeaves_h
