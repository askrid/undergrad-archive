STUDENTID  = 202017316
SUBMITFILE = $(STUDENTID)_JoonwooChoi_assign3
TARBALL    = $(SUBMITFILE).tar.gz

SRC = etcp/src/tcp_in.c etcp/src/tcp_out.c etcp/src/timer.c
PDF = readme.pdf

.PHONY: submit pdf clean check

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

