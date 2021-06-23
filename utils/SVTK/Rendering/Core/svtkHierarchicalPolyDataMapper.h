/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHierarchicalPolyDataMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHierarchicalPolyDataMapper
 * @brief   a class that renders hierarchical polygonal data
 *
 * Legacy class. Use svtkCompositePolyDataMapper instead.
 *
 * @sa
 * svtkPolyDataMapper
 */

#ifndef svtkHierarchicalPolyDataMapper_h
#define svtkHierarchicalPolyDataMapper_h

#include "svtkCompositePolyDataMapper.h"
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkHierarchicalPolyDataMapper : public svtkCompositePolyDataMapper
{

public:
  static svtkHierarchicalPolyDataMapper* New();
  svtkTypeMacro(svtkHierarchicalPolyDataMapper, svtkCompositePolyDataMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkHierarchicalPolyDataMapper();
  ~svtkHierarchicalPolyDataMapper() override;

private:
  svtkHierarchicalPolyDataMapper(const svtkHierarchicalPolyDataMapper&) = delete;
  void operator=(const svtkHierarchicalPolyDataMapper&) = delete;
};

#endif
