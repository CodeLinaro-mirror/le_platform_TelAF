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
