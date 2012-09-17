pickAndPlaceMachine
===================

This Qt based program renders 2D/3D views of your RS-274x gerber files and uses a pick&place file (Centroid) for positioning 3D models of the components.
Currently the software is pre-alpha and not everything works correctly.


License
-------

GPLv3


Platforms known to work
-----------------------

Ubuntu 12.04 (Precise) is used for development.
It should work on other platforms. Windows will be a target later.


Compilation
-----------

Dependencies: vtk

    $ qmake
    $ make
    $ ./pickAndPlaceMachine


Author
------

Sebastian Held <sebastian.held@gmx.de>

Main repository: github.com/sibbi77/pickAndPlaceMachine
