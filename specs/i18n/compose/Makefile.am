if ENABLE_SPECS
if HAVE_PERL

# Main DocBook/XML files (DOCTYPE book)
docbook = libX11-keys.xml

# Included chapters, appendix, images
chapters = $(locales:%=%.xml)

# The location where the DocBook/XML generated formats are installed
shelfdir = $(docdir)/i18n/compose

# Generate DocBook/XML output formats with or without stylesheets
# The XML source is generated and cleaned but not distributed
include $(top_srcdir)/docbook-gen-chunk.am

# Generated DocBook/XML source files using a perl script
%.xml: $(top_builddir)/nls/%/Compose
	$(AM_V_GEN)$(PERL) $(srcdir)/compose-chart.pl \
	 --locale="$(@:.xml=)" --output="$@" $<

libX11-keys.xml: $(locales:%=%.xml)
	$(AM_V_GEN)$(PERL) $(srcdir)/compose-chart.pl \
	 --index --output="$@" $(locales)

endif HAVE_PERL
endif ENABLE_SPECS

EXTRA_DIST =  compose-chart.pl
