#ifndef QtSVTKRenderWindows_H
#define QtSVTKRenderWindows_H

#include "svtkDistanceWidget.h"
#include "svtkImagePlaneWidget.h"
#include "svtkResliceImageViewer.h"
#include "svtkResliceImageViewerMeasurements.h"
#include "svtkSmartPointer.h"
#include <QMainWindow>

// Forward Qt class declarations
class Ui_QtSVTKRenderWindows;

class QtSVTKRenderWindows : public QMainWindow
{
  Q_OBJECT
public:
  // Constructor/Destructor
  QtSVTKRenderWindows(int argc, char* argv[]);
  ~QtSVTKRenderWindows() override {}

public slots:

  virtual void slotExit();
  virtual void resliceMode(int);
  virtual void thickMode(int);
  virtual void SetBlendModeToMaxIP();
  virtual void SetBlendModeToMinIP();
  virtual void SetBlendModeToMeanIP();
  virtual void SetBlendMode(int);
  virtual void ResetViews();
  virtual void Render();
  virtual void AddDistanceMeasurementToView1();
  virtual void AddDistanceMeasurementToView(int);

protected:
  svtkSmartPointer<svtkResliceImageViewer> riw[3];
  svtkSmartPointer<svtkImagePlaneWidget> planeWidget[3];
  svtkSmartPointer<svtkDistanceWidget> DistanceWidget[3];
  svtkSmartPointer<svtkResliceImageViewerMeasurements> ResliceMeasurements;

protected slots:

private:
  // Designer form
  Ui_QtSVTKRenderWindows* ui;
};

#endif // QtSVTKRenderWindows_H
