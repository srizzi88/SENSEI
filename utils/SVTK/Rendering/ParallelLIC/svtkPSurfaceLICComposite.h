/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPSurfaceLICComposite.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPSurfaceLICComposite
 *
 * This class decomposes the image space and shuffles image space
 * data onto the new decomposition with the necessary guard cells
 * to prevent artifacts at the decomposition boundaries. After the
 * image LIC is computed on the new decomposition this class will
 * un-shuffle the computed LIC back onto the original decomposition.
 */

#ifndef svtkPSurfaceLICComposite_h
#define svtkPSurfaceLICComposite_h

#include "svtkOpenGLRenderWindow.h"         // for context
#include "svtkPPixelTransfer.h"             // for pixel transfer
#include "svtkPixelExtent.h"                // for pixel extent
#include "svtkRenderingParallelLICModule.h" // for export macro
#include "svtkSurfaceLICComposite.h"
#include "svtkWeakPointer.h" // for ren context
#include <deque>            // for deque
#include <list>             // for list
#include <vector>           // for vector

class svtkFloatArray;
class svtkRenderWindow;
class svtkTextureObject;
class svtkPainterCommunicator;
class svtkPPainterCommunicator;
class svtkPPixelExtentOps;

class svtkOpenGLHelper;
class svtkOpenGLFramebufferObject;

class SVTKRENDERINGPARALLELLIC_EXPORT svtkPSurfaceLICComposite : public svtkSurfaceLICComposite
{
public:
  static svtkPSurfaceLICComposite* New();
  svtkTypeMacro(svtkPSurfaceLICComposite, svtkSurfaceLICComposite);
  virtual void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set the rendering context. Must set prior to use. Reference is not
   * held, so caller must ensure the renderer is not destroyed during
   * use.
   */
  virtual void SetContext(svtkOpenGLRenderWindow* rwin) override;
  virtual svtkOpenGLRenderWindow* GetContext() override { return this->Context; }

  /**
   * Set the communicator for parallel communication. The Default is
   * COMM_NULL.
   */
  virtual void SetCommunicator(svtkPainterCommunicator* comm) override;

  /**
   * Build programs to move data to the new decomp
   * THIS IS A COLLECTIVE OPERATION
   */
  virtual int BuildProgram(float* vectors) override;

  /**
   * Move a single buffer from the geometry decomp to the LIC decomp.
   * THIS IS A COLLECTIVE OPERATION
   */
  virtual int Gather(
    void* pSendPBO, int dataType, int nComps, svtkTextureObject*& newImage) override;

  /**
   * Move a single buffer from the LIC decomp to the geometry decomp
   * THIS IS A COLLECTIVE OPERATION
   */
  virtual int Scatter(
    void* pSendPBO, int dataType, int nComps, svtkTextureObject*& newImage) override;

protected:
  svtkPSurfaceLICComposite();
  ~svtkPSurfaceLICComposite() override;

private:
  /**
   * Load, compile, and link the shader.
   */
  int InitializeCompositeShader(svtkOpenGLRenderWindow* context);

  /**
   * Composite incoming data.
   */
  int ExecuteShader(const svtkPixelExtent& ext, svtkTextureObject* tex);

  /**
   * The communication cost to move from one decomposition to another
   * is given by the ratio of pixels to send off rank to the total
   * number of source pixels.
   */
  double EstimateCommunicationCost(const std::deque<std::deque<svtkPixelExtent> >& srcExts,
    const std::deque<std::deque<svtkPixelExtent> >& destExts);

  /**
   * The efficiency of a decomposition is the ratio of useful pixels
   * to guard pixels. If this factor shrinks below 1 there may be
   * an issue.
   */
  double EstimateDecompEfficiency(const std::deque<std::deque<svtkPixelExtent> >& exts,
    const std::deque<std::deque<svtkPixelExtent> >& guardExts);

  /**
   * Given a window extent, decompose into the requested number of
   * pieces.
   */
  int DecomposeScreenExtent(std::deque<std::deque<svtkPixelExtent> >& newExts, float* vectors);

  /**
   * Given an extent, decompose into the requested number of
   * pieces.
   */
  int DecomposeExtent(svtkPixelExtent& in, int nPieces, std::list<svtkPixelExtent>& out);

  /**
   * For parallel run. Make a decomposition disjoint. Sorts extents
   * and processes largest to smallest , repeatedly subtracting smaller
   * remaining blocks from the largest remaining.  Each extent in the
   * new disjoint set is shrunk to tightly bound the vector data,
   * extents with empty vectors are removed. This is a global operation
   * as the vector field is distributed and has not been composited yet.
   */
  int MakeDecompDisjoint(const std::deque<std::deque<svtkPixelExtent> >& in,
    std::deque<std::deque<svtkPixelExtent> >& out, float* vectors);

  // decomp set of extents
  int MakeDecompLocallyDisjoint(const std::deque<std::deque<svtkPixelExtent> >& in,
    std::deque<std::deque<svtkPixelExtent> >& out);

  using svtkSurfaceLICComposite::MakeDecompDisjoint;

  /**
   * All gather geometry domain decomposition. The extent of local
   * blocks are passed in, the collection of all blocks is returned
   * along with the dataset extent.
   */
  int AllGatherExtents(const std::deque<svtkPixelExtent>& localExts,
    std::deque<std::deque<svtkPixelExtent> >& remoteExts, svtkPixelExtent& dataSetExt);

  /**
   * All reduce max(|V|) on the new decomposition.
   */
  int AllReduceVectorMax(const std::deque<svtkPixelExtent>& originalExts,
    const std::deque<std::deque<svtkPixelExtent> >& newExts, float* vectors,
    std::vector<std::vector<float> >& vectorMax);

  /**
   * Add guard pixels (Parallel run)
   */
  int AddGuardPixels(const std::deque<std::deque<svtkPixelExtent> >& exts,
    std::deque<std::deque<svtkPixelExtent> >& guardExts,
    std::deque<std::deque<svtkPixelExtent> >& disjointGuardExts, float* vectors);

private:
  svtkPPainterCommunicator* PainterComm; // mpi state
  svtkPPixelExtentOps* PixelOps;
  int CommRank;
  int CommSize;

  svtkWeakPointer<svtkOpenGLRenderWindow> Context; // rendering context

  svtkOpenGLFramebufferObject* FBO; // Framebuffer object
  svtkOpenGLHelper* CompositeShader;

  std::deque<svtkPPixelTransfer> GatherProgram; // ordered steps required to move data to new decomp
  std::deque<svtkPPixelTransfer>
    ScatterProgram; // ordered steps required to unmove data from new decomp

  friend SVTKRENDERINGPARALLELLIC_EXPORT ostream& operator<<(
    ostream& os, svtkPSurfaceLICComposite& ss);

  svtkPSurfaceLICComposite(const svtkPSurfaceLICComposite&) = delete;
  void operator=(const svtkPSurfaceLICComposite&) = delete;
};

SVTKRENDERINGPARALLELLIC_EXPORT
ostream& operator<<(ostream& os, svtkPSurfaceLICComposite& ss);

#endif
