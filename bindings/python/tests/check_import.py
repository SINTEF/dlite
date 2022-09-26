
import sys
import importlib
import warnings


def check_import(module_name, package=None, skip=False, warn=None):
    """Try to import `module_name`.

    Arguments
        module_name: Name of module to import.
        package: Only needed for relative imports.
        skip: Whether to skip the test if the module cannot be imported.
        warn: Whether to warn if the module cannot be loaded.  The default
            is to warn if `skip` is false.

    Returns:
        A module object if the module can be imported.  Otherwise None is
        returned.
    """
    try:
        module = importlib.import_module(module_name, package)
    except ImportError:
        if skip:
            sys.exit(44)  # tell CMake to skip the test
        elif warn or warn is None:
            warnings.warn(f'cannot load module: "{module_name}"',
                          stacklevel=2)
        return None
    return module
