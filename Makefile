.DEFAULT_GOAL := build

BUILD_DIR ?= build/

.PHONY: build
build: clean
	mkdir -p ${BUILD_DIR}
	cd ${BUILD_DIR}; cmake ..; make

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)