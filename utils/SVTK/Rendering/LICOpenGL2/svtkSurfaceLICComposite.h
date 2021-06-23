/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSurfaceLICComposite.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSurfaceLICComposite
 *
 * This class decomposes the image space and shuffles image space
 * data onto the new decomposition with the necessary guard cells
 * to prevent artifacts at the decomposition boundaries. After the
 * image LIC is computed on the new decomposition this class will
 * un-shuffle the computed LIC back onto the original decomposition
 */

#ifndef svtkSurfaceLICComposite_h
#define svtkSurfaceLICComposite_h

#include "svtkObject.h"
#include "svtkPixelExtent.h"               // for pixel extent
#include "svtkRenderingLICOpenGL2Module.h" // for export macro
#include <deque>                          // for deque
#include <vector>                         // for vector

class svtkFloatArray;
class svtkOpenGLRenderWindow;
class svtkTextureObject;
class svtkPainterCommunicator;

class SVTKRENDERINGLICOPENGL2_EXPORT svtkSurfaceLICComposite : public svtkObject
{
public:
  static svtkSurfaceLICComposite* New();
  svtkTypeMacro(svtkSurfaceLICComposite, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Initialize the object based on the following description of the
   * blocks projected onto the render window. wholeExt describes the
   * window size, originalExts describe each block's extent in window
   * coords. stepSize is the window coordinate integration step size.
   * when inplace is true compositing happens on the original extent.
   */
  void Initialize(const svtkPixelExtent& winExt, const std::deque<svtkPixelExtent>& blockExts,
    int strategy, double stepSize, int nSteps, int normalizeVectors, int enhancedLIC,
    int anitalias);

  /**
   * Control the screen space decomposition. The available modes are:

   * INPLACE
   * use the block decomp. This may result in LIC being computed
   * many times for the same pixels and an excessive amount of
   * IPC during compositing if any of the block extents cover
   * or intersect a number of block extents. The input data
   * needs to be shuffled but not unshuffled since for overlapping
   * regions LIC is computed by all processes that overlap.
   * If there is very little overlap between block extents
   * then this method is superior since no unshuffle is needed.

   * INPLACE_DISJOINT
   * use a disjoint version of the block decomp. This will leave
   * non-overlapping data in place, reasigning overlapping regions
   * so that LIC is computed once for each pixel on the screen.
   * An unshuffle step to move data in overlapping region to all
   * processes that overlap.

   * BALANCED
   * move to a new decomp where each rank gets an equal number
   * of pixels. This ensures the best load balancing during LIC
   * and that LIC is computed once for each pixel. In the worst
   * case each pixel will be shuffled and unshuffled.

   * AUTO
   * Use a heuristic to select the mode.
   */
  enum
  {
    COMPOSITE_INPLACE = 0,
    COMPOSITE_INPLACE_DISJOINT,
    COMPOSITE_BALANCED,
    COMPOSITE_AUTO
  };
  void SetStrategy(int val) { this->Strategy = val; }
  int GetStrategy() { return this->Strategy; }

  /**
   * Get the number of new extents assigned to this rank after
   * the decomposition.
   */
  int GetNumberOfCompositeExtents() const { return static_cast<int>(this->CompositeExt.size()); }

  /**
   * Get the extent of the domain over which to compute the LIC. This can
   * be querried only after the Composite takes place.
   */
  const svtkPixelExtent& GetGuardExtent(int i = 0) const { return this->GuardExt[i]; }

  const std::deque<svtkPixelExtent>& GetGuardExtents() const { return this->GuardExt; }

  /**
   * Get the extent of the domain over which to compute the LIC. This can
   * be querried only after the Composite takes place.
   */
  const svtkPixelExtent& GetDisjointGuardExtent(int i = 0) const
  {
    return this->DisjointGuardExt[i];
  }

  const std::deque<svtkPixelExtent>& GetDisjointGuardExtents() const { return this->GuardExt; }

  /**
   * Get the extent of the domain over which to compute the LIC. This can
   * be querried only after the Composite takes place.
   */
  const svtkPixelExtent& GetCompositeExtent(int i = 0) const { return this->CompositeExt[i]; }

  const std::deque<svtkPixelExtent>& GetCompositeExtents() const { return this->CompositeExt; }

  /**
   * Get the whole dataset extent (all blocks).
   */
  const svtkPixelExtent& GetDataSetExtent() const { return this->DataSetExt; }

  /**
   * Get the whole window extent.
   */
  const svtkPixelExtent& GetWindowExtent() const { return this->WindowExt; }

  /**
   * Set up for a serial run, makes the decomp disjoint and adds
   * requisite guard pixles.
   */
  int InitializeCompositeExtents(float* vectors);

  /**
   * Set the rendering context. Must set prior to use. Reference is not
   * held, so caller must ensure the renderer is not destroyed during
   * use.
   */
  virtual void SetContext(svtkOpenGLRenderWindow*) {}
  virtual svtkOpenGLRenderWindow* GetContext() { return nullptr; }

  /**
   * Set the communicator for parallel communication. A duplicate
   * is not made. It is up to the caller to manage the life of
   * the communicator such that it is around while this class
   * needs it and is released after.
   */
  virtual void SetCommunicator(svtkPainterCommunicator*) {}

  /**
   * Set the communicator to the default communicator
   */
  virtual void RestoreDefaultCommunicator() {}

  /**
   * Build programs to move data to the new decomp
   * In parallel THIS IS A COLLECTIVE OPERATION
   */
  virtual int BuildProgram(float*) { return -1; }

  /**
   * Move a single buffer from the geometry decomp to the LIC decomp.
   * THIS IS A COLLECTIVE OPERATION
   */
  virtual int Gather(void*, int, int, svtkTextureObject*&) { return -1; }

  /**
   * Move a single buffer from the LIC decomp to the geometry decomp
   * In parallel THIS IS A COLLECTIVE OPERATION
   */
  virtual int Scatter(void*, int, int, svtkTextureObject*&) { return -1; }

  /**
   * Make a decomposition disjoint with respect to itself. Extents are
   * removed from the input array and disjoint extents are appended onto
   * the output array. This is a local operation.
   */
  static int MakeDecompDisjoint(std::deque<svtkPixelExtent>& in, std::deque<svtkPixelExtent>& out);

protected:
  svtkSurfaceLICComposite();
  ~svtkSurfaceLICComposite() override;

  /**
   * For serial run. Make a decomposition disjoint. Sorts extents and
   * processes largest to smallest , repeatedly subtracting smaller
   * remaining blocks from the largest remaining. Each extent in the
   * new disjoint set is shrunk to tightly bound the vector data,
   * extents with empty vectors are removed. This is a local operation
   * since vector field is local.
   */
  int MakeDecompDisjoint(
    const std::deque<svtkPixelExtent>& in, std::deque<svtkPixelExtent>& out, float* vectors);

  /**
   * Compute max(V) on the given extent.
   */
  float VectorMax(const svtkPixelExtent& ext, float* vectors);

  /**
   * Compute max(V) on a set of extents. Neighboring extents are
   * including in the computation.
   */
  int VectorMax(const std::deque<svtkPixelExtent>& exts, float* vectors, std::vector<float>& vMax);

  /**
   * Add guard pixels (Serial run)
   */
  int AddGuardPixels(const std::deque<svtkPixelExtent>& exts, std::deque<svtkPixelExtent>& guardExts,
    std::deque<svtkPixelExtent>& disjointGuardExts, float* vectors);

  /**
   * shrink pixel extent based on non-zero alpha channel values
   */
  void GetPixelBounds(float* rgba, int ni, svtkPixelExtent& ext);

  /**
   * factor for determining extra padding for guard pixels.
   * depends on window aspect ratio because of anisotropic
   * transform to texture space. see note in implementation.
   */
  float GetFudgeFactor(int nx[2]);

protected:
  int Pass; // id for mpi tagging

  svtkPixelExtent WindowExt;             // screen extent (screen size)
  svtkPixelExtent DataSetExt;            // screen extent of the dataset
  std::deque<svtkPixelExtent> BlockExts; // screen extents of blocks

  std::deque<svtkPixelExtent> CompositeExt;     // screen extents after decomp
  std::deque<svtkPixelExtent> GuardExt;         // screen extents w/ guard cells
  std::deque<svtkPixelExtent> DisjointGuardExt; // screen extents w/ guard cells

  int Strategy; // control for parallel composite

  double StepSize;           // window coordinates step size
  int NumberOfSteps;         // number of integration steps
  int NormalizeVectors;      // does integrator normailze
  int NumberOfGuardLevels;   // 1.5 if enhanced LIC 1 otherwise
  int NumberOfEEGuardPixels; // 1 if enhanced LIC 0 otherwise
  int NumberOfAAGuardPixels; // n antialias passes

private:
  svtkSurfaceLICComposite(const svtkSurfaceLICComposite&) = delete;
  void operator=(const svtkSurfaceLICComposite&) = delete;

  friend ostream& operator<<(ostream& os, svtkSurfaceLICComposite& ss);
};

ostream& operator<<(ostream& os, svtkSurfaceLICComposite& ss);

#endif
