//
// This example introduces the concepts of user interaction with SVTK.
// First, a different interaction style (than the default) is defined.
// Second, the interaction is started.
//
//

// we import the svtk wrapped classes first
import svtk.*;

// then we define our class
public class Cone5 {
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
    // Create the Renderer and assign actors to it. A renderer is like a
    // viewport. It is part or all of a window on the screen and it is
    // responsible for drawing the actors it has.  We also set the
    // background color here.
    //
    svtkRenderer ren1 = new svtkRenderer();
    ren1.AddActor(coneActor);
    ren1.SetBackground(0.1, 0.2, 0.4);

    //
    // Finally we create the render window which will show up on the screen.
    // We add our two renderers into the render window using AddRenderer. We also
    // set the size to be 600 pixels by 300.
    //
    svtkRenderWindow renWin = new svtkRenderWindow();
    renWin.AddRenderer( ren1 );
    renWin.SetSize(300, 300);

    //
    // Make one camera view 90 degrees from other.
    //
    ren1.ResetCamera();
    ren1.GetActiveCamera().Azimuth(90);

    //
    // The svtkRenderWindowInteractor class watches for events (e.g., keypress,
    // mouse) in the svtkRenderWindow. These events are translated into event
    // invocations that SVTK understands (see SVTK/Common/svtkCommand.h for all
    // events that SVTK processes). Then observers of these SVTK events can
    // process them as appropriate.
    svtkRenderWindowInteractor iren = new svtkRenderWindowInteractor();
    iren.SetRenderWindow(renWin);

    //
    // By default the svtkRenderWindowInteractor instantiates an instance
    // of svtkInteractorStyle. svtkInteractorStyle translates a set of events
    // it observes into operations on the camera, actors, and/or properties
    // in the svtkRenderWindow associated with the svtkRenderWinodwInteractor.
    // Here we specify a particular interactor style.
    svtkInteractorStyleTrackballCamera style =
        new svtkInteractorStyleTrackballCamera();
    iren.SetInteractorStyle(style);

    //
    // Unlike the previous examples where we performed some operations and then
    // exited, here we leave an event loop running. The user can use the mouse
    // and keyboard to perform the operations on the scene according to the
    // current interaction style.
    //

    //
    // Initialize and start the event loop. Once the render window appears,
    // mouse in the window to move the camera. The Start() method executes
    // an event loop which listens to user mouse and keyboard events. Note
    // that keypress-e exits the event loop. (Look in svtkInteractorStyle.h
    // for a summary of events, or the appropriate Doxygen documentation.)
    //
    iren.Initialize();
    iren.Start();
  }
}
