TARGETS := $(MAKECMDGOALS)

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -t $@ tafRpcProxy.adef

.PHONY: clean
clean:
	rm -rf _build_* *.update
