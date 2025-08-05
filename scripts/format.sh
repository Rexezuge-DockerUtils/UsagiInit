#!/bin/bash
find . -regex '.*\.\(c\|h\)' -exec clang-format -i {} +
