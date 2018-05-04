BIN	 = video
all: $(BIN)

Q = @
ifeq ("$(origin V)", "command line")
ifeq ($(V),1)
Q =
endif
endif

ifeq ($(origin CC),default)
CC	 = ../../prebuilts/toolschain/usr/bin/arm-linux-gcc
endif
ifeq ($(origin CXX),default)
CXX	 = ../../prebuilts/toolschain/usr/bin/arm-linux-g++
endif

INCS	 = -I./../../out/system/include/ -I./ -Iav_wrapper/ -I./watermark/ -I./power/ -I./uvc/ \
           -I./../../out/system/include/sphinxbase -I./../../out/system/include/pocketsphinx   \
           -I./../../out/system/include/dpp

LIBS	 = -L$(LIBPATH) -lcam_hal \
           -lvoice_process \
           -lm -lpthread -ldl \
           -lion -lvpu -lmpp -lrkipc \
           -lswresample      \
           -lavformat -lavcodec -lavutil -lfdk-aac -lz \
           -lsalsa -lfsmanage -ldpp -lrk_backtrace -liep \
           -lrkfb -lrkrga \
           -lrs_face_recognition -L./ -lReadFace

ETCPATH  = ../../out/system/etc
RESPATH  = ../../out/system/share/minigui/res/images
LIBPATH  = ../../out/system/lib
INSTALL_BIN_PATH = ../../out/system/bin

$(ETCPATH) $(RESPATH) $(INSTALL_BIN_PATH):
	$(Q)mkdir -p $(@)

define all-cpp-files-under
$(shell find $(1) -name "*."$(2) -and -not -name ".*" )
endef

define all-subdir-cpp-files
$(call all-cpp-files-under,.,"cpp")
endef

CPPSRCS	 = $(call all-subdir-cpp-files)

TARGET_CFLAGS ?=
TARGET_CFLAGS += -Wno-multichar -Wall -DUSE_WATERMARK -DAEC_AGC_ANR_ALGORITHM
UI_RES_DIR =
DEPENDENT_FILES = ui_resolution.h

rwildcard=$(wildcard $(1)/$(2)) \
	$(foreach d,$(wildcard $(1)/*),$(call rwildcard,$(d),$(2)))

CSRCS += $(call rwildcard,av_wrapper,*.c)

CSRCS += $(wildcard *.c power/*.c ueventmonitor/*.c collision/*.c \
	parking_monitor/*.c watermark/*.c \storage/*.c screen_capture/*.c \
	uvc/*.c ui/*.c)

ifeq ($(COMPILE_PCBA_FLAG), y)
    TARGET_CFLAGS += -D_PCBA_SERVICE_
    ifeq ($(COMPILE_PCBA_NETWORK), y)
	TARGET_CFLAGS += -D_PCBA_SERVICE_NETWORK_
    endif
    ifeq ($(COMPILE_PCBA_WIFI), y)
	TARGET_CFLAGS += -D_PCBA_SERVICE_WIFI_
	TARGET_CFLAGS += -D_PCBA_SERVICE_WIFI_SSID_=$(COMPILE_PCBA_WIFI_SSID)
	TARGET_CFLAGS += -D_PCBA_SERVICE_WIFI_PASSWD_=$(COMPILE_PCBA_WIFI_PASSWD)
    endif
    ifeq ($(COMPILE_PCBA_RNDIS), y)
	TARGET_CFLAGS += -D_PCBA_SERVICE_RNDIS_
    endif
    CSRCS += $(wildcard pcba/*.c)
endif

ifeq ($(USE_FACE_DETECT),y)
TARGET_CFLAGS += -DENABLE_RS_FACE
LIBS += -lrsface
endif

ifeq ($(COMPILE_UI_TYPE), CVR)
    TARGET_CFLAGS += -DHAVE_GUI=1 -DHAVE_DISPLAY=1 -DCVR -DUSE_ADAS
    UI_RES_DIR = ui/cvr/res
    LIBS += -lminigui_ths -ljpeg -lpng -lodt
    CSRCS += $(wildcard ui/cvr/*.c adas/*.c)
else ifeq ($(COMPILE_UI_TYPE), SDV)
    TARGET_CFLAGS += -DHAVE_GUI=1 -DHAVE_DISPLAY=1 -DSDV -DDV_DEBUG -DLARGE_ION_BUFFER
    UI_RES_DIR = ui/dv/res
    LIBS += -lminigui_ths -ljpeg -lpng -ldvs -lodt
    CSRCS += $(wildcard ui/dv/*.c)
else ifeq ($(COMPILE_UI_TYPE), FACE)
    TARGET_CFLAGS += -DHAVE_GUI=0 -DHAVE_DISPLAY=1 -DSAMPLE -DUSE_ADAS -DUSE_INPUT
    UI_RES_DIR = ui/face/res
    LIBS += -lodt
    CSRCS += $(wildcard ui/face/*.c adas/*.c input/*.c)
else ifeq ($(COMPILE_UI_TYPE), SAMPLE)
    TARGET_CFLAGS += -DHAVE_GUI=0 -DHAVE_DISPLAY=1 -DSAMPLE -DUSE_ADAS -DUSE_INPUT
    UI_RES_DIR = ui/sample/res
    LIBS += -lodt
    CSRCS += $(wildcard ui/sample/*.c adas/*.c input/*.c)
else
    TARGET_CFLAGS += -DHAVE_GUI=0 -DHAVE_DISPLAY=0
endif

ifeq ($(COMPILE_UI_TRUETYPE), y)
    LIBS += -lfreetype
endif

ifeq ($(COMPILE_UI_TSLIB), y)
    LIBS += -lts
endif

ifeq ($(USE_RIL_MOUDLE),y)
    INCS += -I./ril/ -I./ril/tinyalsa/include/
    TARGET_CFLAGS += -DUSE_RIL_MOUDLE
    CSRCS += $(wildcard ril/tinyalsa/*.c ril/*.c)
endif

ifeq ($(USE_SPEECHREC),y)
    TARGET_CFLAGS += -DUSE_SPEECHREC -DSPEECHREC_ETC_FILE_PATH=$(SPEECHREC_ETC_FILE_PATH)
    LIBS += -lpocketsphinx -lsphinxbase
endif

ifneq ($(DEPENDENT_FILES),)
UI_RES_HEADER = $(UI_RES_DIR)/ui_$(strip $(Resolution)).h
ifeq ($(wildcard $(UI_RES_HEADER)),)
$(error missing $(UI_RES_HEADER))
endif
ui_resolution.h: $(UI_RES_HEADER)
	$(Q)install -C $(<) $(@)
endif

RES_FILES =
ifneq ($(UI_RES_DIR),)
RES_FILES = $(call rwildward,$(UI_RES_DIR)/res_$(strip $(Resolution)),*)
endif

ifeq ($(USE_GPS_ZS),y)
LIBS += -l_zsgps
endif

ifeq ($(USE_GPS_MOVTEXT),y)
TARGET_CFLAGS += -DUSE_GPS_MOVTEXT
endif

ifeq ($(USE_ISP_FLIP), y)
TARGET_CFLAGS += -DUSE_ISP_FLIP=1
else
TARGET_CFLAGS += -DUSE_ISP_FLIP=0
endif

COBJS	:= $(CSRCS:.c=.o)
CPPOBJS	:= $(CPPSRCS:.cpp=.o)
CDEPS = $(CSRCS:.c=.d)
CXXDEPS = $(CPPSRCS:.cpp=.d)

CXXFLAGS += -D USE_RK_V4L2_HEAD_FILES
CXXFLAGS += -DDISABLE_FF_MPP
# for libcam_hal
CXXFLAGS += -DSUPPORT_ION -DLINUX -DENABLE_ASSERT -std=c++11

CFLAGS += $(TARGET_CFLAGS)
CXXFLAGS += $(TARGET_CFLAGS)

ifeq ($(filter clean,$(MAKECMDGOALS)),)
-include $(CDEPS) $(CXXDEPS)
endif

$(CDEPS): %.d : %.c
	$(Q)$(CC) $(INCS) $(CFLAGS) $< -MT $@ -MT $*.o -MF $@ -MP -MM
	@echo CC $(@)
$(CXXDEPS): %.d : %.cpp
	$(Q)$(CXX) $(INCS) $(CXXFLAGS) $< -MT $@ -MT $*.o -MF $@ -MP -MM
	@echo CXX $(@)

$(COBJS): %.o : %.c
	$(Q)$(CC) -c $(INCS) $(CFLAGS) $< -o $@
	@echo CC $(@)
$(CPPOBJS): %.o : %.cpp
	$(Q)$(CXX) -c $(INCS) $(CXXFLAGS) $< -o $@
	@echo CXX $(@)

ifneq ($(DEPENDENT_FILES),)
$(CDEPS) $(CXXDEPS) $(COBJS) $(CPPOBJS): $(DEPENDENT_FILES)
endif
$(CDEPS) $(CXXDEPS) $(COBJS) $(CPPOBJS): Makefile

$(BIN): $(CDEPS) $(CXXDEPS) $(COBJS) $(CPPOBJS)
	$(Q)$(CXX) -o $(BIN) $(COBJS) $(CPPOBJS) $(LIBS)
	@echo "video build success!"

install: $(ETCPATH) $(RESPATH) $(INSTALL_BIN_PATH) $(BIN) $(RES_FILES)
	$(Q)install -d $(INSTALL_BIN_PATH)
	$(Q)install -C $(BIN) $(INSTALL_BIN_PATH)
	@cp libReadFace.so $(LIBPATH)/ -a
	@#copy the res to device
	$(Q)cp -rp res/watermark/* $(RESPATH)
ifeq (,$(filter $(COMPILE_UI_TYPE),SAMPLE FACE))
	$(Q)$(if $(UI_RES_DIR),\
	cp -rp $(UI_RES_DIR)/res_$(strip $(Resolution))/MiniGUI.cfg $(ETCPATH) ; \
	cp -rp $(UI_RES_DIR)/res_$(strip $(Resolution))/* $(RESPATH) ;,)
endif

ifeq ($(USE_SPEECHREC),y)
ifeq ($(SPEECHREC_ETC_FILE_PATH), \"/etc\")
	$(Q)echo "copy speechrec etc to /etc"
	$(Q)cp -rp speechrec/res/* ../../out/root/etc
endif
endif

	@echo "video install success!"

clean:
	$(Q)rm -f ui_resolution.h $(BIN) $(COBJS) $(CPPOBJS) $(CDEPS) $(CXXDEPS)
	@echo "video clean success!"

.PHONY: install clean all
