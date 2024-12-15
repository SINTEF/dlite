#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys

import dlite
from dlite.testutils import raises


assert dlite.get_uuid_version() == 4
assert dlite.get_uuid_version("abc") == 5
assert dlite.get_uuid_version("6cb8e707-0fc5-5f55-88d4-d4fed43e64a8") == 0
assert dlite.get_uuid("abc") == "6cb8e707-0fc5-5f55-88d4-d4fed43e64a8"
assert dlite.get_uuid("6cb8e707-0fc5-5f55-88d4-d4fed43e64a8") == (
    "6cb8e707-0fc5-5f55-88d4-d4fed43e64a8"
)

assert dlite.join_meta_uri("name", "version", "ns") == "ns/version/name"
assert dlite.split_meta_uri("ns/version/name") == ["name", "version", "ns"]

assert dlite.join_url("driver", "loc", "mode=r", "fragment") == (
    "driver://loc?mode=r#fragment"
)
assert dlite.join_url("driver", "loc", "mode=r") == "driver://loc?mode=r"
assert dlite.join_url("driver", "loc") == "driver://loc"
assert dlite.join_url("driver", "loc", fragment="frag") == "driver://loc#frag"

assert dlite.split_url("driver://loc?mode=r#fragment") == [
    "driver",
    "loc",
    "mode=r",
    "fragment",
]
assert dlite.split_url("driver://loc?mode=r&verbose=1") == [
    "driver",
    "loc",
    "mode=r&verbose=1",
    "",
]
assert dlite.split_url("driver://loc#fragment") == [
    "driver",
    "loc",
    "",
    "fragment",
]
assert dlite.split_url("loc#fragment") == ["", "loc", "", "fragment"]


# Test errctl context manager
print("No DLite error message is printed to screen:")
try:
    with dlite.errctl(hide=(dlite.DLiteStorageOpenError, dlite.DLiteError)):
        with dlite.HideDLiteWarnings():
            dlite.Instance.from_location("-", "__non-existing__")
except dlite.DLiteStorageOpenError:
    pass

print("No DLite error message is printed to screen:")
try:
    with dlite.errctl(hide="DLiteError"):
        dlite.Instance.from_location("-", "__non-existing__")
except dlite.DLiteStorageOpenError:
    pass

print("DLite error message is printed to screen:")
try:
    with dlite.errctl(hide=dlite.DLiteTypeError):
        dlite.Instance.from_location("-", "__non-existing__")
except dlite.DLiteStorageOpenError:
    pass


# Test checking and comparing semantic version numbers
assert dlite.chk_semver(dlite.__version__) > 0
assert dlite.chk_semver("1.12.2-rc1.alpha+34") == 19
assert dlite.chk_semver("1.12.2-rc1.alpha+34 ") < 0
assert dlite.chk_semver("1.12.2-rc1.alpha+34 ", 6) == 6
assert dlite.chk_semver("1.12.2-rc1.alpha+34 ", 19) == 19

assert dlite.cmp_semver("1.3.11", "1.3.3") > 0
assert dlite.cmp_semver("1.3.11", "1.3.13") < 0
assert dlite.cmp_semver("1.3.11", "1.3.13", 5) == 0


# Test deprecation warnings
# Future deprecation is not displayed
dlite.deprecation_warning("100.3.2", "My deprecated feature...")

with raises(SystemError):
    dlite.deprecation_warning("0.0.1", "My deprecated feature 2...")

# Issuing the same deprecation warning a second or third time should not
# raise an exception
dlite.deprecation_warning("0.0.1", "My deprecated feature 2...")
dlite.deprecation_warning("0.0.1", "My deprecated feature 2...")

with raises(SystemError):
    dlite.deprecation_warning("0.0.x", "My deprecated feature 3...")


# Test uri encode/decode
assert dlite.uriencode("") == ""
assert dlite.uriencode("abc") == "abc"
assert dlite.uriencode("abc\x00def") == "abc%00def"

assert dlite.uridecode("") == ""
assert dlite.uridecode("abc") == "abc"
assert dlite.uridecode("abc%00def") == "abc\x00def"

assert dlite.uridecode(dlite.uriencode("ÆØÅ")) == "ÆØÅ"

# Ignore Windows - it has its own encoding (utf-16) of non-ascii characters
if sys.platform != "win32":
    assert dlite.uriencode("å") == "%C3%A5"
    assert dlite.uridecode("%C3%A5") == "å"
