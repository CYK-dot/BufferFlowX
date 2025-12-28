#!/bin/bash

# Run Python tests for section module
cd section
python3 -m pytest pytest_ut_get_region.py pytest_ut_collect_section.py
cd ..