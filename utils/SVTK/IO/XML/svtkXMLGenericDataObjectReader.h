/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLGenericDataObjectReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLGenericDataObjectReader
 * @brief   Read any type of svtk data object
 *
 * svtkXMLGenericDataObjectReader reads any type of svtk data object encoded
 * in XML format.
 *
 * @sa
 * svtkGenericDataObjectReader
 */

#ifndef svtkXMLGenericDataObjectReader_h
#define svtkXMLGenericDataObjectReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLDataReader.h"

class svtkHierarchicalBoxDataSet;
class svtkMultiBlockDataSet;
class svtkImageData;
class svtkPolyData;
class svtkRectilinearGrid;
class svtkStructuredGrid;
class svtkUnstructuredGrid;

class SVTKIOXML_EXPORT svtkXMLGenericDataObjectReader : public svtkXMLDataReader
{
public:
  svtkTypeMacro(svtkXMLGenericDataObjectReader, svtkXMLDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLGenericDataObjectReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  svtkDataObject* GetOutput();
  svtkDataObject* GetOutput(int idx);
  //@}

  //@{
  /**
   * Get the output as various concrete types. This method is typically used
   * when you know exactly what type of data is being read.  Otherwise, use
   * the general GetOutput() method. If the wrong type is used nullptr is
   * returned.  (You must also set the filename of the object prior to
   * getting the output.)
   */
  svtkHierarchicalBoxDataSet* GetHierarchicalBoxDataSetOutput();
  svtkImageData* GetImageDataOutput();
  svtkMultiBlockDataSet* GetMultiBlockDataSetOutput();
  svtkPolyData* GetPolyDataOutput();
  svtkRectilinearGrid* GetRectilinearGridOutput();
  svtkStructuredGrid* GetStructuredGridOutput();
  svtkUnstructuredGrid* GetUnstructuredGridOutput();
  //@}

  /**
   * Overridden method.
   */
  svtkIdType GetNumberOfPoints() override;

  /**
   * Overridden method.
   */
  svtkIdType GetNumberOfCells() override;

  /**
   * Overridden method. Not Used. Delegated.
   */
  void SetupEmptyOutput() override;

  /**
   * This method can be used to find out the type of output expected without
   * needing to read the whole file.
   */
  virtual int ReadOutputType(const char* name, bool& parallel);

protected:
  svtkXMLGenericDataObjectReader();
  ~svtkXMLGenericDataObjectReader() override;

  /**
   * Overridden method. Not used. Return "svtkDataObject".
   */
  const char* GetDataSetName() override;

  int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  virtual int RequestUpdateExtent(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  svtkXMLReader* Reader; // actual reader

private:
  svtkXMLGenericDataObjectReader(const svtkXMLGenericDataObjectReader&) = delete;
  void operator=(const svtkXMLGenericDataObjectReader&) = delete;
};

#endif
