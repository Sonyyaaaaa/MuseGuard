IMAGE ?= museguard-build:lab3

.PHONY: image build build-lab4 clean test check coverage toolchain

image:
	docker build -t $(IMAGE) docker

build: image
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace $(IMAGE) python3 tools/build_identity.py
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace \
		--user "$(shell id -u):$(shell id -g)" $(IMAGE) \
		make -C firmware all

build-lab4: image
	python3 lab4/build_identity.py
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace \
		--user "$(shell id -u):$(shell id -g)" $(IMAGE) \
		make -C firmware MG_FREERTOS=1 BUILD_DIR=build/lab4 TARGET=museguard-lab4 all

clean:
	$(MAKE) -C firmware clean

check:
	python3 tools/check_architecture.py
	python3 tools/check_wiring.py

test: check
	$(MAKE) -C tests test

coverage: check
	$(MAKE) -C tests coverage

toolchain: image
	docker run --rm $(IMAGE) sh -c 'arm-none-eabi-gcc --version; gcc --version; dpkg-query -W'
