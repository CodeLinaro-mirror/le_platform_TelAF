
SDK_HOME:=$(sdk_rootfs)/..

ReportReaderBypass.so: ReportReaderBypass.cpp Makefile
	g++ $(CXXFLAGS) -shared -fPIC -o $@ $< -I$(SDK_HOME)/simulation/libs/loc -I$(SDK_HOME)/rootfs/include -ldl

clean:
	@rm -rf *.so
