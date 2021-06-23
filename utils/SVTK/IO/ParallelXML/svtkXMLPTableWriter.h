/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPTableWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPTableWriter
 * @brief   Write PSVTK XML UnstructuredGrid files.
 *
 * svtkXMLPTableWriter writes the PSVTK XML Table
 * file format.  One table input can be written into a
 * parallel file format with any number of pieces spread across files.
 * The standard extension for this writer's file format is "pvtt".
 * This writer uses svtkXMLTableWriter to write the
 * individual piece files.
 *
 * @sa
 * svtkXMLTableWriter
 */

#ifndef svtkXMLPTableWriter_h
#define svtkXMLPTableWriter_h

#include "svtkIOParallelXMLModule.h" // For export macro
#include "svtkXMLPDataObjectWriter.h"

class svtkCallbackCommand;
class svtkMultiProcessController;
class svtkTable;
class svtkXMLTableWriter;
class svtkXMLPDataObjectWriter;

class SVTKIOPARALLELXML_EXPORT svtkXMLPTableWriter : public svtkXMLPDataObjectWriter
{
public:
  static svtkXMLPTableWriter* New();
  svtkTypeMacro(svtkXMLPTableWriter, svtkXMLPDataObjectWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get/Set the writer's input.
   */
  svtkTable* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  svtkXMLPTableWriter();
  ~svtkXMLPTableWriter() override;

  /**
   * see algorithm for more info
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * Return the type of data being actually written
   */
  const char* GetDataSetName() override;

  /**
   * Create a writer for the piece at a given index
   */
  svtkXMLWriter* CreatePieceWriter(int index);

  /**
   * Create a table writer for the actual piece. Used by
   * CreatePieceWriter(int index)
   */
  svtkXMLTableWriter* CreateTablePieceWriter();

  /**
   * Write a piece of the dataset on disk. Called by WritePieceInternal()
   */
  int WritePiece(int index) override;

  /**
   * Method called by the superclass::WriteInternal(). Write a piece using
   * WritePiece(int index).
   */
  int WritePieceInternal() override;

  /**
   * Write Data associated with the input dataset
   */
  void WritePData(svtkIndent indent) override;

  /**
   * Write RowData. Called by WritePData(svtkIndent indent)
   */
  void WritePRowData(svtkDataSetAttributes* ds, svtkIndent indent);

private:
  svtkXMLPTableWriter(const svtkXMLPTableWriter&) = delete;
  void operator=(const svtkXMLPTableWriter&) = delete;

  /**
   * Initializes PieceFileNameExtension.
   */
  void SetupPieceFileNameExtension() override;
};

#endif
