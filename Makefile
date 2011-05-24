all:
	cd lib && $(MAKE)
	cd drivers && $(MAKE)
	cd kern && $(MAKE) upload

clean:
	cd lib && $(MAKE) clean
	cd drivers && $(MAKE) clean
	cd kern && $(MAKE) clean
	-rm -rf bin
