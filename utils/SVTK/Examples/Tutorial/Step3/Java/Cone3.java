//
// This example demonstrates how to use multiple renderers within a
// render window. It is a variation of the Cone.py example. Please
// refer to that example for additional documentation.
//

import java.lang.Thread;

// we import the svtk wrapped classes first
import svtk.*;

// then we define our class
public class Cone3 {
  // in the static constructor we load in the native code
  // The libraries must be in your path to work
  static {
    System.loadLibrary("svtkCommonJava");
    System.loadLibrary("svtkFilteringJava");
    System.loadLibrary("svtkIOJava");
    System.loadLibrary("svtkImagingJava");
    System.loadLibrary("svtkGraphicsJava");
    System.loadLibrary("svtkRenderingJava");
  }

  // now the main program
  public static void main (String []args) throws Exception {
    //
    // Next we create an instance of svtkConeSource and set some of its
    // properties. The instance of svtkConeSource "cone" is part of a
    // visualization pipeline (it is a source process object); it produces
    // data (output type is svtkPolyData) which other filters may process.
    //
    svtkConeSource cone = new svtkConeSource();
    cone.SetHeight( 3.0 );
    cone.SetRadius( 1.0 );
    cone.SetResolution( 10 );

    //
    // In this example we terminate the pipeline with a mapper process object.
    // (Intermediate filters such as svtkShrinkPolyData could be inserted in
    // between the source and the mapper.)  We create an instance of
    // svtkPolyDataMapper to map the polygonal data into graphics primitives. We
    // connect the output of the cone source to the input of this mapper.
    //
    svtkPolyDataMapper coneMapper = new svtkPolyDataMapper();
    coneMapper.SetInputConnection(cone.GetOutputPort());

    //
    // Create an actor to represent the cone. The actor orchestrates rendering of
    // the mapper's graphics primitives. An actor also refers to properties via a
    // svtkProperty instance, and includes an internal transformation matrix. We
    // set this actor's mapper to be coneMapper which we created above.
    //
    svtkActor coneActor = new svtkActor();
    coneActor.SetMapper(coneMapper);

    //
    // Create two renderers and assign actors to them. A renderer renders into a
    // viewport within the svtkRenderWindow. It is part or all of a window on the
    // screen and it is responsible for drawing the actors it has.  We also set
    // the background color here. In this example we are adding the same actor
    // to two different renderers; it is okay to add different actors to
    // different renderers as well.
    //
    svtkRenderer ren1 = new svtkRenderer();
    ren1.AddActor(coneActor);
    ren1.SetBackground(0.1, 0.2, 0.4);
    ren1.SetViewport(0.0, 0.0, 0.5, 1.0);

    svtkRenderer ren2 = new svtkRenderer();
    ren2.AddActor(coneActor);
    ren2.SetBackground(0.1, 0.2, 0.4);
    ren2.SetViewport(0.5, 0.0, 1.0, 1.0);

    //
    // Finally we create the render window which will show up on the screen.
    // We add our two renderers into the render window using AddRenderer. We also
    // set the size to be 600 pixels by 300.
    //
    svtkRenderWindow renWin = new svtkRenderWindow();
    renWin.AddRenderer( ren1 );
    renWin.AddRenderer( ren2 );
    renWin.SetSize(600, 300);

    //
    // Make one camera view 90 degrees from other.
    //
    ren1.ResetCamera();
    ren1.GetActiveCamera().Azimuth(90);

    //
    // now we loop over 360 degrees and render the cone each time
    //
    int i;
    for (i = 0; i < 360; ++i)
      {
      Thread.sleep(10);
      // render the image
      renWin.Render();
      // rotate the active camera by one degree
      ren1.GetActiveCamera().Azimuth( 1 );
      ren2.GetActiveCamera().Azimuth( 1 );
      }
    }
}
