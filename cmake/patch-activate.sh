#!/bin/bash

help() {
cat <<EOF
Usage: patch-activate.sh [--package PACKAGE]

Patch virtualenv activate script to support LD_LIBRARY_PATH

Options:
  -h, --help   Show this help and exit.
  -p PACKAGE, --package PACKAGE
               Insert root directory of this Python package to
               LD_LIBRARY_PATH.

How to use this script:
  1. Make sure that environment variable VIRTUAL_ENV is set to the
     root directory of your virtual environments.
  2. Activate your virtual environment.
  3. Install dlite
  4. Run `patch-activate.sh`
  5. Deactivate and reactivate your virtual environment to ensure that
     LD_LIBRARY_PATH is properly set.
EOF
}

# Usage: app_package PACKAGE
# Append path to root of Python package to `libdirs`
add_package() {
    pkgdir=$(dirname $(python -c "import $1; print($1.__file__)"))
    relpath=$(realpath --relative-to="$VIRTUAL_ENV" "$pkgdir")
    if [ "${relpath#..}" = "$relpath" ]; then
        libdirs="$libdirs:\$VIRTUAL_ENV/$relpath"
    else
        echo ""; exit 1
    fi
}


set -e

if [ -z "$VIRTUAL_ENV" ]; then
   echo "Not in a virtual environment" >&2
   exit 1
fi

#libdirs=$(python -c 'import pathlib, site; print(":".join([f"{p}/dlite" for p in site.getsitepackages() if pathlib.Path(f"{p}/dlite").exists()] + [str(pathlib.Path(p).parent.parent) for p in site.getsitepackages()]))')
libdirs=$(python -c 'import pathlib, site; print(":".join(str(pathlib.Path(p).parent.parent) for p in site.getsitepackages()))')

# Parse options
while true; do
    case "$1" in
        '')            break;;
        -h|--help)     help; exit 0;;
        -p|--package)  add_package $2; shift;;
        *)             echo "Invalid argument: '$1'"; exit 1;;
    esac
    shift
done


cd $VIRTUAL_ENV/bin

# We try two variants of the first hunk and three of the second hunk,
# corresponding to the different versions of the activate script created
# with mkvirtualenvwrapper and venv from the standard Python library for
# Python 3.7, 3.9 and 3.11.
# Hence, at least tree hunks will always fail.
patch -u -f -F 1 <<EOF
--- activate    2023-06-18 20:37:58.486341271 +0200
+++ activate.diff       2023-06-18 21:51:48.895940777 +0200
@@ -17,6 +17,11 @@
         export PATH
         unset _OLD_VIRTUAL_PATH
     fi
+    if ! [ -z "\${_OLD_VIRTUAL_LD_LIBRARY_PATH+_}" ] ; then
+        LD_LIBRARY_PATH="\$_OLD_VIRTUAL_LD_LIBRARY_PATH"
+        export LD_LIBRARY_PATH
+        unset _OLD_VIRTUAL_LD_LIBRARY_PATH
+    fi
     if ! [ -z "\${_OLD_VIRTUAL_PYTHONHOME+_}" ] ; then
         PYTHONHOME="\$_OLD_VIRTUAL_PYTHONHOME"
         export PYTHONHOME
@@ -8,6 +8,11 @@
         export PATH
         unset _OLD_VIRTUAL_PATH
     fi
+    if [ -n "\${_OLD_VIRTUAL_LD_LIBRARY_PATH:-}" ] ; then
+        LD_LIBRARY_PATH="\${_OLD_VIRTUAL_LD_LIBRARY_PATH:-}"
+        export LD_LIBRARY_PATH
+        unset _OLD_VIRTUAL_LD_LIBRARY_PATH
+    fi
     if [ -n "\${_OLD_VIRTUAL_PYTHONHOME:-}" ] ; then
         PYTHONHOME="\${_OLD_VIRTUAL_PYTHONHOME:-}"
         export PYTHONHOME
@@ -54,6 +59,10 @@
 PATH="\$VIRTUAL_ENV/bin:\$PATH"
 export PATH

+_OLD_VIRTUAL_LD_LIBRARY_PATH="\$LD_LIBRARY_PATH"
+LD_LIBRARY_PATH="$libdirs:\$LD_LIBRARY_PATH"
+export LD_LIBRARY_PATH
+
 # unset PYTHONHOME if set
 if ! [ -z "\${PYTHONHOME+_}" ] ; then
     _OLD_VIRTUAL_PYTHONHOME="\$PYTHONHOME"
@@ -45,6 +50,10 @@
 PATH="$VIRTUAL_ENV/bin:$PATH"
 export PATH

+_OLD_VIRTUAL_LD_LIBRARY_PATH="$LD_LIBRARY_PATH"
+LD_LIBRARY_PATH="$libdirs:$LD_LIBRARY_PATH"
+export LD_LIBRARY_PATH
+
 # unset PYTHONHOME if set
 # this will fail if PYTHONHOME is set to the empty string (which is bad anyway)
 # could use \`if (set -u; : $PYTHONHOME) ;\` in bash
@@ -60,6 +60,10 @@ _OLD_VIRTUAL_PATH="$PATH"
 PATH="$VIRTUAL_ENV/bin:$PATH"
 export PATH

+_OLD_VIRTUAL_LD_LIBRARY_PATH="\$LD_LIBRARY_PATH"
+LD_LIBRARY_PATH="$libdirs:\$LD_LIBRARY_PATH"
+export LD_LIBRARY_PATH
+
 if [ "x" != x ] ; then
     VIRTUAL_ENV_PROMPT=""
 else
EOF

exit $?
