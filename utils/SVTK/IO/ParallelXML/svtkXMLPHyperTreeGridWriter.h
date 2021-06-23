/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPHyperTreeGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPHyperTreeGridWriter
 * @brief   Write PSVTK XML HyperTreeGrid files.
 *
 * svtkXMLPHyperTreeGridWriter writes the PSVTK XML HyperTreeGrid
 * file format.  One hypertree grid input can be written into a
 * parallel file format with any number of pieces spread across files.
 * The standard extension for this writer's file format is "phtg".
 * This writer uses svtkXMLHyperTreeGridWriter to write the
 * individual piece files.
 *
 * @sa
 * svtkXMLHyperTreeGridWriter
 */

#ifndef svtkXMLPHyperTreeGridWriter_h
#define svtkXMLPHyperTreeGridWriter_h

#include "svtkXMLPDataObjectWriter.h"

class svtkCallbackCommand;
class svtkMultiProcessController;
class svtkHyperTreeGrid;
class svtkXMLHyperTreeGridWriter;
class svtkXMLPDataObjectWriter;

class SVTKIOPARALLELXML_EXPORT svtkXMLPHyperTreeGridWriter : public svtkXMLPDataObjectWriter
{
public:
  static svtkXMLPHyperTreeGridWriter* New();
  svtkTypeMacro(svtkXMLPHyperTreeGridWriter, svtkXMLPDataObjectWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get/Set the writer's input.
   */
  svtkHyperTreeGrid* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  svtkXMLPHyperTreeGridWriter();
  ~svtkXMLPHyperTreeGridWriter() override;

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
  svtkXMLHyperTreeGridWriter* CreateHyperTreeGridPieceWriter(int index);

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

private:
  svtkXMLPHyperTreeGridWriter(const svtkXMLPHyperTreeGridWriter&) = delete;
  void operator=(const svtkXMLPHyperTreeGridWriter&) = delete;

  /**
   * Initializes PieceFileNameExtension.
   */
  void SetupPieceFileNameExtension() override;
};

#endif
