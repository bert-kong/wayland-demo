# target file system for poky
# change lib64 in apq8098/lib64 to lib
#SYSROOT=/usr2/c_akong/workspace/LE.UM.0.0-qtivdec-dev/poky/build/tmp-glibc/sysroots/apq8098/usr/


#BASE_DIR=/local/mnt2/workspace2/LINUX/sa8155p/snapdragon-auto-gen3-lv-0-1_hlos_dev-00003.1.git/apps/apps_proc/poky/build
BASE_DIR=/local/mnt2/workspace2/LINUX/sa8155p/sa8155p-lv-0-1_hlos_dev.git-00005.3.git/apps_proc/poky/build

SYSROOT=$(BASE_DIR)/tmp-glibc/work/sa8155-agl-linux/ais/0.1-r0/recipe-sysroot

export LD_LIBRARY_PATH=$(SYSROOT)/usr/lib64

INC_PATH=-I$(SYSROOT)/usr/include -I$(SYSROOT)/usr/include/drm

# libraries binaries path

# cross-compiler tools

#TOOLS_DIR = $(BASE_DIR)/tmp-glibc/work/sa8155-agl-linux/ais/0.1-r0/recipe-sysroot-native/usr/bin/aarch64-agl-linux
#CROSS_COMPILE = aarch64-agl-linux-


# build tools 
#CC  = $(TOOLS_DIR)/$(CROSS_COMPILE)gcc
#CXX = $(TOOLS_DIR)/$(CROSS_COMPILE)g++
#LD  = $(TOOLS_DIR)/$(CROSS_COMPILE)ld
#AR  = $(TOOLS_DIR)/$(CROSS_COMPILE)ar

CROSS_COMPILE = aarch64-linux-gnu-
CC  = /usr/bin/$(CROSS_COMPILE)gcc
CXX = /usr/bin/$(CROSS_COMPILE)g++
LD  = /usr/bin/$(CROSS_COMPILE)ld
AR  = /usr/bin/$(CROSS_COMPILE)ar

# c, c++, LD flags
CFLAGS = -g -O0  $(INC_PATH)
CPPFLAGS = -std=c++11 -g -O0 -save-temps=obj $(INC_PATH)
LDFLAGS = -L$(SYSROOT)/lib -L$(SYSROOT)/usr/lib -lgbm -ldrm -lwayland-client -lcairo -lpng -lz -lpthread -lexpat -lrt

#-print-search-dirs


#  C/C++ source files
app_SRC = main.cpp
lib_SRC = swapchain.cpp circle.cpp

# C/C++ header files
INC = swapchain.hxx circle.hxx

# object files
app_OBJ=$(app_SRC:.cpp=.o) 
lib_OBJ=$(lib_SRC:.cpp=.o)

# wsi dependencies
wsi:$(app_OBJ) $(lib_OBJ)
	$(CXX) -o wsi --sysroot=$(SYSROOT) $(app_OBJ) $(lib_OBJ) $(LDFLAGS)
	chmod 775 wsi
	mv wsi /usr2/c_akong/workspace2/ANDROID/820/apq8096au-la-1-3_ap_standard_oem.git-10-09/LINUX/android/out/target/product/msm8996/testcases/bk_test/arm64/
	#$(CC) $(CFLAGS) -o $@ $^ --sysroot=$(SYSROOT) $(LDFLAGS)


# app_OBJ dependencies
app_OBJ:$(app_SRC) $(INC)
	$(CXX) -c $(CPPFLAGS) $(app_SRC)

# libraries dependencies
lib_OBJ:$(lib_SRC) $(INC)
	$(CXX) -c $(CPPFLAGS) $(lib_SRC)

#libobj.so: $(lib_OBJ)  
#        @echo linking $@  
#        $(CXX) $^ -o $@ $(LDFLAGS) -shared $(PKG_LIBS) 



.PHONY: clean
clean:
	rm -f *.o *.i *.ii *.s wsi
