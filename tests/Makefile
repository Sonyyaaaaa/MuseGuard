CC = cc
CPPFLAGS = -I../firmware/Devices -I../firmware/App
CFLAGS = -std=c11 -O0 -g -Wall -Wextra -Werror
SOURCES = test_museguard.c ../firmware/Devices/mpu6050.c ../firmware/Devices/simple_sensors.c ../firmware/App/museguard.c
LAB3_SOURCES = test_lab3.c ../firmware/App/event_store.c ../firmware/App/exhibit.c ../firmware/App/cadence.c
BUILD = build/$(shell uname -s)-$(shell uname -m)
COVERAGE = coverage/$(shell uname -s)-$(shell uname -m)
.PHONY: test clean coverage
test: $(BUILD)/test_museguard $(BUILD)/test_lab3
	./$(BUILD)/test_museguard
	./$(BUILD)/test_lab3
$(BUILD)/test_museguard: $(SOURCES) $(wildcard fixtures/*.h ../firmware/Devices/*.h ../firmware/App/*.h)
	mkdir -p $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@
clean:
	rm -rf $(BUILD)

coverage:
	mkdir -p $(COVERAGE)
	rm -f $(COVERAGE)/*.gcda
	$(CC) $(CPPFLAGS) $(CFLAGS) --coverage $(SOURCES) -o $(COVERAGE)/test_museguard
	./$(COVERAGE)/test_museguard
	$(CC) $(CPPFLAGS) $(CFLAGS) --coverage $(LAB3_SOURCES) -o $(COVERAGE)/test_lab3
	./$(COVERAGE)/test_lab3
	$(GCOV) -b $(COVERAGE)/*.gcno
	mv *.gcov $(COVERAGE)/
GCOV ?= gcov

$(BUILD)/test_lab3: $(LAB3_SOURCES) $(wildcard ../firmware/App/*.h)
	mkdir -p $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LAB3_SOURCES) -o $@
