#!/bin/sh
#
# Patch virtualenv activate script to support LD_LIBRARY_PATH
#

if [ -z "$VIRTUAL_ENV" ]; then
   echo "Not in a virtual environment" >&2
   exit 1
fi

cd $VIRTUAL_ENV/bin

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
@@ -54,6 +59,10 @@
 PATH="\$VIRTUAL_ENV/bin:\$PATH"
 export PATH

+_OLD_VIRTUAL_LD_LIBRARY_PATH="\$LD_LIBRARY_PATH"
+LD_LIBRARY_PATH="\$VIRTUAL_ENV/lib:\$LD_LIBRARY_PATH"
+export LD_LIBRARY_PATH
+
 # unset PYTHONHOME if set
 if ! [ -z "\${PYTHONHOME+_}" ] ; then
     _OLD_VIRTUAL_PYTHONHOME="\$PYTHONHOME"
EOF

exit $?
