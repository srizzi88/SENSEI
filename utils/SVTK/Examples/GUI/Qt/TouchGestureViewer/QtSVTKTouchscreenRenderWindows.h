#ifndef QtSVTKTouchscreenRenderWindows_H
#define QtSVTKTouchscreenRenderWindows_H

#include "svtkDistanceWidget.h"
#include "svtkImagePlaneWidget.h"
#include "svtkResliceImageViewer.h"
#include "svtkResliceImageViewerMeasurements.h"
#include "svtkSmartPointer.h"
#include <QMainWindow>

// Forward Qt class declarations
class Ui_QtSVTKTouchscreenRenderWindows;

class QtSVTKTouchscreenRenderWindows : public QMainWindow
{
  Q_OBJECT
public:
  // Constructor/Destructor
  QtSVTKTouchscreenRenderWindows(int argc, char* argv[]);
  ~QtSVTKTouchscreenRenderWindows() override {}

private:
  // Designer form
  Ui_QtSVTKTouchscreenRenderWindows* ui;
};

#endif // QtSVTKTouchscreenRenderWindows_H
