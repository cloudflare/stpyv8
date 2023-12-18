#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import unittest
import pytest

ICU_DATA_FOLDERS_UNIX = ("/usr/share/stpyv8", os.path.expanduser("~/.local/share/stpyv8"))
ICU_DATA_FOLDERS_OSX = ("/Library/Application Support/STPyV8", os.path.expanduser('~/Library/Application Support/STPyV8'))
ICU_DATA_FOLDERS_WINDOWS = (os.path.join(os.environ["PROGRAMDATA"], "STPyV8") if "PROGRAMDATA" in os.environ else None, )

icu_data_folders = None
if os.name in ("posix", ):
    icu_data_folders = ICU_DATA_FOLDERS_OSX if sys.platform in ("darwin", ) else ICU_DATA_FOLDERS_UNIX
else:
    icu_data_folders = ICU_DATA_FOLDERS_WINDOWS

ICU_TEST_SKIP = os.name in ("nt", ) and os.getenv('PROGRAMDATA') is None


class TestICU(unittest.TestCase):
    @pytest.mark.skipif(ICU_TEST_SKIP, reason = "ICU data file not available")
    def testIcu(self):
        icu_paths = (os.path.join(folder, "icudtl.dat") for folder in icu_data_folders)
        self.assertTrue(any(os.path.exists(icu) for icu in icu_paths))
