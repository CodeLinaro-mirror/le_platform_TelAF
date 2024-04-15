ifneq ($(wildcard $(PKG_CONFIG_SYSROOT_DIR)/usr/lib/libtelux_audio.so),)
    export TELUX_AUDIO_SUPPORTED := y
else
    export TELUX_AUDIO_SUPPORTED := n
endif

ifneq ($(wildcard $(PKG_CONFIG_SYSROOT_DIR)/usr/lib/libtelux_loc.so),)
    export TELUX_LOCATION_SUPPORTED := y
else
    export TELUX_LOCATION_SUPPORTED := n
endif

ifneq ($(wildcard $(PKG_CONFIG_SYSROOT_DIR)/usr/lib/libtelux_wlan.so),)
    export TELUX_WLAN_SUPPORTED := y
else
    export TELUX_WLAN_SUPPORTED := n
endif

TELUX_PC := $(PKG_CONFIG_SYSROOT_DIR)/usr/lib/pkgconfig/telux.pc
TELUX_VERSION := $(shell grep '^Version:' $(TELUX_PC) | awk '{print $$2}')
$(info TELUX_VERSION: $(TELUX_VERSION))
TELUX_VERSION_AUDIO_MULTI_FORMAT_PB_SUPPORTED := 1.66.2
DEFAULT_GT_OR_EQ := $(shell awk -v ver1=$(TELUX_VERSION) -v ver2=$(TELUX_VERSION_AUDIO_MULTI_FORMAT_PB_SUPPORTED) 'BEGIN { if (ver1 >= ver2) print "y"; else print "n" }')

ifeq ($(DEFAULT_GT_OR_EQ),y)
    export TELUX_AUDIO_MULTI_FORMAT_PB_SUPPORTED := y
else
    export TELUX_AUDIO_MULTI_FORMAT_PB_SUPPORTED := n
endif
