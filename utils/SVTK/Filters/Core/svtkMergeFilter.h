/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMergeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMergeFilter
 * @brief   extract separate components of data from different datasets
 *
 * svtkMergeFilter is a filter that extracts separate components of data from
 * different datasets and merges them into a single dataset. The output from
 * this filter is of the same type as the input (i.e., svtkDataSet.) It treats
 * both cell and point data set attributes.
 */

#ifndef svtkMergeFilter_h
#define svtkMergeFilter_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro

class svtkFieldList;

class SVTKFILTERSCORE_EXPORT svtkMergeFilter : public svtkDataSetAlgorithm
{
public:
  static svtkMergeFilter* New();
  svtkTypeMacro(svtkMergeFilter, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify object from which to extract geometry information.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetGeometryConnection for connecting the pipeline.
   */
  void SetGeometryInputData(svtkDataSet* input) { this->SetInputData(input); }
  svtkDataSet* GetGeometry();
  //@}

  /**
   * Specify object from which to extract geometry information.
   * Equivalent to SetInputConnection(0, algOutput)
   */
  void SetGeometryConnection(svtkAlgorithmOutput* algOutput) { this->SetInputConnection(algOutput); }

  //@{
  /**
   * Specify object from which to extract scalar information.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetScalarConnection for connecting the pipeline.
   */
  void SetScalarsData(svtkDataSet*);
  svtkDataSet* GetScalars();
  //@}

  /**
   * Specify object from which to extract scalar information.
   * Equivalent to SetInputConnection(1, algOutput)
   */
  void SetScalarsConnection(svtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(1, algOutput);
  }

  //@{
  /**
   * Set / get the object from which to extract vector information.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetVectorsConnection for connecting the pipeline.
   */
  void SetVectorsData(svtkDataSet*);
  svtkDataSet* GetVectors();
  //@}

  /**
   * Set the connection from which to extract vector information.
   * Equivalent to SetInputConnection(2, algOutput)
   */
  void SetVectorsConnection(svtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(2, algOutput);
  }

  //@{
  /**
   * Set / get the object from which to extract normal information.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetNormalsConnection for connecting the pipeline.
   */
  void SetNormalsData(svtkDataSet*);
  svtkDataSet* GetNormals();
  //@}

  /**
   * Set  the connection from which to extract normal information.
   * Equivalent to SetInputConnection(3, algOutput)
   */
  void SetNormalsConnection(svtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(3, algOutput);
  }

  //@{
  /**
   * Set / get the object from which to extract texture coordinates
   * information.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetTCoordsConnection for connecting the pipeline.
   */
  void SetTCoordsData(svtkDataSet*);
  svtkDataSet* GetTCoords();
  //@}

  /**
   * Set the connection from which to extract texture coordinates
   * information.
   * Equivalent to SetInputConnection(4, algOutput)
   */
  void SetTCoordsConnection(svtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(4, algOutput);
  }

  //@{
  /**
   * Set / get the object from which to extract tensor data.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetTensorsConnection for connecting the pipeline.
   */
  void SetTensorsData(svtkDataSet*);
  svtkDataSet* GetTensors();
  //@}

  /**
   * Set the connection from which to extract tensor data.
   * Equivalent to SetInputConnection(5, algOutput)
   */
  void SetTensorsConnection(svtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(5, algOutput);
  }

  /**
   * Set the object from which to extract a field and the name
   * of the field. Note that this does not create pipeline
   * connectivity.
   */
  void AddField(const char* name, svtkDataSet* input);

protected:
  svtkMergeFilter();
  ~svtkMergeFilter() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkFieldList* FieldList;

private:
  svtkMergeFilter(const svtkMergeFilter&) = delete;
  void operator=(const svtkMergeFilter&) = delete;
};

#endif
