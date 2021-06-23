/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEnSight6Reader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkEnSight6Reader
 * @brief   class to read EnSight6 files
 *
 * svtkEnSight6Reader is a class to read EnSight6 files into svtk.
 * Because the different parts of the EnSight data can be of various data
 * types, this reader produces multiple outputs, one per part in the input
 * file.
 * All variable information is being stored in field data.  The descriptions
 * listed in the case file are used as the array names in the field data.
 * For complex vector variables, the description is appended with _r (for the
 * array of real values) and _i (for the array if imaginary values).  Complex
 * scalar variables are stored as a single array with 2 components, real and
 * imaginary, listed in that order.
 * @warning
 * You must manually call Update on this reader and then connect the rest
 * of the pipeline because (due to the nature of the file format) it is
 * not possible to know ahead of time how many outputs you will have or
 * what types they will be.
 * This reader can only handle static EnSight datasets (both static geometry
 * and variables).
 */

#ifndef svtkEnSight6Reader_h
#define svtkEnSight6Reader_h

#include "svtkEnSightReader.h"
#include "svtkIOEnSightModule.h" // For export macro

class svtkMultiBlockDataSet;
class svtkIdTypeArray;
class svtkPoints;

class SVTKIOENSIGHT_EXPORT svtkEnSight6Reader : public svtkEnSightReader
{
public:
  static svtkEnSight6Reader* New();
  svtkTypeMacro(svtkEnSight6Reader, svtkEnSightReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkEnSight6Reader();
  ~svtkEnSight6Reader() override;

  /**
   * Read the geometry file.  If an error occurred, 0 is returned; otherwise 1.
   */
  int ReadGeometryFile(const char* fileName, int timeStep, svtkMultiBlockDataSet* output) override;

  /**
   * Read the measured geometry file.  If an error occurred, 0 is returned;
   * otherwise 1.
   */
  int ReadMeasuredGeometryFile(
    const char* fileName, int timeStep, svtkMultiBlockDataSet* output) override;

  /**
   * Read scalars per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.  If there will be more than one component in
   * the scalars array, we assume that 0 is the first component added to the array.
   */
  int ReadScalarsPerNode(const char* fileName, const char* description, int timeStep,
    svtkMultiBlockDataSet* output, int measured = 0, int numberOfComponents = 1,
    int component = 0) override;

  /**
   * Read vectors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadVectorsPerNode(const char* fileName, const char* description, int timeStep,
    svtkMultiBlockDataSet* output, int measured = 0) override;

  /**
   * Read tensors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadTensorsPerNode(const char* fileName, const char* description, int timeStep,
    svtkMultiBlockDataSet* output) override;

  /**
   * Read scalars per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.  If there will be more than one component in the
   * scalars array, we assume that 0 is the first component added to the array.
   */
  int ReadScalarsPerElement(const char* fileName, const char* description, int timeStep,
    svtkMultiBlockDataSet* output, int numberOfComponents = 1, int component = 0) override;

  /**
   * Read vectors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadVectorsPerElement(const char* fileName, const char* description, int timeStep,
    svtkMultiBlockDataSet* output) override;

  /**
   * Read tensors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadTensorsPerElement(const char* fileName, const char* description, int timeStep,
    svtkMultiBlockDataSet* output) override;

  /**
   * Read an unstructured part (partId) from the geometry file and create a
   * svtkUnstructuredGrid output.  Return 0 if EOF reached.
   */
  int CreateUnstructuredGridOutput(
    int partId, char line[256], const char* name, svtkMultiBlockDataSet* output) override;

  /**
   * Read a structured part from the geometry file and create a
   * svtkStructuredGridOutput.  Return 0 if EOF reached.
   */
  int CreateStructuredGridOutput(
    int partId, char line[256], const char* name, svtkMultiBlockDataSet* output) override;

  // global list of points for the unstructured parts of the model
  int NumberOfUnstructuredPoints;
  svtkPoints* UnstructuredPoints;
  svtkIdTypeArray* UnstructuredNodeIds; // matching of node ids to point ids
private:
  svtkEnSight6Reader(const svtkEnSight6Reader&) = delete;
  void operator=(const svtkEnSight6Reader&) = delete;
};

#endif
