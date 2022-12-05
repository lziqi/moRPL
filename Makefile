MAIN  =  main

#
#  choose  a  compiler
#
CC  =  mpic++
CC_DEP  =  c++

#
#  choose  DEP  =  -M  for  cc  and  -MM  for  gcc
#
#
DEP  =  -M
#DEP  =  -MM


# prpl
PRPL_INCLUDE  =  ./

# mpi
MPI_INCLUDE = /usr/lib/x86_64-linux-gnu/openmpi/include
MPI_LIB = /usr/lib/x86_64-linux-gnu/openmpi/lib

# gdal
GDAL_INCLUDE  =  /usr/include/gdal
GDAL_LIB  =  /usr/lib

# opencl
OPENCL_INCLUDE = /usr/include/CL
OPENCL_LIB = /usr/lib/x86_64-linux-gnu

TIFF_LIB = /home/lziqi/Code/software/tiff/lib



CPPFLAGS=  -O  -I./  -I$(GDAL_INCLUDE)  -I$(PRPL_INCLUDE) -I$(MPI_INCLUDE)   -I$(OPENCL_INCLUDE)

CPPLIBS  =  -L$(GDAL_LIB)  -L$(MPI_LIB) -L$(OPENCL_LIB) -L$(TIFF_LIB) -lOpenCL -lgdal -ltiff -std=c++11

# 库文件的src cpp
PRPL_SRC  = pgtiol-dataset.cpp\
	       pgtiol-gTiffMetaData.cpp\
               pgtiol-gTiffData.cpp \
	       pgtiol-gTiffManager.cpp \
               prpl-basicTypes.cpp \
               prpl-neighborhood.cpp  \
               prpl-cellspaceInfo.cpp \
               prpl-cellspaceGeoinfo.cpp  \
               prpl-cellspace.cpp  \
               prpl-cellStream.cpp \
               prpl-subCellspaceInfo.cpp  \
               prpl-subCellspace.cpp  \
               prpl-layer.cpp  \
               prpl-transition.cpp  \
               prpl-smplDcmp.cpp \
               prpl-ownershipMap.cpp \
               prpl-exchangeMap.cpp \
               prpl-process.cpp \
               prpl-transferInfo.cpp \
               prpl-dataManager.cpp  \
               morpl-Error.cpp \
               morpl-OperatorDevice.cpp \
               morpl-openclManager.cpp \
               morpl-node.cpp \
               morpl-Test.cpp \
               demoTrans.cpp \

#                aspectTrans.cpp 
MAIN_SRC  =  main.cpp  # pAspect.cpp

# 所有的cpp
SRC  =  ${PRPL_SRC}  ${MAIN_SRC}

# prpl的头文件
PRPL_H  =  ${PRPL_SRC:.cpp=.h}

OBJS  =  ${SRC:.cpp=.o}
DEPS  =  ${SRC:.cpp=.d}

$(MAIN)    :   $(OBJS)
	$(CC)   $(CPPFLAGS)   $(OBJS)   -o    $(MAIN)    $(CPPLIBS)
  
include  Makefile.inc

depend  :  $(SRC)  $(PRPL_H)
	$(CC_DEP)  $(DEP)  $(CPPFLAGS)  $(SRC)  >  Makefile.inc

clean  :
	-rm  *.o
clean_all  :
	-rm  *.o  $(MAIN)

#include  $(DEPS)


#.SUFFIXES  :  .o  .c  .d

#%.d:  %.c
#       $(SHELL)  -ec  '$(CC)  -MM  $(CPPFLAGS)  $<  \
#       |  sed  '\''s/$*\.o/&  $@/g'\''  >  $@'
#c.d:
#       $(SHELL)  -ec  '$(CC)  -MM  $(CPPFLAGS)  $<  \
#       |  sed  '\''s/$*\.o/&  $@/g'\''  >  $@'

