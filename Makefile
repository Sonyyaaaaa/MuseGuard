IMAGE ?= stm32c0-wokwi-build:local

.PHONY: image build clean test check coverage toolchain

image:
	docker build -t $(IMAGE) docker

build: image
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace \
		--user "$(shell id -u):$(shell id -g)" $(IMAGE) \
		make -C firmware all

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
