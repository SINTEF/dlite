"""Test dlite.testutils"""
from dlite.testutils import (
    raises, UnexpectedSuccessError, UnexpectedExceptionError
)


with raises(ValueError):
    raise ValueError("an expected exception, should not be reported")


try:
    with raises(ValueError):
        pass
except UnexpectedSuccessError:
    pass
else:
    assert False, "An UnexpectedSuccessError was not produced!"


try:
    with raises(ValueError):
        raise TypeError("raining an unexpected exception")
except UnexpectedExceptionError:
    pass
else:
    assert False,  "An UnexpectedExceptionError was not produced!"
