# File: Makefile
# DateTime: Fri Mar 28 12:27:18 2025
# Last checked in by: starritt
#
TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# Make EPICS 7 friendly
#
USR_CFLAGS += -DUSE_TYPED_RSET

#==================================================
# build a support library

LIBRARY_IOC += Rattle

DBD += Rattle.dbd

# specify all source files to be compiled and added to the library
#
Rattle_SRCS += Rattle.c

Rattle_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

# end
