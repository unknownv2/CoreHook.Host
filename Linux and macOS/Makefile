UNAME := $(shell uname)


CXX = clang++
CXX_FLAGS =  -Wall -Wno-null-conversion -std=c++11 -g -O3 -fno-omit-frame-pointer -fms-extensions -fwrapv -ferror-limit=4096 -Werror -Wno-unused-private-field -Wno-unused-variable -Wno-tautological-compare -Wno-constant-logical-operand -Wno-pragma-pack -Wno-unknown-warning-option -Wno-invalid-offsetof -Wno-incompatible-ms-struct -fsigned-char -fPIC
CXX_DEFINES = -DAMD64 -DBIT64=1 -DDISABLE_CONTRACTS -DNDEBUG -DPLATFORM_UNIX=1 -DURTBLDENV_FRIENDLY=Retail -D_AMD64_ -D_WIN64

.PHONY: linux macos

all:
ifeq ($(UNAME),Linux)
	$(MAKE) linux
endif
ifeq ($(UNAME),Darwin)
	$(MAKE) macos
endif


macos:
	$(CXX) $(CXX_FLAGS) $(CXX_DEFINES) coreruncommon.cpp -I. -fstack-protector -shared -o libcorerun.dylib

linux:
	$(CXX)-3.9 $(CXX_FLAGS) $(CXX_DEFINES) -DLINUX64 coreruncommon.cpp -I. -fstack-protector-strong -shared -o libcorerun.so -lpthread

clean:
ifeq ($(UNAME),Linux)
	rm -f libcorerun.so
endif
ifeq ($(UNAME),Darwin)
	rm -f libcorerun.dylib
endif
