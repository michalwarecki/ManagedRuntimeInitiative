#
# Copyright 1998-2007 Sun Microsystems, Inc.  All Rights Reserved.
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
#
# This code is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 only, as
# published by the Free Software Foundation.  Sun designates this
# particular file as subject to the "Classpath" exception as provided
# by Sun in the LICENSE file that accompanied this code.
#
# This code is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# version 2 for more details (a copy is included in the LICENSE file that
# accompanied this code).
#
# You should have received a copy of the GNU General Public License version
# 2 along with this work; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
#
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
# CA 95054 USA or visit www.sun.com if you need additional information or
# have any questions.
#

#
# Makefile for building javax
#

BUILDDIR = ..
include $(BUILDDIR)/common/Defs.gmk

#
# Files to compile
#
AUTO_FILES_JAVA_DIRS =       \
    javax/naming             \
    com/sun/naming/internal  \
    javax/net                \
    javax/script             \
    javax/security/auth      \
    javax/security/cert      \
    javax/security/sasl      \
    javax/smartcardio        \
    javax/tools              \
    javax/xml

# imageio uses xml, so build it last

SUBDIRS = \
	accessibility \
	print \
	swing \
	crypto \
	sound \
	sql \
	rmi \
	management \
	imageio

all build:
	$(SUBDIRS-loop)
clean clobber::
	$(SUBDIRS-loop)

#
# Rules
#
include $(BUILDDIR)/common/Classes.gmk

