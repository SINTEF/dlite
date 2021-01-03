#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import sys

import dlite


assert dlite.get_uuid_version() == 4
assert dlite.get_uuid_version('abc') == 5
assert dlite.get_uuid_version('6cb8e707-0fc5-5f55-88d4-d4fed43e64a8') == 0
assert dlite.get_uuid('abc') == '6cb8e707-0fc5-5f55-88d4-d4fed43e64a8'
assert dlite.get_uuid('6cb8e707-0fc5-5f55-88d4-d4fed43e64a8') == (
    '6cb8e707-0fc5-5f55-88d4-d4fed43e64a8')

assert dlite.join_meta_uri('name', 'version', 'ns') == 'ns/version/name'
assert dlite.split_meta_uri('ns/version/name') == ['name', 'version', 'ns']

assert dlite.join_url('driver', 'loc', 'mode=r', 'fragment') == (
    'driver://loc?mode=r#fragment')
assert dlite.join_url('driver', 'loc', 'mode=r') == 'driver://loc?mode=r'
assert dlite.join_url('driver', 'loc') == 'driver://loc'
assert dlite.join_url('driver', 'loc', fragment='frag') == 'driver://loc#frag'

assert dlite.split_url('driver://loc?mode=r#fragment') == [
    'driver', 'loc', 'mode=r', 'fragment']
assert dlite.split_url('driver://loc?mode=r&verbose=1') == [
    'driver', 'loc', 'mode=r&verbose=1', '']
assert dlite.split_url('driver://loc#fragment') == [
    'driver', 'loc', '', 'fragment']
assert dlite.split_url('loc#fragment') == [
    '', 'loc', '', 'fragment']
