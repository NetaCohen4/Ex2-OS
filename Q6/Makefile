CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -fprofile-arcs -ftest-coverage -g
CLIENTFLAG = -D_POSIX_C_SOURCE=200112L
TARGETS = drinks_bar atom_supplier molecule_requester

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


# after running manualy produce gcov report 
coverage-report:
	gcov *.c

# after running manualy produce lcov/genhtml report with html
coverage-html: coverage-report
	lcov --capture --directory . --output-file coverage.info --quiet
	genhtml coverage.info --output-directory coverage_html --quiet
	@echo "Open coverage_html/index.html in browser"

coverage-clean:
	rm -f *.gcda *.gcno *.gcov coverage.info
	rm -rf coverage_html