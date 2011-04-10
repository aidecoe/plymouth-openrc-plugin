PLUGIN_NAME  := plymouth
PLUGINS_PATH := $(DESTDIR)/lib/rc/plugins

CFLAGS 	     += -fPIC -Wall
CPPFLAGS     += -DDEBUG
LDLIBS       := -leinfo -lrc
LDFLAGS      += -fPIC -shared

INSTALL      := install -D


.PHONY: install clean


$(PLUGIN_NAME).so: $(PLUGIN_NAME).o
	$(CC) $(LDFLAGS) -Wl,-soname,$@ $+ -o $@ $(LDLIBS)

clean:
	$(RM) *.o *.so

install: $(PLUGIN_NAME).so
	$(INSTALL) $(PLUGIN_NAME).so $(PLUGINS_PATH)/$(PLUGIN_NAME).so

uninstall:
	$(RM) $(PLUGINS_PATH)/$(PLUGIN_NAME).so
