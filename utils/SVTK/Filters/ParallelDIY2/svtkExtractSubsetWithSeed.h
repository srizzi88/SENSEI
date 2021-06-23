/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSubsetWithSeed.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkExtractSubsetWithSeed
 * @brief extract a line or plane in the ijk space starting with a seed
 *
 * svtkExtractSubsetWithSeed is a filter that can extract a line or a plane
 * in the i-j-k space starting with a seed point. The filter supports cases
 * where the structured grid is split up into multiple blocks (across multiple
 * ranks). It also handles cases were the ijk origin for each the blocks is not
 * aligned.
 *
 * The implementation starts with the seed point and then extracts a line
 * in the chosen direction. Then, using the face center for the terminal
 * faces as the new seeds it continues seeding and extracting until a seed can
 * no longer extract a new grid. The same principle holds when extracting a
 * plane, except in that case multiple seeds are generated using face centers
 * for each face alone the plane edges.
 */

#ifndef svtkExtractSubsetWithSeed_h
#define svtkExtractSubsetWithSeed_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkFiltersParallelDIY2Module.h" // for export macros

class svtkMultiProcessController;

class SVTKFILTERSPARALLELDIY2_EXPORT svtkExtractSubsetWithSeed : public svtkDataObjectAlgorithm
{
public:
  static svtkExtractSubsetWithSeed* New();
  svtkTypeMacro(svtkExtractSubsetWithSeed, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the extraction seed point. This is specified in world coordinates
   * i.e. x-y-z space.
   */
  svtkSetVector3Macro(Seed, double);
  svtkGetVector3Macro(Seed, double);
  //@}

  enum
  {
    LINE_I = 0,
    LINE_J,
    LINE_K,
    PLANE_IJ,
    PLANE_JK,
    PLANE_KI,
  };
  //@{
  /**
   * Get/Set the directions in the ijk spaced to extract starting with the
   * seed.
   */
  svtkSetClampMacro(Direction, int, LINE_I, PLANE_KI);
  svtkGetMacro(Direction, int);
  void SetDirectionToLineI() { this->SetDirection(LINE_I); }
  void SetDirectionToLineJ() { this->SetDirection(LINE_J); }
  void SetDirectionToLineK() { this->SetDirection(LINE_K); }
  void SetDirectionToPlaneIJ() { this->SetDirection(PLANE_IJ); }
  void SetDirectionToPlaneJK() { this->SetDirection(PLANE_JK); }
  void SetDirectionToPlaneKI() { this->SetDirection(PLANE_KI); }
  //@}

  //@{
  /**
   * Get/Set the controller to use. By default
   * svtkMultiProcessController::GlobalController will be used.
   */
  void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}
protected:
  svtkExtractSubsetWithSeed();
  ~svtkExtractSubsetWithSeed() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int RequestInformation(svtkInformation*, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkExtractSubsetWithSeed(const svtkExtractSubsetWithSeed&) = delete;
  void operator=(const svtkExtractSubsetWithSeed&) = delete;

  double Seed[3] = { 0, 0, 0 };
  int Direction = LINE_I;
  svtkMultiProcessController* Controller = nullptr;
};

#endif
