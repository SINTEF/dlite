Patch activate
==============
By default, [virtualenv] does not set `LD_LIBRARY_PATH`.
This will result in errors when running applications that links to libdlite, like for example, `dlite-codegen`.
To fix this, `$VIRTUAL_ENV/lib/` needs to be appended/prepended to `LD_LIBRARY_PATH`.
This is attempted done with the script patch_activate.sh which is run as a part of the installation.

If not successful, this can be done manually by modifying the `activate` shell file, located at `$WORKON_HOME/<envs_name>/bin/activate`. First, the user should add

``` bash
if ! [ -z "${_OLD_LD_LIBRARY_PATH}" ] ; then
    LD_LIBRARY_PATH="$_OLD_LD_LIBRARY_PATH"
    export LD_LIBRARY_PATH
    unset _OLD_LD_LIBRARY_PATH
fi
```
at the end of the `deactivate` function in the `activate` shell file. Next, add
``` bash
export _OLD_LD_LIBRARY_PATH=$LD_LIBRARY_PATH
export LD_LIBRARY_PATH="$VIRTUAL_ENV/lib/:$LD_LIBRARY_PATH"
```
at the end of `activate`.

**Explanation:** The value of `LD_LIBRARY_PATH` is exported (saved) into a new temporary environment variable, `_OLD_LD_LIBRARY_PATH`.
`$VIRTUAL_ENV/lib/` is then prepended to `LD_LIBRARY_PATH`.
The `if` statement within the `deactivate` function checks whether the variable `_OLD_LD_LIBRARY_PATH` has been declared.
If true, then the `deactivate` function will set `LD_LIBRARY_PATH` to its original value and unset the temporary environment variable `_OLD_LD_LIBRARY_PATH`.



[virtualenv]: https://docs.python.org/3/library/venv.html
