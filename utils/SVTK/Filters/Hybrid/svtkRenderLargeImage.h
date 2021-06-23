/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderLargeImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRenderLargeImage
 * @brief   Use tiling to generate a large rendering
 *
 * svtkRenderLargeImage provides methods needed to read a region from a file.
 */

#ifndef svtkRenderLargeImage_h
#define svtkRenderLargeImage_h

#include "svtkAlgorithm.h"
#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkImageData.h"           // makes things a bit easier

class svtkRenderer;
class svtkActor2DCollection;
class svtkCollection;
class svtkRenderLargeImage2DHelperClass;

class SVTKFILTERSHYBRID_EXPORT svtkRenderLargeImage : public svtkAlgorithm
{
public:
  static svtkRenderLargeImage* New();
  svtkTypeMacro(svtkRenderLargeImage, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The magnification of the current render window
   */
  svtkSetMacro(Magnification, int);
  svtkGetMacro(Magnification, int);
  //@}

  /**
   * Indicates what renderer to get the pixel data from.
   */
  virtual void SetInput(svtkRenderer*);

  //@{
  /**
   * Returns which renderer is being used as the source for the pixel data.
   */
  svtkGetObjectMacro(Input, svtkRenderer);
  //@}

  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkImageData* GetOutput();

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkRenderLargeImage();
  ~svtkRenderLargeImage() override;

  int Magnification;
  svtkRenderer* Input;
  void RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);
  void RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  // see algorithm for more info
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  // Adjust the coordinates of all 2D actors to fit new window size
  void Rescale2DActors();
  // Shift each actor according to the tile we are rendering
  void Shift2DActors(int x, int y);
  // put them all back to their previous state when finished.
  void Restore2DActors();
  // 2D Actors need to be rescaled and shifted about for each tile
  // use this helper class to make life easier.
  svtkRenderLargeImage2DHelperClass* StoredData;

private:
  svtkRenderLargeImage(const svtkRenderLargeImage&) = delete;
  void operator=(const svtkRenderLargeImage&) = delete;
};

#endif
