"""Python module with mapping functions."""
import numpy as np


def formula(symbols):
    """Convert a list of atomic symbols to a chemical formula."""
    lst = symbols.tolist()
    return "".join(f"{c}{lst.count(c)}" for c in set(lst))


def norm(array, axis=-1):
    """Returns the norm array along the given axis (default the last)."""
    # Note that `array` is a Quantity object.  The returned value
    # will also be a Quantity object with the same unit.  Hence, the
    # unit is always handled explicitly.  This makes it possible for
    # conversion function to change unit as well.
    return np.sqrt(np.sum(array**2, axis=axis))


def max(vector):
    """Returns the largest element."""
    return vector.max()
