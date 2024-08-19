Code changes
============
Although that DLite has not reached version 1.0.0, the intention is for it to be stable and not introduce unnecessary backward-incompatible changes.


Versioning
----------
Before reaching version 1.0.0, DLite follows the following versioning rules that are well-known from Python:

* **patch release**: Only backward-compatible changes.
  - Addition of new backward-compatible features.
  - New deprecation warnings can be added.
  - Bug fixes.

* **minor release**: Backward-incompatible changes.
  - Addition of new backward-incompatible features.
  - Removal of deprecated features.
  - Changing default behavior.

After version 1.0.0, DLite should strictly follow the [semantic versioning] rules.


Feature deprecation
-------------------
In Python, use `dlite.deprecation_warning()` to mark deprecation warnings.
This function is a simple wrapper around `warnings.warn(msg, DeprecationWarning, stacklevel=2)` with the release version when the feature is planned to be removed as an additional argument.

In C, use the `dlite_deprecation_warning()` function to mark a deprecation warning.

The idea with these functions is to make it easy to grep for deprecation warnings that should be remove in a new backward-incompatible release.
The following command can e.g. be used to find all deprecation warnings:

    find \( -name '*.py' -o -name '*.i' -o -name '*.c' \) ! -path './build*' | xargs grep -n -A1 deprecation_warning


How to handle different types of code changes
---------------------------------------------

### Backward-compatible API changes
The easiest API changes are those that only adds new functions or classes without changing the existing code, apart from adding deprecation warnings hinting about how to use the new API.

Adding new arguments to an existing function or new methods to existing classes falls also into this category.
**Note**, new (positional) arguments should be added to the end of the argument list, such that existing code using positional arguments will be unaffected by the change.

### New behavior
All changes in behavior in existing code should be considered backward-incompatible.

Where it make sense, DLite will try to optionally keep the new/old behaviour over some releases in order for users to adapt to the new behavior.

Each optional new behavior should have a:
- name
- description
- release number for when the behavior was introduced (keeping old behavior as default)
- release number for when the new behavior should be the default (managed automatically)
- expected release number for when the old behavior should be removed (require developer effort)
- value: True means that the behavior is enabled.

All behavior changes will be described in a single source file `src/dlite-behavior.c`.

Whether to enable a behavior can be configured in several ways:

- **programmatically from Python**:

      >>> dlite.Behavior.<NAME> = True

- **programmatically from C**:

      dlite_behavior_set("<NAME>", 1);

- **via environment variables**:

      export DLITE_BEHAVIOR_<NAME>=1

  You can use the environment variable `DLITE_BEHAVIOR` to initialise all
  behavior variables.  This is e.g. useful for testing.

  The empty string or any of the following values will enable the behavior:
  "true", ".true.", "on", "yes", 1

  Any of the following values will disable the behavior:
  "false", ".false.", "off", "no", 0

A warning will automatically be issued if a behavior is not selected explicitly.


[semantic versioning]: https://semver.org/
