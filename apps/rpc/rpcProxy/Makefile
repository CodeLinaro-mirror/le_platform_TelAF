TARGETS := $(MAKECMDGOALS)

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -t $@ tafRpcProxy.adef -C "-O2" -X "-O2"

.PHONY: clean
clean:
	rm -rf _build_* *.update
