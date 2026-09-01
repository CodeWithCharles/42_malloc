#!/usr/bin/make -f
# ---------------------------------------------------------------------------- #
#                                   ft_malloc                                  #
# ---------------------------------------------------------------------------- #
all: all-ft-malloc
bonus: all

include Makefile.vars Makefile.msg

# -------------------------------- Main build -------------------------------- #
all-ft-malloc: $(LIBS_MAKE_RULE) $(NAME_PATH)

$(NAME_PATH): $(OBJS)
	$(call qcmd,$(MKDIR) -p $(@D))
	$(call bcmd,ld,$(NAME),$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LD_LIBS))
	$(call qcmd,ln -sf $(NAME) $(LINK_PATH))
	$(call omsg,$(NAME) + symlink $(LINK))

$(OBJDIR)/%.c.o: $(SRCDIR)/%.c
	$(call qcmd,$(MKDIR) -p $(@D))
	$(call bcmd,cc,$<,$(CC) -c $(CFLAGS) -o $@ $<)

$(OBJDIR)/%.S.o: $(SRCDIR)/%.S
	$(call qcmd,$(MKDIR) -p $(@D))
	$(call bcmd,as,$<,$(CC) $(CFLAGS) -c $< -o $@)

# ------------------------------- Checks config ------------------------------ #
Makefile.cfg:
	$(call emsg,Makefile.cfg missing - use ./configure.sh)
	@exit 1

# ---------------------------------- Cleanup --------------------------------- #
clean:
	$(call rmsg,Deleting objects ($(OBJDIR)))
	$(call qcmd,$(RM) -rf $(OBJDIR))

fclean: clean
	$(call rmsg,Deleting $(NAME) and symlink)
	$(call qcmd,$(RM) -f $(NAME_PATH) $(LINK_PATH))

cleanlibs: $(LIBS_CLEAN_RULE)
fcleanlibs: fclean $(LIBS_FCLEAN_RULE)

mrproper: fclean fcleanlibs
	$(call rmsg, Deleting Makefile.cfg)
	$(call qcmd,$(RM) -f Makefile.cfg)

re: fclean all

# ----------------------------------- Tests ---------------------------------- #
test:
	$(call qcmd,$(MAKE) -C tests)

-include $(DEPS)

.PHONY: all bonus all-ft-malloc clean fclean cleanlibs \
fcleanlibs mrproper re test