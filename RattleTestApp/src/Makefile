# $File$
# $Revision$
# $DateTime$
# Last checked in by: $Author$
#

TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = RattleTest
# RattleTest.dbd will be created and installed
DBD += RattleTest.dbd

# RattleTest.dbd will be made up from these files:
RattleTest_DBD += base.dbd

# Include dbd files from all support applications:
# Add all the support libraries needed by this IOC
#
RattleTest_DBD  += Rattle.dbd
RattleTest_LIBS += Rattle

# RattleTest_registerRecordDeviceDriver.cpp derives from RattleTest.dbd
RattleTest_SRCS += RattleTest_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
RattleTest_SRCS_DEFAULT += RattleTestMain.cpp
RattleTest_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#RattleTest_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
RattleTest_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

# end
