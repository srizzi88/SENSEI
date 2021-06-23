/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLDataObjectWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLDataObjectWriter
 * @brief   Write any type of SVTK XML file.
 *
 * svtkXMLDataObjectWriter is a wrapper around the SVTK XML file format
 * writers.  Given an input svtkDataSet, the correct writer is
 * automatically selected based on the type of input.
 *
 * @sa
 * svtkXMLImageDataWriter svtkXMLStructuredGridWriter
 * svtkXMLRectilinearGridWriter svtkXMLPolyDataWriter
 * svtkXMLUnstructuredGridWriter
 */

#ifndef svtkXMLDataObjectWriter_h
#define svtkXMLDataObjectWriter_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLWriter.h"

class svtkCallbackCommand;

class SVTKIOXML_EXPORT svtkXMLDataObjectWriter : public svtkXMLWriter
{
public:
  svtkTypeMacro(svtkXMLDataObjectWriter, svtkXMLWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLDataObjectWriter* New();

  /**
   * Get/Set the writer's input.
   */
  svtkDataSet* GetInput();

  /**
   * Creates a writer for the given dataset type. May return nullptr for
   * unsupported/unrecognized dataset types. Returns a new instance. The caller
   * is responsible of calling svtkObject::Delete() or svtkObject::UnRegister() on
   * it when done.
   */
  static svtkXMLWriter* NewWriter(int dataset_type);

protected:
  svtkXMLDataObjectWriter();
  ~svtkXMLDataObjectWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

  // Override writing method from superclass.
  int WriteInternal() override;

  // Dummies to satisfy pure virtuals from superclass.
  const char* GetDataSetName() override;
  const char* GetDefaultFileExtension() override;

  // Callback registered with the InternalProgressObserver.
  static void ProgressCallbackFunction(svtkObject*, unsigned long, void*, void*);
  // Progress callback from internal writer.
  virtual void ProgressCallback(svtkAlgorithm* w);

  // The observer to report progress from the internal writer.
  svtkCallbackCommand* InternalProgressObserver;

private:
  svtkXMLDataObjectWriter(const svtkXMLDataObjectWriter&) = delete;
  void operator=(const svtkXMLDataObjectWriter&) = delete;
};

#endif
