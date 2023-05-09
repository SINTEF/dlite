# -- initial cmake cache file for building agains python3.7


# Build DLite against python3.7 on a system with a newer interpreter
# ==================================================================
#
# 1. Install python3.7
# --------------------
# On Fedora do `dnf install python3.7`.
#
# 2. Create virtual environment
# -----------------------------
# With virtualenvwrapper installed, do
#
#     mkvirtualenv dlite37 -p /usr/bin/python3.7
#     python3.7 -m pip install -U pip
#     python3.7 -m pip install -r requirements.txt
#     python3.7 -m pip install -r requirements_dev.txt
#
# 3. Build dlite
# --------------
#     mkdir build-python3.7
#     cd build-python3.7
#     cmake -C ../build-python3.7.cmake ..
#


file(REAL_PATH "~/.envs/dlite37" virtualenv EXPAND_TILDE)

set(CMAKE_INSTALL_PREFIX "${virtualenv}" CACHE PATH
  "Install path prefix, prepended onto install directories.")

set(Python3_EXECUTABLE "${virtualenv}/bin/python3.7" CACHE FILEPATH
  "Python executable.")

set(Python3_LIBRARY "/usr/lib64/libpython3.7m.so" CACHE FILEPATH
  "Python library.")

set(Python3_INCLUDE_DIR "/usr/include/python3.7m" CACHE PATH
  "Python include dir.")

set(Python3_NumPy_INCLUDE_DIR "${virtualenv}/lib64/python3.7/site-packages/numpy/core/include" CACHE PATH "NumPy include dir.")
