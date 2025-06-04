CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -fprofile-arcs -ftest-coverage -g
CLIENTFLAG = -D_POSIX_C_SOURCE=200112L
TARGETS=drinks_bar atom_supplier molecule_requester

all: $(TARGETS)

drinks_bar: drinks_bar.c
	$(CC) $(CFLAGS) -o drinks_bar drinks_bar.c

atom_supplier: atom_supplier.c
	$(CC) $(CFLAGS) $(CLIENTFLAG) -o atom_supplier atom_supplier.c

molecule_requester: molecule_requester.c
	$(CC) $(CFLAGS) $(CLIENTFLAG) -o molecule_requester molecule_requester.c

clean:
	rm -f $(TARGETS) *.gcda *.gcno *.gcov coverage.info
	rm -rf coverage_html

# Run coverage tools after executing your tests or programs
coverage: $(TARGETS)
	./drinks_bar -T 5555 -U 7777 -f file.txt -t 10 -c 100 -o 100 -h 100 || true
	./atom_supplier -h 127.0.0.1 -p 5555 || true
	./molecule_requester -h 127.0.0.1 -p 7777 || true
	gcov *.c

# Optional: HTML report with lcov/genhtml
coverage-html:
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage_html
	@echo "Open coverage_html/index.html in browser"

coverage-clean:
	rm -f *.gcda *.gcno *.gcov coverage.info
	rm -rf coverage_html
