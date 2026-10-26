.PHONY: all test install clean
all:
	sh build.sh
test: all
	python3 tests/run_tests.py ./bin/linux-x64/uma
	python3 tests/run_tests.py ./bin/linux-x64/uma bytecode
	python3 tests/run_tests.py ./bin/linux-x64/uma executable
	python3 tests/test_compiler.py ./bin/linux-x64/uma
	python3 tests/test_v3.py ./bin/linux-x64/uma
	python3 tests/test_v31.py ./bin/linux-x64/uma
	python3 tests/test_lsp.py ./bin/linux-x64/uma
	./bin/linux-x64/uma test tests/stdlib
	node --test vscode-extension/test/*.test.js
install:
	sh instalar-linux.sh
clean:
	rm -f bin/linux-x64/uma
