/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAvatar.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkAvatar
 * @brief Renders head and hands for a user in VR
 *
 * Set position and orientation for the head and two hands,
 * shows an observer where the avatar is looking and pointing.
 */

#ifndef svtkAvatar_h
#define svtkAvatar_h

#include "svtkActor.h"
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkAvatar : public svtkActor
{
public:
  static svtkAvatar* New();
  svtkTypeMacro(svtkAvatar, svtkActor);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set/Get the head and hand transforms.
   */
  svtkGetVector3Macro(HeadPosition, double);
  svtkSetVector3Macro(HeadPosition, double);
  svtkGetVector3Macro(HeadOrientation, double);
  svtkSetVector3Macro(HeadOrientation, double);

  svtkGetVector3Macro(LeftHandPosition, double);
  svtkSetVector3Macro(LeftHandPosition, double);
  svtkGetVector3Macro(LeftHandOrientation, double);
  svtkSetVector3Macro(LeftHandOrientation, double);

  svtkGetVector3Macro(RightHandPosition, double);
  svtkSetVector3Macro(RightHandPosition, double);
  svtkGetVector3Macro(RightHandOrientation, double);
  svtkSetVector3Macro(RightHandOrientation, double);

  /**
   * Up vector, in world coords. Must be normalized.
   */
  svtkGetVector3Macro(UpVector, double);
  svtkSetVector3Macro(UpVector, double);

  //@{
  /**
   * Normally, hand position/orientation is set explicitly.
   * If set to false, hand and arm will follow the torso
   * in a neutral position.
   */
  svtkSetMacro(UseLeftHand, bool);
  svtkGetMacro(UseLeftHand, bool);
  svtkBooleanMacro(UseLeftHand, bool);
  svtkSetMacro(UseRightHand, bool);
  svtkGetMacro(UseRightHand, bool);
  svtkBooleanMacro(UseRightHand, bool);
  //@}

  //@{
  /**
   * Show just the hands. Default false.
   */
  svtkSetMacro(ShowHandsOnly, bool);
  svtkGetMacro(ShowHandsOnly, bool);
  svtkBooleanMacro(ShowHandsOnly, bool);
  //@}

protected:
  svtkAvatar();
  ~svtkAvatar() override;

  double HeadPosition[3];
  double HeadOrientation[3];
  double LeftHandPosition[3];
  double LeftHandOrientation[3];
  double RightHandPosition[3];
  double RightHandOrientation[3];
  enum
  {
    TORSO,
    LEFT_FORE,
    RIGHT_FORE,
    LEFT_UPPER,
    RIGHT_UPPER,
    NUM_BODY,
  };
  double BodyPosition[NUM_BODY][3];
  double BodyOrientation[NUM_BODY][3];

  double UpVector[3];

  bool UseLeftHand;
  bool UseRightHand;
  bool ShowHandsOnly;

private:
  svtkAvatar(const svtkAvatar&) = delete;
  void operator=(const svtkAvatar&) = delete;
};

#endif // svtkAvatar_h
