/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageRectilinearWipe.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageRectilinearWipe
 * @brief   make a rectilinear combination of two images.
 *
 * svtkImageRectilinearWipe makes a rectilinear combination of two
 * images. The two input images must correspond in size, scalar type and
 * number of components.
 * The resulting image has four possible configurations
 * called:
 *   Quad - alternate input 0 and input 1 horizontally and
 *     vertically. Select this with SetWipeModeToQuad. The Position
 *     specifies the location of the quad intersection.
 *   Corner - 3 of one input and 1 of the other. Select the location of
 *     input 0 with with SetWipeModeToLowerLeft, SetWipeModeToLowerRight,
 *     SetWipeModeToUpperLeft and SetWipeModeToUpperRight. The Position
 *     selects the location of the corner.
 *   Horizontal - alternate input 0 and input 1 with a vertical
 *     split. Select this with SetWipeModeToHorizontal. Position[0]
 *     specifies the location of the vertical transition between input 0
 *     and input 1.
 *   Vertical - alternate input 0 and input 1 with a horizontal
 *     split. Only the y The intersection point of the rectilinear points
 *     is controlled with the Point ivar.
 *
 * @par Thanks:
 * This work was supported by PHS Research Grant No. 1 P41 RR13218-01
 * from the National Center for Research Resources.
 *
 * @sa
 * svtkImageCheckerboard
 */

#ifndef svtkImageRectilinearWipe_h
#define svtkImageRectilinearWipe_h

#include "svtkImagingHybridModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

#define SVTK_WIPE_QUAD 0
#define SVTK_WIPE_HORIZONTAL 1
#define SVTK_WIPE_VERTICAL 2
#define SVTK_WIPE_LOWER_LEFT 3
#define SVTK_WIPE_LOWER_RIGHT 4
#define SVTK_WIPE_UPPER_LEFT 5
#define SVTK_WIPE_UPPER_RIGHT 6

class SVTKIMAGINGHYBRID_EXPORT svtkImageRectilinearWipe : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageRectilinearWipe* New();
  svtkTypeMacro(svtkImageRectilinearWipe, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the location of the image transition. Note that position is
   * specified in pixels.
   */
  svtkSetVector2Macro(Position, int);
  svtkGetVectorMacro(Position, int, 2);
  //@}

  //@{
  /**
   * Set/Get the location of the wipe axes. The default is X,Y (ie vector
   * values of 0 and 1).
   */
  svtkSetVector2Macro(Axis, int);
  svtkGetVectorMacro(Axis, int, 2);
  //@}

  /**
   * Set the two inputs to this filter.
   */
  virtual void SetInput1Data(svtkDataObject* in) { this->SetInputData(0, in); }
  virtual void SetInput2Data(svtkDataObject* in) { this->SetInputData(1, in); }

  //@{
  /**
   * Specify the wipe mode. This mode determnis how input 0 and input
   * 1 are combined to produce the output. Each mode uses one or both
   * of the values stored in Position.
   * SetWipeToQuad - alternate input 0 and input 1 horizontally and
   * vertically. The Position specifies the location of the quad
   * intersection.
   * SetWipeToLowerLeft{LowerRight,UpperLeft.UpperRight} - 3 of one
   * input and 1 of the other. Select the location of input 0 to the
   * LowerLeft{LowerRight,UpperLeft,UpperRight}. Position
   * selects the location of the corner.
   * SetWipeToHorizontal - alternate input 0 and input 1 with a vertical
   * split. Position[0] specifies the location of the vertical
   * transition between input 0 and input 1.
   * SetWipeToVertical - alternate input 0 and input 1 with a
   * horizontal split. Position[1] specifies the location of the
   * horizontal transition between input 0 and input 1.
   */
  svtkSetClampMacro(Wipe, int, SVTK_WIPE_QUAD, SVTK_WIPE_UPPER_RIGHT);
  svtkGetMacro(Wipe, int);
  void SetWipeToQuad() { this->SetWipe(SVTK_WIPE_QUAD); }
  void SetWipeToHorizontal() { this->SetWipe(SVTK_WIPE_HORIZONTAL); }
  void SetWipeToVertical() { this->SetWipe(SVTK_WIPE_VERTICAL); }
  void SetWipeToLowerLeft() { this->SetWipe(SVTK_WIPE_LOWER_LEFT); }
  void SetWipeToLowerRight() { this->SetWipe(SVTK_WIPE_LOWER_RIGHT); }
  void SetWipeToUpperLeft() { this->SetWipe(SVTK_WIPE_UPPER_LEFT); }
  void SetWipeToUpperRight() { this->SetWipe(SVTK_WIPE_UPPER_RIGHT); }
  //@}

protected:
  svtkImageRectilinearWipe();
  ~svtkImageRectilinearWipe() override {}

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int threadId) override;

  int Position[2];
  int Wipe;
  int Axis[2];

private:
  svtkImageRectilinearWipe(const svtkImageRectilinearWipe&) = delete;
  void operator=(const svtkImageRectilinearWipe&) = delete;
};

#endif
