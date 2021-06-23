/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositeDataWriter
 * @brief   legacy SVTK file writer for svtkCompositeDataSet
 * subclasses.
 *
 * svtkCompositeDataWriter is a writer for writing legacy SVTK files for
 * svtkCompositeDataSet and subclasses.
 * @warning
 * This is an experimental format. Use XML-based formats for writing composite
 * datasets. Saving composite dataset in legacy SVTK format is expected to change
 * in future including changes to the file layout.
 */

#ifndef svtkCompositeDataWriter_h
#define svtkCompositeDataWriter_h

#include "svtkDataWriter.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkCompositeDataSet;
class svtkHierarchicalBoxDataSet;
class svtkMultiBlockDataSet;
class svtkMultiPieceDataSet;
class svtkNonOverlappingAMR;
class svtkOverlappingAMR;
class svtkPartitionedDataSet;
class svtkPartitionedDataSetCollection;

class SVTKIOLEGACY_EXPORT svtkCompositeDataWriter : public svtkDataWriter
{
public:
  static svtkCompositeDataWriter* New();
  svtkTypeMacro(svtkCompositeDataWriter, svtkDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkCompositeDataSet* GetInput();
  svtkCompositeDataSet* GetInput(int port);
  //@}

protected:
  svtkCompositeDataWriter();
  ~svtkCompositeDataWriter() override;

  //@{
  /**
   * Performs the actual writing.
   */
  void WriteData() override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  //@}

  bool WriteCompositeData(ostream*, svtkMultiBlockDataSet*);
  bool WriteCompositeData(ostream*, svtkMultiPieceDataSet*);
  bool WriteCompositeData(ostream*, svtkHierarchicalBoxDataSet*);
  bool WriteCompositeData(ostream*, svtkOverlappingAMR*);
  bool WriteCompositeData(ostream*, svtkNonOverlappingAMR*);
  bool WriteCompositeData(ostream*, svtkPartitionedDataSet*);
  bool WriteCompositeData(ostream*, svtkPartitionedDataSetCollection*);
  bool WriteBlock(ostream* fp, svtkDataObject* block);

private:
  svtkCompositeDataWriter(const svtkCompositeDataWriter&) = delete;
  void operator=(const svtkCompositeDataWriter&) = delete;
};

#endif
