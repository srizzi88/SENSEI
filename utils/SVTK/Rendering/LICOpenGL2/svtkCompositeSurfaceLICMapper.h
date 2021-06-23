/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeSurfaceLICMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositeSurfaceLICMapper
 * @brief   mapper for composite dataset
 *
 * svtkCompositeSurfaceLICMapper is similar to
 * svtkGenericCompositeSurfaceLICMapper but requires that its inputs all have the
 * same properties (normals, tcoord, scalars, etc) It will only draw
 * polys and it does not support edge flags. The advantage to using
 * this class is that it generally should be faster
 */

#ifndef svtkCompositeSurfaceLICMapper_h
#define svtkCompositeSurfaceLICMapper_h

#include "svtkCompositePolyDataMapper2.h"
#include "svtkNew.h"                       // for ivars
#include "svtkRenderingLICOpenGL2Module.h" // For export macro

class svtkSurfaceLICInterface;

class SVTKRENDERINGLICOPENGL2_EXPORT svtkCompositeSurfaceLICMapper
  : public svtkCompositePolyDataMapper2
{
public:
  static svtkCompositeSurfaceLICMapper* New();
  svtkTypeMacro(svtkCompositeSurfaceLICMapper, svtkCompositePolyDataMapper2);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the svtkSurfaceLICInterface used by this mapper
   */
  svtkSurfaceLICInterface* GetLICInterface() { return this->LICInterface.Get(); }
  //@}

  /**
   * Lots of LIC setup code
   */
  void Render(svtkRenderer* ren, svtkActor* act) override;

protected:
  svtkCompositeSurfaceLICMapper();
  ~svtkCompositeSurfaceLICMapper() override;

  svtkNew<svtkSurfaceLICInterface> LICInterface;

  svtkCompositeMapperHelper2* CreateHelper() override;

  // copy values to the helpers
  void CopyMapperValuesToHelper(svtkCompositeMapperHelper2* helper) override;

private:
  svtkCompositeSurfaceLICMapper(const svtkCompositeSurfaceLICMapper&) = delete;
  void operator=(const svtkCompositeSurfaceLICMapper&) = delete;
};

#endif
