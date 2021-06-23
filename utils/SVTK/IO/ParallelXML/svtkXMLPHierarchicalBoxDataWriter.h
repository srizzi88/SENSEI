/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPHierarchicalBoxDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPHierarchicalBoxDataWriter
 * @brief   parallel writer for
 * svtkHierarchicalBoxDataSet for backwards compatibility.
 *
 * svtkXMLPHierarchicalBoxDataWriter is an empty subclass of
 * svtkXMLPUniformGridAMRWriter for backwards compatibility.
 */

#ifndef svtkXMLPHierarchicalBoxDataWriter_h
#define svtkXMLPHierarchicalBoxDataWriter_h

#include "svtkIOParallelXMLModule.h" // For export macro
#include "svtkXMLPUniformGridAMRWriter.h"

class SVTKIOPARALLELXML_EXPORT svtkXMLPHierarchicalBoxDataWriter : public svtkXMLPUniformGridAMRWriter
{
public:
  static svtkXMLPHierarchicalBoxDataWriter* New();
  svtkTypeMacro(svtkXMLPHierarchicalBoxDataWriter, svtkXMLPUniformGridAMRWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkXMLPHierarchicalBoxDataWriter();
  ~svtkXMLPHierarchicalBoxDataWriter() override;

private:
  svtkXMLPHierarchicalBoxDataWriter(const svtkXMLPHierarchicalBoxDataWriter&) = delete;
  void operator=(const svtkXMLPHierarchicalBoxDataWriter&) = delete;
};

#endif
