## Modules

A Cassette program is broken up into modules. At runtime, the global environment
contains primitive functions, plus the results of previously instantiated
modules. Each module is instantiated in order of dependency, and the entrypoint
module is instantiated last.

A module is instantiated with these steps:

- If the module imports any dependencies, then it extends the environment to
  alias each dependency. Each imported module is looked up and then redefined in
  the alias frame.
- The body of the module is executed. If this includes any defines or let
  assignments, the environment is extended for these to be defined.
- The module's exports are packaged into a tuple, the environment is reverted to
  the global environment, and the export tuple is defined there as the module's
  value.

Within a module, imported functions can be used up with these steps:

- The imported module is looked up in the alias frame.
- The function is looked up in the imported module's export tuple.
