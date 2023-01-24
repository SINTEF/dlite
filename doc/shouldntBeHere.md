
Setting up the environment
--------------------------

### Note about VirtualEnvWrapper

By default, [VirtualEnvWrapper](https://virtualenvwrapper.readthedocs.io/en/latest/) does not set `LD_LIBRARY_PATH`. There are two ways of fixing this. 

First, after compiling and installing `dlite`, the user needs to modify the `activate` file, located at `$WORKON_HOME/<envs_name>/bin/activate` by adding
``` bash
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$VIRTUAL_ENV/lib"
```


Alternatively, for proper a cleanup, the user may add
``` bash
_OLD_LD_LIBRARY_PATH="$LD_LIBRARY_PATH"
export LD_LIBRARY_PATH="$VIRTUAL_ENV/lib:$LD_LIBRARY_PATH"
```
to the end of `$WORKON_HOME/<env_name>/bin/activate`, followed by


``` bash
export LD_LIBRARY_PATH="$_OLD_LD_LIBRARY_PATH"
unset _OLD_LD_LIBRARY_PATH
```
to the end of the `deactivate` shell function in the same file.

