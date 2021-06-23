/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPUniformGridAMRWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPUniformGridAMRWriter
 * @brief   parallel writer for
 * svtkUniformGridAMR and subclasses.
 *
 * svtkXMLPCompositeDataWriter writes (in parallel or serially) svtkUniformGridAMR
 * and subclasses. When running in parallel all processes are expected to have
 * the same meta-data (i.e. amr-boxes, structure, etc.) however they may now
 * have the missing data-blocks. This class extends
 * svtkXMLUniformGridAMRWriter to communicate information about data blocks
 * to the root node so that the root node can write the XML file describing the
 * structure correctly.
 */

#ifndef svtkXMLPUniformGridAMRWriter_h
#define svtkXMLPUniformGridAMRWriter_h

#include "svtkIOParallelXMLModule.h" // For export macro
#include "svtkXMLUniformGridAMRWriter.h"

class svtkMultiProcessController;

class SVTKIOPARALLELXML_EXPORT svtkXMLPUniformGridAMRWriter : public svtkXMLUniformGridAMRWriter
{
public:
  static svtkXMLPUniformGridAMRWriter* New();
  svtkTypeMacro(svtkXMLPUniformGridAMRWriter, svtkXMLUniformGridAMRWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

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

protected:
  svtkXMLPUniformGridAMRWriter();
  ~svtkXMLPUniformGridAMRWriter() override;

  /**
   * Overridden to reduce information about data-types across all processes.
   */
  void FillDataTypes(svtkCompositeDataSet*) override;

  svtkMultiProcessController* Controller;

private:
  svtkXMLPUniformGridAMRWriter(const svtkXMLPUniformGridAMRWriter&) = delete;
  void operator=(const svtkXMLPUniformGridAMRWriter&) = delete;
};

#endif
