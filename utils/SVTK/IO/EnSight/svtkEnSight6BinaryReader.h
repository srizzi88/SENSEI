/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEnSight6BinaryReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkEnSight6BinaryReader
 * @brief   class to read binary EnSight6 files
 *
 * svtkEnSight6BinaryReader is a class to read binary EnSight6 files into svtk.
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

#ifndef svtkEnSight6BinaryReader_h
#define svtkEnSight6BinaryReader_h

#include "svtkEnSightReader.h"
#include "svtkIOEnSightModule.h" // For export macro

class svtkMultiBlockDataSet;
class svtkIdTypeArray;
class svtkPoints;

class SVTKIOENSIGHT_EXPORT svtkEnSight6BinaryReader : public svtkEnSightReader
{
public:
  static svtkEnSight6BinaryReader* New();
  svtkTypeMacro(svtkEnSight6BinaryReader, svtkEnSightReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkEnSight6BinaryReader();
  ~svtkEnSight6BinaryReader() override;

  // Returns 1 if successful.  Sets file size as a side action.
  int OpenFile(const char* filename);

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

  /**
   * Internal function to read in a line up to 80 characters.
   * Returns zero if there was an error.
   */
  int ReadLine(char result[80]);

  /**
   * Internal function to read in a single integer.
   * Tries to determine the byte order of this file.
   * Returns zero if there was an error.
   */
  int ReadIntNumber(int* result);

  /**
   * Internal function to read in an integer array.
   * Returns zero if there was an error.
   */
  int ReadIntArray(int* result, int numInts);

  /**
   * Internal function to read in a float array.
   * Returns zero if there was an error.
   */
  int ReadFloatArray(float* result, int numFloats);

  //@{
  /**
   * Read to the next time step in the geometry file.
   */
  int SkipTimeStep();
  int SkipStructuredGrid(char line[256]);
  int SkipUnstructuredGrid(char line[256]);
  //@}

  // global list of points for the unstructured parts of the model
  int NumberOfUnstructuredPoints;
  svtkPoints* UnstructuredPoints;
  svtkIdTypeArray* UnstructuredNodeIds; // matching of node ids to point ids

  int ElementIdsListed;

  // The size of the file is used to choose byte order.
  svtkTypeUInt64 FileSize;

  istream* BinaryIFile;

private:
  svtkEnSight6BinaryReader(const svtkEnSight6BinaryReader&) = delete;
  void operator=(const svtkEnSight6BinaryReader&) = delete;
};

#endif
