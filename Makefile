NAME = CELLMACE
DESCRIPTION = "Cell Machine CE"
ICON = icon.png
VERSION_NO = v1.1
YEAR = 2026

ARCHIVED = NO

COMPRESSED = YES
COMPRESSED_MODE = zx0
HAS_PRINTF = NO

CFLAGS = -Wall -Wextra -Oz -DVERSION_NO='"$(VERSION_NO)"' -DYEAR='"$(YEAR)"'
CXXFLAGS = -Wall -Wextra -Oz -DVERSION_NO='"$(VERSION_NO)"' -DYEAR='"$(YEAR)"'

EXTRA_CLEAN = cd $(GFXDIR) && convimg --clean

include $(shell cedev-config --makefile)