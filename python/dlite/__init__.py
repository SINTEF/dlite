# This empty package exists for setup.py to not fail on setup(...) because
# setup cannot find a package "dlite" in the root directory. We are using
# setup(packages=[dlite"], ...) to start setting up an empty package which is
# then populated by the CMake build.
