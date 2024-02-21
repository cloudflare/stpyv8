#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import unittest

ICU_DATA_FOLDERS_UNIX = (
    "/usr/share/stpyv8",
    os.path.expanduser("~/.local/share/stpyv8"),
)
ICU_DATA_FOLDERS_OSX = (
    "/Library/Application Support/STPyV8",
    os.path.expanduser("~/Library/Application Support/STPyV8"),
)
ICU_DATA_FOLDERS_WINDOWS = (
    os.path.join(os.environ["PROGRAMDATA"], "STPyV8")
    if "PROGRAMDATA" in os.environ
    else None,
    os.path.join(os.environ["APPDATA"], "STPyV8") if "APPDATA" in os.environ else None,
)

icu_data_folders = None
if os.name in ("posix",):
    icu_data_folders = (
        ICU_DATA_FOLDERS_OSX if sys.platform in ("darwin",) else ICU_DATA_FOLDERS_UNIX
    )
else:
    icu_data_folders = ICU_DATA_FOLDERS_WINDOWS


class TestICU(unittest.TestCase):
    def testIcu(self):
        icu_paths = (os.path.join(folder, "icudtl.dat") for folder in icu_data_folders)
        self.assertTrue(any(os.path.exists(icu) for icu in icu_paths))
