/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLHierarchicalBoxDataReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLHierarchicalBoxDataReader
 * @brief   Reader for hierarchical datasets
 * (for backwards compatibility).
 *
 *
 * svtkXMLHierarchicalBoxDataReader is an empty subclass of
 * svtkXMLUniformGridAMRReader. This is only for backwards compatibility. Newer
 * code should simply use svtkXMLUniformGridAMRReader.
 *
 * @warning
 * The reader supports reading v1.1 and above. For older versions, use
 * svtkXMLHierarchicalBoxDataFileConverter.
 */

#ifndef svtkXMLHierarchicalBoxDataReader_h
#define svtkXMLHierarchicalBoxDataReader_h

#include "svtkXMLUniformGridAMRReader.h"

class SVTKIOXML_EXPORT svtkXMLHierarchicalBoxDataReader : public svtkXMLUniformGridAMRReader
{
public:
  static svtkXMLHierarchicalBoxDataReader* New();
  svtkTypeMacro(svtkXMLHierarchicalBoxDataReader, svtkXMLUniformGridAMRReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkXMLHierarchicalBoxDataReader();
  ~svtkXMLHierarchicalBoxDataReader() override;

private:
  svtkXMLHierarchicalBoxDataReader(const svtkXMLHierarchicalBoxDataReader&) = delete;
  void operator=(const svtkXMLHierarchicalBoxDataReader&) = delete;
};

#endif
