/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLMoleculeMapper
 * @brief   An accelerated class for rendering molecules
 *
 * A svtkMoleculeMapper that uses imposters to do the rendering. It uses
 * svtkOpenGLSphereMapper and svtkOpenGLStickMapper to do the rendering.
 */

#ifndef svtkOpenGLMoleculeMapper_h
#define svtkOpenGLMoleculeMapper_h

#include "svtkDomainsChemistryOpenGL2Module.h" // For export macro
#include "svtkMoleculeMapper.h"
#include "svtkNew.h" // For svtkNew

class svtkOpenGLSphereMapper;
class svtkOpenGLStickMapper;

class SVTKDOMAINSCHEMISTRYOPENGL2_EXPORT svtkOpenGLMoleculeMapper : public svtkMoleculeMapper
{
public:
  static svtkOpenGLMoleculeMapper* New();
  svtkTypeMacro(svtkOpenGLMoleculeMapper, svtkMoleculeMapper);

  //@{
  /**
   * Reimplemented from base class
   */
  void Render(svtkRenderer*, svtkActor*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  //@}

  /**
   * provide access to the underlying mappers
   */
  svtkOpenGLSphereMapper* GetFastAtomMapper() { return this->FastAtomMapper; }
  /**
   * allows a mapper to update a selections color buffers
   * Called from a prop which in turn is called from the selector
   */
  void ProcessSelectorPixelBuffers(
    svtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, svtkProp* prop) override;

  /**
   * Helper method to set ScalarMode on both FastAtomMapper and FastBondMapper.
   * true means SVTK_COLOR_MODE_MAP_SCALARS, false SVTK_COLOR_MODE_DIRECT_SCALARS.
   */
  void SetMapScalars(bool map) override;

protected:
  svtkOpenGLMoleculeMapper();
  ~svtkOpenGLMoleculeMapper() override;

  void UpdateAtomGlyphPolyData() override;
  void UpdateBondGlyphPolyData() override;

  //@{
  /**
   * Internal mappers
   */
  svtkNew<svtkOpenGLSphereMapper> FastAtomMapper;
  svtkNew<svtkOpenGLStickMapper> FastBondMapper;
  //@}

private:
  svtkOpenGLMoleculeMapper(const svtkOpenGLMoleculeMapper&) = delete;
  void operator=(const svtkOpenGLMoleculeMapper&) = delete;
};

#endif
