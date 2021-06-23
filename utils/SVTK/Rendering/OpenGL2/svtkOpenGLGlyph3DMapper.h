/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLGlyph3DMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLGlyph3DMapper
 * @brief   svtkOpenGLGlyph3D on the GPU.
 *
 * Do the same job than svtkGlyph3D but on the GPU. For this reason, it is
 * a mapper not a svtkPolyDataAlgorithm. Also, some methods of svtkOpenGLGlyph3D
 * don't make sense in svtkOpenGLGlyph3DMapper: GeneratePointIds, old-style
 * SetSource, PointIdsName, IsPointVisible.
 *
 * @sa
 * svtkOpenGLGlyph3D
 */

#ifndef svtkOpenGLGlyph3DMapper_h
#define svtkOpenGLGlyph3DMapper_h

#include "svtkGlyph3DMapper.h"
#include "svtkNew.h"                    // For svtkNew
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLGlyph3DHelper;
class svtkBitArray;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLGlyph3DMapper : public svtkGlyph3DMapper
{
public:
  static svtkOpenGLGlyph3DMapper* New();
  svtkTypeMacro(svtkOpenGLGlyph3DMapper, svtkGlyph3DMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Method initiates the mapping process. Generally sent by the actor
   * as each frame is rendered.
   */
  void Render(svtkRenderer* ren, svtkActor* a) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow* window) override;

  //@{
  /**
   * Get the maximum number of LOD. OpenGL context must be bound.
   * The maximum number of LOD depends on GPU capabilities.
   */
  svtkIdType GetMaxNumberOfLOD() override;

  /**
   * Set the number of LOD.
   */
  void SetNumberOfLOD(svtkIdType nb) override;

  /**
   * Configure LODs. Culling must be enabled.
   * distance have to be a positive value, it is the distance to the camera scaled by
   * the instanced geometry bounding box.
   * targetReduction have to be between 0 and 1, 0 disable decimation, 1 draw a point.
   *
   * @sa svtkDecimatePro::SetTargetReduction
   */
  virtual void SetLODDistanceAndTargetReduction(
    svtkIdType index, float distance, float targetReduction) override;
  //@}

protected:
  svtkOpenGLGlyph3DMapper();
  ~svtkOpenGLGlyph3DMapper() override;

  /**
   * Render setup
   */
  virtual void Render(svtkRenderer*, svtkActor*, svtkDataSet*);

  /**
   * Send mapper ivars to sub-mapper.
   * \pre mapper_exists: mapper != 0
   */
  void CopyInformationToSubMapper(svtkOpenGLGlyph3DHelper*);

  void SetupColorMapper();

  svtkMapper* ColorMapper;

  class svtkOpenGLGlyph3DMapperEntry;
  class svtkOpenGLGlyph3DMapperSubArray;
  class svtkOpenGLGlyph3DMapperArray;
  svtkOpenGLGlyph3DMapperArray* GlyphValues; // array of value for datasets

  /**
   * Build data structures associated with
   */
  virtual void RebuildStructures(svtkOpenGLGlyph3DMapperSubArray* entry, svtkIdType numPts,
    svtkActor* actor, svtkDataSet* dataset, svtkBitArray* maskArray);

  svtkMTimeType BlockMTime; // Last time BlockAttributes was modified.

private:
  svtkOpenGLGlyph3DMapper(const svtkOpenGLGlyph3DMapper&) = delete;
  void operator=(const svtkOpenGLGlyph3DMapper&) = delete;
};

#endif
