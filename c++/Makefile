bin := bin/
tmp := build/

progs := bridge chat

src := $(filter-out main_%.cpp, $(wildcard *.cpp))
obj := $(src:%.cpp=$(tmp)%.oxx)
out := $(progs:%=$(bin)%)

CXXFLAGS := -Wall -Wextra -Werror -O0 -g -std=gnu++17 -ffunction-sections -fdata-sections
LDFLAGS := -Wall -Wextra -Werror -O0 -g -Wl,--gc-sections
libs := pthread zmq

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
