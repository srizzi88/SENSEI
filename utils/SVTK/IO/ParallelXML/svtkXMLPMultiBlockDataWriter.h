/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPMultiBlockDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPMultiBlockDataWriter
 * @brief   parallel writer for
 * svtkHierarchicalBoxDataSet.
 *
 * svtkXMLPCompositeDataWriter writes (in parallel or serially) the SVTK XML
 * multi-group, multi-block hierarchical and hierarchical box files. XML
 * multi-group data files are meta-files that point to a list of serial SVTK
 * XML files.
 */

#ifndef svtkXMLPMultiBlockDataWriter_h
#define svtkXMLPMultiBlockDataWriter_h

#include "svtkIOParallelXMLModule.h" // For export macro
#include "svtkXMLMultiBlockDataWriter.h"

class svtkCompositeDataSet;
class svtkMultiProcessController;

class SVTKIOPARALLELXML_EXPORT svtkXMLPMultiBlockDataWriter : public svtkXMLMultiBlockDataWriter
{
public:
  static svtkXMLPMultiBlockDataWriter* New();
  svtkTypeMacro(svtkXMLPMultiBlockDataWriter, svtkXMLMultiBlockDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the number of pieces that are being written in parallel.
   */
  svtkSetMacro(NumberOfPieces, int);
  svtkGetMacro(NumberOfPieces, int);
  //@}

  //@{
  /**
   * Get/Set the range of pieces assigned to this writer.
   */
  svtkSetMacro(StartPiece, int);
  svtkGetMacro(StartPiece, int);
  //@}

  //@{
  /**
   * Controller used to communicate data type of blocks.
   * By default, the global controller is used. If you want another
   * controller to be used, set it with this.
   * If no controller is set, only the local blocks will be written
   * to the meta-file.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  /**
   * Set whether this instance will write the meta-file. WriteMetaFile
   * is set to flag only on process 0 and all other processes have
   * WriteMetaFile set to 0 by default.
   */
  void SetWriteMetaFile(int flag) override;

  // See the svtkAlgorithm for a description of what these do
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkXMLPMultiBlockDataWriter();
  ~svtkXMLPMultiBlockDataWriter() override;

  /**
   * Determine the data types for each of the leaf nodes.
   * Currently each process requires this information in order to
   * simplify creating the file names for both the metadata file
   * as well as the actual dataset files.  It takes into account
   * that a piece of a dataset may be distributed in multiple pieces
   * over multiple processes.
   */
  void FillDataTypes(svtkCompositeDataSet*) override;

  svtkMultiProcessController* Controller;

  /**
   * Internal method called recursively to create the xml tree for
   * the children of compositeData as well as write the actual data
   * set files.  element will only have added nested information.
   * writerIdx is the global piece index used to create unique
   * filenames for each file written.  This function returns 0 if
   * no files were written from compositeData.  Process 0 creates
   * the metadata for all of the processes/files.
   */
  int WriteComposite(
    svtkCompositeDataSet* compositeData, svtkXMLDataElement* parent, int& currentFileIndex) override;

  /**
   * Internal method to write a non svtkCompositeDataSet subclass as
   * well as add in the file name to the metadata file.
   * Element is the containing XML metadata element that may
   * have data overwritten and added to (the index XML attribute
   * should not be touched though).  writerIdx is the piece index
   * that gets incremented for the globally numbered piece.
   * If this piece exists on multiple processes than it also takes
   * care of that in the metadata description. This function returns
   * 0 if no file was written.
   */
  int ParallelWriteNonCompositeData(
    svtkDataObject* dObj, svtkXMLDataElement* parentXML, int currentFileIndex);

  /**
   * Return the name of the file given the currentFileIndex (also the current
   * globally numbered piece index), the procId the file exists on, and
   * the dataSetType.
   */
  virtual svtkStdString CreatePieceFileName(int currentFileIndex, int procId, int dataSetType);

  /**
   * Utility function to remove any already written files
   * in case writer failed.
   */
  void RemoveWrittenFiles(const char* subDirectory) override;

  //@{
  /**
   * Piece information.
   */
  int StartPiece;
  int NumberOfPieces;
  //@}

private:
  svtkXMLPMultiBlockDataWriter(const svtkXMLPMultiBlockDataWriter&) = delete;
  void operator=(const svtkXMLPMultiBlockDataWriter&) = delete;

  class svtkInternal;
  svtkInternal* XMLPMultiBlockDataWriterInternal;
};

#endif
