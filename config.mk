
TELUX_PC := $(PKG_CONFIG_SYSROOT_DIR)/usr/lib/pkgconfig/telux.pc
TELUX_VERSION := $(shell find $(TELUX_PC) 2> /dev/null | xargs -r grep '^Version:' | awk '{print $$2}')
ifneq ($(TELUX_VERSION),)
    $(info TELUX_VERSION: $(TELUX_VERSION))
endif
