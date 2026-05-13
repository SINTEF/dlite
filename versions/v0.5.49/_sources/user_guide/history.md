DLite is an implementation of [SOFT], which stands for SINTEF Open
Framework and Tools and is a set of concepts for how to achieve
semantic interoperability as well as implementations and corresponding
tooling.

The development of SOFT was motivated by many years of experience with
developing scientific software, where it was observed that a lot of
efforts went into developing parts that had little to do with the
domain.
A significant part of the development process was spent on different
software engineering tasks, such as code design, the handling of I/O,
correct memory handling of the program state and writing import and
export filters in order to use data from different
sources.
In addition comes the code maintenance with support of legacy formats
and the introduction of new features and changes to internal data
state in the scientific software.
With SOFT it is possible to utilize reusable software components that
handle all this, or develop new reusable software components that can
be used by others in the same framework.
At the core of SOFT are the [SOFT data models], which by design provide
a simple but powerful way to represent scientific data.

Originally DLite started as a simplified pure C implementation of SOFT
based on [SOFT5], but has with time developed into a robust framework
with a large set of [features].
There is also [SOFT7], a new version of SOFT written in pure Python
where new features are tested out.


[SOFT]: https://www.sintef.no/en/publications/publication/1553408/
[SOFT5]: https://github.com/NanoSim/Porto/blob/porto/Preview-Final-Release/doc/manual/02_soft_introduction.md
[SOFT7]: https://github.com/SINTEF/soft7
[SOFT data models]: https://github.com/NanoSim/Porto/blob/porto/Preview-Final-Release/doc/manual/02_soft_introduction.md#soft5-features
