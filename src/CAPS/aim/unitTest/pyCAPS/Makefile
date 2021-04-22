#
test:
	python --version
	$(VALGRIND_COMMAND) python -m unittest discover

clean:
	rm -rf __pycache__
	rm -f *.pyc

cleanall: clean
	rm -rf workDir_*
