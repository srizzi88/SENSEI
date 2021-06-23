//
// This example creates a polygonal model of a cone, and then renders it to
// the screen. It will rotate the cone 360 degrees and then exit. The basic
// setup of source -> mapper -> actor -> renderer -> renderwindow is
// typical of most SVTK programs.
//

// We import the svtk wrapped classes first.
import svtk.*;

// Then we define our class.
public class Cone {

  // In the static constructor we load in the native code.
  // The libraries must be in your path to work.
  static {
    System.loadLibrary("svtkCommonJava");
    System.loadLibrary("svtkFilteringJava");
    System.loadLibrary("svtkIOJava");
    System.loadLibrary("svtkImagingJava");
    System.loadLibrary("svtkGraphicsJava");
    System.loadLibrary("svtkRenderingJava");
  }

  // now the main program
  public static void main (String []args) {
    //
    // Next we create an instance of svtkConeSource and set some of its
    // properties. The instance of svtkConeSource "cone" is part of a
    // visualization pipeline (it is a source process object); it produces data
    // (output type is svtkPolyData) which other filters may process.
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
    coneMapper.SetInputConnection( cone.GetOutputPort() );

    //
    // Create an actor to represent the cone. The actor orchestrates rendering
    // of the mapper's graphics primitives. An actor also refers to properties
    // via a svtkProperty instance, and includes an internal transformation
    // matrix. We set this actor's mapper to be coneMapper which we created
    // above.
    //
    svtkActor coneActor = new svtkActor();
    coneActor.SetMapper( coneMapper );

    //
    // Create the Renderer and assign actors to it. A renderer is like a
    // viewport. It is part or all of a window on the screen and it is
    // responsible for drawing the actors it has.  We also set the background
    // color here
    //
    svtkRenderer ren1 = new svtkRenderer();
    ren1.AddActor( coneActor );
    ren1.SetBackground( 0.1, 0.2, 0.4 );

    //
    // Finally we create the render window which will show up on the screen
    // We put our renderer into the render window using AddRenderer. We also
    // set the size to be 300 pixels by 300
    //
    svtkRenderWindow renWin = new svtkRenderWindow();
    renWin.AddRenderer( ren1 );
    renWin.SetSize( 300, 300 );

    //
    // now we loop over 360 degrees and render the cone each time
    //
    int i;
    for (i = 0; i < 360; ++i)
      {
      // render the image
      renWin.Render();
      // rotate the active camera by one degree
      ren1.GetActiveCamera().Azimuth( 1 );
      }

    }
}

