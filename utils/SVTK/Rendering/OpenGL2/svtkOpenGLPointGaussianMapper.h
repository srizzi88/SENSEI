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
 * @class   svtkOpenGLPointGaussianMapper
 * @brief   draw PointGaussians using imposters
 *
 * An OpenGL mapper that uses imposters to draw PointGaussians. Supports
 * transparency and picking as well.
 */

#ifndef svtkOpenGLPointGaussianMapper_h
#define svtkOpenGLPointGaussianMapper_h

#include "svtkPointGaussianMapper.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include <vector>                      // for ivar

class svtkOpenGLPointGaussianMapperHelper;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLPointGaussianMapper : public svtkPointGaussianMapper
{
public:
  static svtkOpenGLPointGaussianMapper* New();
  svtkTypeMacro(svtkOpenGLPointGaussianMapper, svtkPointGaussianMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  /**
   * Based on emissive setting
   */
  bool HasTranslucentPolygonalGeometry() override;

  /**
   * This calls RenderPiece (in a for loop if streaming is necessary).
   */
  void Render(svtkRenderer* ren, svtkActor* act) override;

  /**
   * allows a mapper to update a selections color buffers
   * Called from a prop which in turn is called from the selector
   */
  void ProcessSelectorPixelBuffers(
    svtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, svtkProp* prop) override;

protected:
  svtkOpenGLPointGaussianMapper();
  ~svtkOpenGLPointGaussianMapper() override;

  void ReportReferences(svtkGarbageCollector* collector) override;

  std::vector<svtkOpenGLPointGaussianMapperHelper*> Helpers;
  svtkOpenGLPointGaussianMapperHelper* CreateHelper();
  void CopyMapperValuesToHelper(svtkOpenGLPointGaussianMapperHelper* helper);

  svtkTimeStamp HelperUpdateTime;
  svtkTimeStamp ScaleTableUpdateTime;
  svtkTimeStamp OpacityTableUpdateTime;

  // unused
  void RenderPiece(svtkRenderer*, svtkActor*) override {}

  void RenderInternal(svtkRenderer*, svtkActor*);

  // create the table for opacity values
  void BuildOpacityTable();

  // create the table for scale values
  void BuildScaleTable();

  float* OpacityTable;  // the table
  double OpacityScale;  // used for quick lookups
  double OpacityOffset; // used for quick lookups
  float* ScaleTable;    // the table
  double ScaleScale;    // used for quick lookups
  double ScaleOffset;   // used for quick lookups

  /**
   * We need to override this method because the standard streaming
   * demand driven pipeline may not be what we need as we can handle
   * hierarchical data as input
   */
  svtkExecutive* CreateDefaultExecutive() override;

  /**
   * Need to define the type of data handled by this mapper.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * Need to loop over the hierarchy to compute bounds
   */
  void ComputeBounds() override;

  // used by the hardware selector
  std::vector<std::vector<unsigned int> > PickPixels;

private:
  svtkOpenGLPointGaussianMapper(const svtkOpenGLPointGaussianMapper&) = delete;
  void operator=(const svtkOpenGLPointGaussianMapper&) = delete;
};

#endif
