all:
	@cd lib && echo "Building lib" && $(MAKE) TOP=lib/ --no-print-directory
	@cd drivers && echo "Building drivers" && $(MAKE) TOP=drivers/ --no-print-directory
	@cd servers && echo "Building servers" && $(MAKE) TOP=servers/ --no-print-directory
	@cd user && echo "Building user programs" && $(MAKE) TOP=user/ --no-print-directory
	@cd kern && echo "Building kern" && $(MAKE) TOP=kern/ --no-print-directory
	-@cscope -R -b

clean:
	@cd lib && $(MAKE) clean --no-print-directory --no-builtin-rules
	@cd drivers && $(MAKE) clean --no-print-directory --no-builtin-rules
	@cd servers && $(MAKE) clean --no-print-directory --no-builtin-rules
	@cd user && $(MAKE) clean --no-print-directory --no-builtin-rules
	@cd kern && $(MAKE) clean --no-print-directory --no-builtin-rules
	-@rm -rf bin

upload: all
	@cd kern && make upload

tags:
	@cscope -R -b
