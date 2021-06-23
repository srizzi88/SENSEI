package svtk.sample.rendering.annotation;

import javax.swing.JFrame;

import svtk.svtkActor;
import svtk.svtkCubeAxesActor;
import svtk.svtkDelaunay2D;
import svtk.svtkNativeLibrary;
import svtk.svtkPanel;
import svtk.svtkPoints;
import svtk.svtkPolyData;
import svtk.svtkPolyDataMapper;
import svtk.svtkRenderer;
import svtk.svtkStringArray;

public class LabeledCubeAxesActor {
  // Load SVTK library and print which library was not properly loaded
  static {
    if(!svtkNativeLibrary.LoadAllNativeLibraries()) {
      for(svtkNativeLibrary lib : svtkNativeLibrary.values()) {
        if(!lib.IsLoaded()) {
          System.out.println(lib.GetLibraryName() + " not loaded");
        }
      }
    }
    svtkNativeLibrary.DisableOutputWindow(null);
  }

  public static void main(String args[]) {
    try {
      javax.swing.SwingUtilities.invokeLater(new Runnable() {
        @Override
        public void run() {
          createVtkPanel();
        }
      });
    } catch(Exception e) {
      e.printStackTrace();
    }
  }

  /**
   * @param panel
   */
  static void createVtkPanel() {
    JFrame mainFrame = new JFrame();
    svtkPanel panel = new svtkPanel();
    svtkRenderer renderer = panel.GetRenderer();

    svtkPoints points = getPoints();

    svtkPolyData polyData = new svtkPolyData();
    polyData.SetPoints(points);

    svtkDelaunay2D delaunay = new svtkDelaunay2D();
    delaunay.SetInputData(polyData);

    svtkPolyDataMapper mapper = new svtkPolyDataMapper();
    mapper.SetInputConnection(delaunay.GetOutputPort());

    svtkActor surfaceActor = new svtkActor();
    surfaceActor.SetMapper(mapper);
    renderer.AddActor(surfaceActor);

    svtkCubeAxesActor cubeAxesActor = new svtkCubeAxesActor();
    cubeAxesActor.SetCamera(renderer.GetActiveCamera());
    cubeAxesActor.SetBounds(points.GetBounds());
    cubeAxesActor.SetXTitle("Date");
    cubeAxesActor.SetXAxisMinorTickVisibility(0);
    cubeAxesActor.SetAxisLabels(0, getXLabels());
    cubeAxesActor.SetYTitle("Place");
    cubeAxesActor.SetYAxisMinorTickVisibility(0);
    cubeAxesActor.SetAxisLabels(1, getYLabels());
    cubeAxesActor.SetZTitle("Value");
    renderer.AddActor(cubeAxesActor);

    renderer.ResetCamera();
    mainFrame.add(panel);
    mainFrame.setSize(600, 600);
    panel.Render();
    mainFrame.setVisible(true);
    panel.Render();
  }

  /**
   * @return a list of places ordered the same as the y point values.
   */
  static svtkStringArray getYLabels() {
    svtkStringArray yLabel = new svtkStringArray();
    yLabel.InsertNextValue("A");
    yLabel.InsertNextValue("B");
    yLabel.InsertNextValue("C");
    return yLabel;
  }

  /**
   * @return a list of dates ordered the same as the x point values.
   * We have 4 X-values, from 0.5 to 3, that we will label with 6 dates
   * that will be linearly distributed along the axis.
   */
  static svtkStringArray getXLabels() {
    svtkStringArray xLabel = new svtkStringArray();
    xLabel.InsertNextValue("Jan");
    xLabel.InsertNextValue("Feb");
    xLabel.InsertNextValue("Mar");
    xLabel.InsertNextValue("Apr");
    xLabel.InsertNextValue("May");
    xLabel.InsertNextValue("June");
    return xLabel;
  }

  /**
   * @return data to plot.
   */
  static svtkPoints getPoints() {
    svtkPoints points = new svtkPoints();
    points.InsertNextPoint(0.5, 0, 0.);
    points.InsertNextPoint(1, 0, 1.);
    points.InsertNextPoint(2, 0, 0.4);
    points.InsertNextPoint(3, 0, 0.5);
    points.InsertNextPoint(0.5, 1, 0.3);
    points.InsertNextPoint(1, 1, 0.3);
    points.InsertNextPoint(2, 1, 0.8);
    points.InsertNextPoint(3, 1, 0.6);
    points.InsertNextPoint(0.5, 2, 0.5);
    points.InsertNextPoint(1, 2, 0.8);
    points.InsertNextPoint(2, 2, 0.3);
    points.InsertNextPoint(3, 2, 0.4);
    return points;
  }
}
