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
 * @class   svtkOpenVROverlay
 * @brief   OpenVR overlay
 *
 * svtkOpenVROverlay support for VR overlays
 */

#ifndef svtkOpenVROverlay_h
#define svtkOpenVROverlay_h

#include "svtkNew.h" // for ivars
#include "svtkObject.h"
#include "svtkRenderingOpenVRModule.h" // For export macro
#include "svtkWeakPointer.h"           // for ivars
#include <map>                        // ivars
#include <openvr.h>                   // for ivars
#include <vector>                     // ivars

class svtkJPEGReader;
class svtkOpenVROverlaySpot;
class svtkOpenVRRenderWindow;
class svtkTextureObject;
class svtkOpenVRCameraPose;
class svtkOpenVRCamera;
class svtkXMLDataElement;

class SVTKRENDERINGOPENVR_EXPORT svtkOpenVROverlay : public svtkObject
{
public:
  static svtkOpenVROverlay* New();
  svtkTypeMacro(svtkOpenVROverlay, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Render the overlay
   */
  virtual void Render();

  /**
   * Create the overlay
   */
  virtual void Create(svtkOpenVRRenderWindow* rw);

  /**
   * Get handle to the overlay
   */
  vr::VROverlayHandle_t GetOverlayHandle() { return this->OverlayHandle; }

  /**
   * Get handle to the overlay texture
   */
  svtkTextureObject* GetOverlayTexture() { return this->OverlayTexture.Get(); }

  //@{
  /**
   * methods to support events on the overlay
   */
  virtual void MouseMoved(int x, int y);
  virtual void MouseButtonPress(int x, int y);
  virtual void MouseButtonRelease(int x, int y);
  //@}

  svtkOpenVROverlaySpot* GetLastSpot() { return this->LastSpot; }

  std::vector<svtkOpenVROverlaySpot>& GetSpots() { return this->Spots; }

  /***
   * update the texture because this spot has changed
   */
  virtual void UpdateSpot(svtkOpenVROverlaySpot* spot);

  //@{
  /**
   * Set/Get a prefix for saving camera poses
   */
  void SetSessionName(const std::string& name) { this->SessionName = name; }
  std::string GetSessionName() { return this->SessionName; }
  //@}

  //@{
  /**
   * Set/Get a file for the dashboard image
   */
  void SetDashboardImageFileName(const std::string& name) { this->DashboardImageFileName = name; }
  std::string GetDashboardImageFileName() { return this->DashboardImageFileName; }
  //@}

  svtkOpenVRCameraPose* GetSavedCameraPose(int i);
  virtual void SetSavedCameraPose(int i, svtkOpenVRCameraPose*);
  virtual void WriteCameraPoses(ostream& os);
  virtual void WriteCameraPoses();
  virtual void ReadCameraPoses();
  virtual void ReadCameraPoses(istream& is);
  virtual void ReadCameraPoses(svtkXMLDataElement* xml);
  virtual void SaveCameraPose(int num);
  virtual void LoadCameraPose(int num);
  virtual void LoadNextCameraPose();
  virtual std::map<int, svtkOpenVRCameraPose>& GetSavedCameraPoses()
  {
    return this->SavedCameraPoses;
  }

  // not used for dashboard overlays
  void Show();
  void Hide();

protected:
  svtkOpenVROverlay();
  ~svtkOpenVROverlay() override;

  virtual void SetupSpots() {}

  vr::IVRSystem* VRSystem;

  // for the overlay
  vr::VROverlayHandle_t OverlayHandle;
  vr::VROverlayHandle_t OverlayThumbnailHandle;
  svtkNew<svtkTextureObject> OverlayTexture;

  virtual void SetDashboardImageData(svtkJPEGReader* rdr);

  // std::vector<svtkOpenVRActiveSpot> ActiveSpots;
  unsigned char* OriginalTextureData;
  unsigned char* CurrentTextureData;

  std::vector<svtkOpenVROverlaySpot> Spots;
  svtkOpenVROverlaySpot* LastSpot;

  std::string SessionName;
  std::string DashboardImageFileName;
  std::map<int, svtkOpenVRCameraPose> SavedCameraPoses;

  svtkWeakPointer<svtkOpenVRRenderWindow> Window;
  int LastCameraPoseIndex;

  double LastSpotIntensity;
  double ActiveSpotIntensity;

private:
  svtkOpenVROverlay(const svtkOpenVROverlay&) = delete;
  void operator=(const svtkOpenVROverlay&) = delete;
};

#endif
