NAME = CELLMACE
DESCRIPTION = "Cell Machine CE"
ICON = icon.png

ARCHIVED = NO

COMPRESSED = YES
COMPRESSED_MODE = zx0
HAS_PRINTF = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

EXTRA_CLEAN = cd $(GFXDIR) && convimg --clean

include $(shell cedev-config --makefile)