build:
	cmake --build ./code/build

test:
	python3 ./tests/test_lsh.py

run:
	./code/build/lsh