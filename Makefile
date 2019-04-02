# Default action: build for debug
TARGET:=DEBUG=1

# Call Makefile-build to build for the specified target
all:
	$(MAKE) -f Makefile-build $(TARGET)

.PHONY : all release debug dll install run clean

# Build for release
release: TARGET:=DEBUG=0
release: all

# Build for debug
debug: TARGET:=DEBUG=1
debug: all

# Build dynamic library
dll: TARGET:=DEBUG=0 BUILD_DLL=1 dll
dll: all

# Install dynamic library in the system.
install: TARGET:=DEBUG=0 BUILD_DLL=1 install
install: all

# Run the application
run: TARGET:=run
run: all

# Clean all files
clean: TARGET:=clean
clean: all
