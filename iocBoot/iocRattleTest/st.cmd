#!../../bin/linux-x86_64/RattleTest
  
# $File$
# $Revision$
# $DateTime$
# Last checked in by: $Author$
#
  
#- You may have to change ThresholdEstimateTest to something else
#- everywhere it appears in this file
  
< envPaths
  
cd "${TOP}"
  
## Register all support components
dbLoadDatabase "dbd/RattleTest.dbd"
RattleTest_registerRecordDeviceDriver pdbbase
  
## Load record instances
#
dbLoadRecords("db/test_rattle.db","")
  
cd "${TOP}/iocBoot/${IOC}"
iocInit
  
dbl
  
# end
