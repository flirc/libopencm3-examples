##
## This file is part of the libopencm3 project.
##
## Copyright (C) 2018 Jason Kotzin <jasonkotzin@gmail.com>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

BINARY = uart_shell
DEVICE = samd21E17

SCMVERSION := $(shell git describe --tags --dirty=+ 2>/dev/null || \
        git rev-parse --short=16 HEAD 2>/dev/null || \
        echo "UNKNOWN")
BRANCH := $(shell git branch 2> /dev/null || echo '* UNKNOWN' | awk '/^\*/ {print $2;}')

TGT_CFLAGS += -DSCMVERSION='"$(SCMVERSION)"' -DBRANCH='"$(BRANCH)"' -Wno-unused-parameter
TGT_CFLAGS += -nodefaultlibs -ffreestanding

include ../../Makefile.include
