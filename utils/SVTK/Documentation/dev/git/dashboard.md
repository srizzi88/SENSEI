SVTK Dashboard Scripts
=====================

This page documents how to use the SVTK `dashboard` branch in [Git][].
See the [README](README.md) for more information.

[Git]: http://git-scm.com

Using the Dashboard Scripts
---------------------------

The `dashboard` branch contains a dashboard client helper script.
Use these commands to track it:

    $ mkdir -p ~/Dashboards/SVTKScripts
    $ cd ~/Dashboards/SVTKScripts
    $ git init
    $ git remote add -t dashboard origin https://gitlab.kitware.com/svtk/svtk.git
    $ git pull origin

The `svtk_common.cmake` script contains setup instructions in its
top comments.

Update the `dashboard` branch to get the latest version of this
script by simply running:

    $ git pull

Here is a link to the script as it appears today: [svtk_common.cmake][].

[svtk_common.cmake]: https://gitlab.kitware.com/svtk/svtk/tree/dashboard/svtk_common.cmake

Changing the Dashboard Scripts
------------------------------

If you find bugs in the hooks themselves or would like to add new features,
the can be edited in the usual Git manner:

    $ git checkout -b my-topic-branch

Make your edits, test it, and commit the result.  Create a patch file with:

    $ git format-patch origin/dashboard

And post the results in the [Development][] category in the [SVTK Discourse][] forum.

[Development]: https://discourse.svtk.org/c/development
[SVTK Discourse]: https://discourse.svtk.org/
