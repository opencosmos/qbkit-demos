bin := bin/
tmp := build/

progs := $(patsubst main_%.cpp, %, $(wildcard main_*.cpp))

src := $(filter-out main_%.cpp, $(wildcard *.cpp))
obj := $(src:%.cpp=$(tmp)%.oxx)
out := $(progs:%=$(bin)%)

O ?= 0

CXXFLAGS := -Wall -Wextra -Werror -O$(O) -g -std=gnu++17 -ffunction-sections -fdata-sections -flto
LDFLAGS := -Wall -Wextra -Werror -O$(O) -g -Wl,--gc-sections -flto
libs := zmq pthread

CXX := clang++
LD := clang++

.SECONDEXPANSION:
.SECONDARY:

.PHONY: all
all: $(out)

.PHONY: clean
clean:
	rm -f -- $(tmp)/* $(bin)/*
	rmdir -- $(tmp) $(bin)

%/:
	mkdir -p $@

$(out): $(obj) $(tmp)main_$$(@F).oxx | $(bin)
	$(LD) $(LDFLAGS) -o $@ $^ $(libs:%=-l%)

$(tmp)%.oxx: %.cpp | $(tmp)
	$(CXX) $(CXXFLAGS) -MMD -c -o $@ $<

-include $(wildcard $(tmp)*.d)
