/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridVolumeMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkUnstructuredGridVolumeMapper
 * @brief   Abstract class for an unstructured grid volume mapper
 *
 *
 * svtkUnstructuredGridVolumeMapper is the abstract definition of a volume mapper for
 * unstructured data (svtkUnstructuredGrid). Several basic types of volume mappers
 * are supported as subclasses.
 *
 * @sa
 * svtkUnstructuredGridVolumeRayCastMapper
 */

#ifndef svtkUnstructuredGridVolumeMapper_h
#define svtkUnstructuredGridVolumeMapper_h

#include "svtkAbstractVolumeMapper.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class svtkRenderer;
class svtkVolume;
class svtkUnstructuredGridBase;
class svtkWindow;

class SVTKRENDERINGVOLUME_EXPORT svtkUnstructuredGridVolumeMapper : public svtkAbstractVolumeMapper
{
public:
  svtkTypeMacro(svtkUnstructuredGridVolumeMapper, svtkAbstractVolumeMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the input data
   */
  virtual void SetInputData(svtkUnstructuredGridBase*);
  virtual void SetInputData(svtkDataSet*);
  svtkUnstructuredGridBase* GetInput();
  //@}

  svtkSetMacro(BlendMode, int);
  void SetBlendModeToComposite()
  {
    this->SetBlendMode(svtkUnstructuredGridVolumeMapper::COMPOSITE_BLEND);
  }
  void SetBlendModeToMaximumIntensity()
  {
    this->SetBlendMode(svtkUnstructuredGridVolumeMapper::MAXIMUM_INTENSITY_BLEND);
  }
  svtkGetMacro(BlendMode, int);

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Render the volume
   */
  void Render(svtkRenderer* ren, svtkVolume* vol) override = 0;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override {}

  enum
  {
    COMPOSITE_BLEND,
    MAXIMUM_INTENSITY_BLEND
  };

protected:
  svtkUnstructuredGridVolumeMapper();
  ~svtkUnstructuredGridVolumeMapper() override;

  int BlendMode;

  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkUnstructuredGridVolumeMapper(const svtkUnstructuredGridVolumeMapper&) = delete;
  void operator=(const svtkUnstructuredGridVolumeMapper&) = delete;
};

#endif
