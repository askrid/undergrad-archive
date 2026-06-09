STUDENTID  = 202017316
SUBMITFILE = $(STUDENTID)_JoonwooChoi_assign4
TARBALL    = $(SUBMITFILE).tar.gz

SRC = sr_router.c sr_arpcache.c
PDF = readme.pdf

.PHONY: submit clean check

submit: $(TARBALL)

$(TARBALL): $(SRC) $(PDF)
	rm -rf $(SUBMITFILE)
	mkdir $(SUBMITFILE)
	cp $(SRC) $(PDF) $(SUBMITFILE)/
	tar zcf $(TARBALL) $(SUBMITFILE)
	rm -rf $(SUBMITFILE)
	@echo
	@echo "built: $(TARBALL)"
	@tar tzf $(TARBALL)

check: $(TARBALL)
	@echo "archive contents:"
	@tar tzf $(TARBALL)

clean:
	rm -rf $(SUBMITFILE) $(TARBALL)

