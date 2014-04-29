SRC_LIST = $(wildcard *.cpp)
TGT_LIST = $(basename $(SRC_LIST))

CPPFLAGS = -I${BOOST_INTRUSIVE}/include -I${BOOST}/include
CXXFLAGS = -Wall -Wextra -pedantic -Wno-ignored-qualifiers -Wno-long-long -g3 -O0

.PHONY: all run clean

all: $(TGT_LIST)

%: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

run: $(TGT_LIST)
	for f in $^; do \
	    echo "===== running: $$f"; \
	    ./$$f || break; \
	done

clean:
	rm -f $(TGT_LIST)
