
SDK_HOME:=$(sdk_rootfs)/..

SRCS := PhoneFactoryImplStubBypass.cpp SimProfileManagerBypass.cpp MultiSimManagerImplBypass.cpp

PhoneFactoryImplStubBypass.so: $(SRCS) Makefile
	g++ $(CXXFLAGS) -shared -fPIC -o $@ $(SRCS) -I$(SDK_HOME)/simulation/libs -I$(SDK_HOME)/simulation/libs/tel -I$(SDK_HOME)/rootfs/include

clean:
	@rm -rf *.so
