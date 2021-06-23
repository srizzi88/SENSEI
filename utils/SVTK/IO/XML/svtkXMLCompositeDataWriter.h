/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLCompositeDataWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLCompositeDataWriter
 * @brief   Writer for multi-group datasets
 *
 * svtkXMLCompositeDataWriter writes (serially) the SVTK XML multi-group,
 * multi-block hierarchical and hierarchical box files. XML multi-group
 * data files are meta-files that point to a list of serial SVTK XML files.
 * @sa
 * svtkXMLPCompositeDataWriter
 */

#ifndef svtkXMLCompositeDataWriter_h
#define svtkXMLCompositeDataWriter_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkStdString.h"   // needed for svtkStdString.
#include "svtkXMLWriter.h"

class svtkCallbackCommand;
class svtkCompositeDataSet;
class svtkXMLDataElement;
class svtkXMLCompositeDataWriterInternals;

class SVTKIOXML_EXPORT svtkXMLCompositeDataWriter : public svtkXMLWriter
{
public:
  svtkTypeMacro(svtkXMLCompositeDataWriter, svtkXMLWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

  /**
   * Get/Set the number of pieces into which the inputs are split.
   */

  //@{
  /**
   * Get/Set the number of ghost levels to be written.
   */
  svtkGetMacro(GhostLevel, int);
  svtkSetMacro(GhostLevel, int);
  //@}

  //@{
  /**
   * Get/Set whether this instance will write the meta-file.
   */
  svtkGetMacro(WriteMetaFile, int);
  virtual void SetWriteMetaFile(int flag);
  //@}

  /**
   * See the svtkAlgorithm for a description of what these do
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkXMLCompositeDataWriter();
  ~svtkXMLCompositeDataWriter() override;

  /**
   * Methods to define the file's major and minor version numbers.
   * Major version incremented since v0.1 composite data readers cannot read
   * the files written by this new reader.
   */
  int GetDataSetMajorVersion() override { return 1; }
  int GetDataSetMinorVersion() override { return 0; }

  /**
   * Create a filename for the given index.
   */
  svtkStdString CreatePieceFileName(int Piece);

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  int WriteData() override;
  const char* GetDataSetName() override;

  // Create a default executive.
  svtkExecutive* CreateDefaultExecutive() override;

  svtkInformation* InputInformation;

  /**
   * Determine the data types for each of the leaf nodes.
   */
  virtual void FillDataTypes(svtkCompositeDataSet*);

  /**
   * Returns the number of leaf nodes (also includes empty leaf nodes).
   */
  unsigned int GetNumberOfDataTypes();

  /**
   * Returns the array pointer to the array of leaf nodes.
   */
  int* GetDataTypesPointer();

  // Methods to create the set of writers matching the set of inputs.
  void CreateWriters(svtkCompositeDataSet*);
  svtkXMLWriter* GetWriter(int index);

  // Methods to help construct internal file names.
  void SplitFileName();
  const char* GetFilePrefix();
  const char* GetFilePath();

  /**
   * Returns the default extension to use for the given dataset type.
   * Returns nullptr if an extension cannot be determined.
   */
  const char* GetDefaultFileExtensionForDataSet(int dataset_type);

  /**
   * Write the collection file if it is requested.
   * This is overridden in parallel writers to communicate the hierarchy to the
   * root which then write the meta file.
   */
  int WriteMetaFileIfRequested();

  // Make a directory.
  void MakeDirectory(const char* name);

  // Remove a directory.
  void RemoveADirectory(const char* name);

  // Internal implementation details.
  svtkXMLCompositeDataWriterInternals* Internal;

  // The number of ghost levels to write for unstructured data.
  int GhostLevel;

  /**
   * Whether to write the collection file on this node. This could
   * potentially be set to 0 (i.e. do not write) for optimization
   * if the file structured does not change but the data does.
   */
  int WriteMetaFile;

  // Callback registered with the InternalProgressObserver.
  static void ProgressCallbackFunction(svtkObject*, unsigned long, void*, void*);
  // Progress callback from internal writer.
  virtual void ProgressCallback(svtkAlgorithm* w);

  // The observer to report progress from the internal writer.
  svtkCallbackCommand* InternalProgressObserver;

  /**
   * Internal method called recursively to create the xml tree for
   * the children of compositeData as well as write the actual data
   * set files.  element will only have added nested information.
   * writerIdx is the global piece index used to create unique
   * filenames for each file written.
   * This function returns 0 if no files were written from
   * compositeData.
   */
  virtual int WriteComposite(
    svtkCompositeDataSet* compositeData, svtkXMLDataElement* element, int& writerIdx) = 0;

  /**
   * Internal method to write a non svtkCompositeDataSet subclass as
   * well as add in the file name to the metadata file.
   * Element is the containing XML metadata element that may
   * have data overwritten and added to (the index XML attribute
   * should not be touched though).  writerIdx is the piece index
   * that gets incremented for the globally numbered piece.
   * This function returns 0 if no file was written (not necessarily an error).
   * this->ErrorCode is set on error.
   */
  virtual int WriteNonCompositeData(
    svtkDataObject* dObj, svtkXMLDataElement* element, int& writerIdx, const char* fileName);

  /**
   * Utility function to remove any already written files
   * in case writer failed.
   */
  virtual void RemoveWrittenFiles(const char* SubDirectory);

private:
  svtkXMLCompositeDataWriter(const svtkXMLCompositeDataWriter&) = delete;
  void operator=(const svtkXMLCompositeDataWriter&) = delete;
};

#endif
