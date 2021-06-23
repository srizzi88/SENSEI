//
// This example shows how to add an observer to a Java program. It extends
// the Step1/Java/Cone.java Java example (see that example for information on
// the basic setup).
//
// SVTK uses a command/observer design pattern. That is, observers watch for
// particular events that any svtkObject (or subclass) may invoke on
// itself. For example, the svtkRenderer invokes a "StartEvent" as it begins
// to render. Here we add an observer that invokes a command when this event
// is observed.
//
//
// Show how to add an observer to the Cone example
//

// we import the svtk wrapped classes first
import svtk.*;

// then we define our class
public class Cone2 {
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

  // Define the callback
  public void myCallback()
  {
    System.out.println("Starting a render");
  }

  // now the main program
  public static void main (String []args) {
    //
    // Now we create the pipeline as usual, see Cone.java in Step1 for details
    //
    svtkConeSource cone = new svtkConeSource();
    cone.SetHeight( 3.0 );
    cone.SetRadius( 1.0 );
    cone.SetResolution( 10 );

    svtkPolyDataMapper coneMapper = new svtkPolyDataMapper();
    coneMapper.SetInputConnection( cone.GetOutputPort() );
    svtkActor coneActor = new svtkActor();
    coneActor.SetMapper( coneMapper );

    svtkRenderer ren1 = new svtkRenderer();
    ren1.AddActor( coneActor );
    ren1.SetBackground( 0.1, 0.2, 0.4 );

    // Add the observer here, the first argument is the event name
    // the second argument is the instance to invoke the method on
    // the third argument is which method to invoke
    Cone2 me = new Cone2();
    ren1.AddObserver("StartEvent",me,"myCallback");

    // setup the window
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

