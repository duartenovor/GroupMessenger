#------------Execute others Makefiles -> action: ALL-----------------
all: 
	$(MAKE) all -C Client ;\
	$(MAKE) all -C Server

.PHONY: $(SUBDIRS)

#------------Execute others Makefiles -> action: CLEAN-----------------
clean: 
	$(MAKE) clean -C Client ;\
	$(MAKE) clean -C Server

.PHONY: $(SUBDIRS)

#------------Execute others Makefiles -> action: TRANSFER-----------------
transfer_client:
	$(MAKE) transfer_client -C Client

#------------Execute others Makefiles -> action: TRANSFER-----------------
transfer_server:
	$(MAKE) transfer_server -C Server

#------------Execute others Makefiles -> action: TRANSFER-----------------
transfer_both:
	$(MAKE) transfer_client -C Client
	$(MAKE) transfer_server -C Server