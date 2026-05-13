Configure behavior changes
==========================
When new backward incompatible changes are implemented in DLite, the user will in
most cases get a warning of the form

> Warning: Behavior `<NAME>` is not configured. It will be enabled by default from v0.7.0. See https://sintef.github.io/dlite/user_guide/configure_behavior_changes.html for more info.

where `<NAME>` will be the name of the behavior that is going to be changed.

You can turn off this warning by configuring whether you want the old (deprecated) behavior or the new behavior by defining the environment variable `DLITE_BEHAVIOR_<NAME>`.

If `DLITE_BEHAVIOR_<NAME>` is defined without a value or its value is one of "true", ".true.", "on", "yes" or 1, the behavior change will be enabled.

If `DLITE_BEHAVIOR_<NAME>` is one of "false", ".false.", "off", "no" or 0, the behavior change will be disabled.

It is also possible to enable/disable all defined behavior changes by setting the environment variable `DLITE_BEHAVIOR`. This can be overwritten for individual behavior changes by setting `DLITE_BEHAVIOR_<NAME>`.

Currently the different scheduled behavior changes are only documented in the [behavior table] in the source code.  In this table you can find a description of the behavior change `<NAME>` in addition to the version it was added, the version it will be turned on by default and the version it is planned for removal.





[behavior table]: https://github.com/SINTEF/dlite/blob/6f38fb49ea2dbb6252bf44cd6d239a6adff6050e/src/dlite-behavior.c#L23
