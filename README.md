#NDPluginPipeWriter
EPICS AreaDetector file plugin which writes to a Unix pipe that feeds Tim 
Madden's MPI code that compresses images and writes to APS Sector 8 (XPCS) 
IMM file format 
(https://subversion.xray.aps.anl.gov/detpool/FCCDATCA/trunk/detectorMPI/).

In order to install this plugin, the following modifications were made in
areaDetector, ADCore, & ioc

In areaDetector/configure/RELEASE.local add:
```
NDPLUGINPIPEWRITER=$(AREA_DETECTOR)/NDPluginPipeWriter
```

To areaDetector/Makefile add:
```
DIRS := $(DIRS) NDPluginPipeWriter
$(NDPluginPipeWriter)_DEPEND_DIRS += $(ADCore)
```

in $(ADCore)/ADApp/commonDriverMakefile add:
```
ifdef NDPLUGINPIPEWRITER
  PROD_LIBS             += NDPluginPipeWriter
  $(PROD_NAME)_DBD      += NDPluginPipeWriter.dbd
endif
```
