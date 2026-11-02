# Builds the tcc-wasm kit with TinyCC. See README.md.
TCC ?= tcc
CC ?= cc
CFLAGS = -std=c99 -Wall -Werror -Isrc
CLANG_FLAGS = -std=c99 -Wall -Wextra -Wswitch-enum -Werror -fsyntax-only -Isrc
FRONT = src/front/base.c src/front/json.c src/front/lexer.c src/front/parser.c src/front/fmt.c src/front/front.c src/front/eval.c src/front/check.c src/front/search.c src/front/lower.c
TARGET = src/wasm.c
SRC = src/main.c $(FRONT) $(TARGET) src/engine.c src/codes.c src/mcp.c src/front/jsonin.c
FRONT_TOOL = test/fronttool.c $(filter-out src/front/front.c src/front/lower.c,$(FRONT))
CUBICAL_TOOL = test/cubical.c $(filter-out src/front/lower.c,$(FRONT))
MCP_TEST = test/mcp.c src/mcp.c src/front/jsonin.c src/front/base.c
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

build/sim: test/sim.c $(SRC) $(HEADERS) build/domain.c
	$(TCC) $(CFLAGS) -o build/sim test/sim.c $(filter-out src/main.c,$(SRC)) build/domain.c

build/report: test/report.c $(SRC) $(HEADERS) build/domain.c
	$(TCC) $(CFLAGS) -o build/report test/report.c $(filter-out src/main.c,$(SRC)) build/domain.c

build/engine: test/engine.c $(SRC) $(HEADERS) build/domain.c
	$(TCC) $(CFLAGS) -o build/engine test/engine.c $(filter-out src/main.c,$(SRC)) build/domain.c

build/skill-test: test/skill.c
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/skill-test test/skill.c

build/verbs-test: test/verbs.c $(SRC) $(HEADERS) build/domain.c
	$(TCC) $(CFLAGS) -o build/verbs-test test/verbs.c $(filter-out src/main.c,$(SRC)) build/domain.c

build/mcp-test: $(MCP_TEST) $(HEADERS)
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/mcp-test $(MCP_TEST)

build/readme-test: test/readme.c
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/readme-test test/readme.c

build/tokcount: test/tokcount.c
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/tokcount test/tokcount.c

build/tokcount-io: test/tokcount-io.c
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/tokcount-io test/tokcount-io.c

check-clang: build/domain.c
	$(CC) $(CLANG_FLAGS) $(SRC) build/domain.c
	$(CC) $(CLANG_FLAGS) $(FRONT_TOOL)
	$(CC) $(CLANG_FLAGS) test/diagnostics.c $(filter-out src/main.c,$(SRC)) build/domain.c
	$(CC) $(CLANG_FLAGS) $(CUBICAL_TOOL) build/domain.c
	$(CC) $(CLANG_FLAGS) test/sim.c $(filter-out src/main.c,$(SRC)) build/domain.c
	$(CC) $(CLANG_FLAGS) test/report.c $(filter-out src/main.c,$(SRC)) build/domain.c
	$(CC) $(CLANG_FLAGS) test/engine.c $(filter-out src/main.c,$(SRC)) build/domain.c
	$(CC) $(CLANG_FLAGS) test/skill.c
	$(CC) $(CLANG_FLAGS) test/verbs.c $(filter-out src/main.c,$(SRC)) build/domain.c
	$(CC) $(CLANG_FLAGS) $(MCP_TEST)
	$(CC) $(CLANG_FLAGS) test/readme.c
	$(CC) $(CLANG_FLAGS) test/tokcount.c
	$(CC) $(CLANG_FLAGS) test/tokcount-io.c

check: build/langc build/fronttool build/diagnostics build/cubical build/sim build/report build/engine build/skill-test build/verbs-test build/mcp-test build/readme-test build/tokcount build/tokcount-io check-clang
	sh test/gate.sh
	sh test/review.sh
	build/fronttool
	build/diagnostics
	build/cubical
	build/sim
	build/report
	build/skill-test
	build/verbs-test
	build/mcp-test
	build/readme-test
	build/tokcount --check docs/TOKEN-BASELINE.md examples/inventory-escrow.merge test/baseline/inventory.py test/baseline/inventory.rs
	build/tokcount-io

clean:
	rm -rf build
