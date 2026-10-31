# Builds the tcc-wasm kit with TinyCC. See README.md.
TCC ?= tcc
CC ?= cc
CFLAGS = -std=c99 -Wall -Werror -Isrc
CLANG_FLAGS = -std=c99 -Wall -Wextra -Wswitch-enum -Werror -fsyntax-only -Isrc
FRONT = src/front/base.c src/front/lexer.c src/front/parser.c src/front/fmt.c src/front/front.c src/front/eval.c src/front/check.c src/front/search.c src/front/lower.c
TARGET = src/wasm.c
SRC = src/main.c $(FRONT) $(TARGET)
FRONT_TOOL = test/fronttool.c $(filter-out src/front/front.c src/front/lower.c,$(FRONT))
CUBICAL_TOOL = test/cubical.c $(filter-out src/front/lower.c,$(FRONT))
HEADERS = $(wildcard src/*.h src/front/*.h)

.PHONY: build check check-clang clean

build: build/langc build/fronttool

build/domain.c: domain/domain.lang gen/embed.c
	mkdir -p build
	$(TCC) -run gen/embed.c domain/domain.lang build/domain.c

build/langc: $(SRC) $(HEADERS) build/domain.c
	$(TCC) $(CFLAGS) -o build/langc $(SRC) build/domain.c

build/fronttool: $(FRONT_TOOL) $(HEADERS)
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/fronttool $(FRONT_TOOL)

build/diagnostics: test/diagnostics.c $(SRC) $(HEADERS) build/domain.c
	$(TCC) $(CFLAGS) -o build/diagnostics test/diagnostics.c $(filter-out src/main.c,$(SRC)) build/domain.c

build/cubical: $(CUBICAL_TOOL) $(HEADERS) build/domain.c
	$(TCC) $(CFLAGS) -o build/cubical $(CUBICAL_TOOL) build/domain.c

check-clang: build/domain.c
	$(CC) $(CLANG_FLAGS) $(SRC) build/domain.c
	$(CC) $(CLANG_FLAGS) $(FRONT_TOOL)
	$(CC) $(CLANG_FLAGS) test/diagnostics.c $(filter-out src/main.c,$(SRC)) build/domain.c
	$(CC) $(CLANG_FLAGS) $(CUBICAL_TOOL) build/domain.c

check: build/langc build/fronttool build/diagnostics build/cubical check-clang
	sh test/gate.sh
	sh test/review.sh
	build/fronttool
	build/diagnostics
	build/cubical

clean:
	rm -rf build
