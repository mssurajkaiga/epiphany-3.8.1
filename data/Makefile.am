SUBDIRS = pages

@INTLTOOL_SERVER_RULE@
@INTLTOOL_DESKTOP_RULE@

desktop_in_files = epiphany.desktop.in.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)

convertdir = $(datarootdir)/GConf/gsettings
convert_DATA = epiphany.convert

gsettings_ENUM_NAMESPACE = org.gnome.Epiphany
gsettings_ENUM_FILES = $(top_srcdir)/lib/ephy-prefs.h

gsettings_SCHEMAS = org.gnome.epiphany.gschema.xml
@GSETTINGS_RULES@

aboutdir = $(pkgdatadir)
about_DATA = epiphany.xhtml

aboutdialogdir = $(pkgdatadir)
aboutdialog_DATA = about.ini

mimepermissiondir = $(pkgdatadir)
mimepermission_DATA = mime-types-permissions.xml

xsldir = $(pkgdatadir)
xsl_DATA = \
	epiphany-bookmarks-html.xsl

# Dbus service file
servicedir = $(datadir)/dbus-1/services
service_in_files = org.gnome.Epiphany.service.in
service_DATA = $(service_in_files:.service.in=.service)
 
# Rule to make the service file with bindir expanded
$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@bindir\@|$(bindir)|" $< > $@

# Default bookmarks
# We don't put translations in the resulting rdf since the code can get
# the translations via gettext (although it can also get translations
# from the rdf if they are there and not available via gettext)
default_bookmarksdir = $(pkgdatadir)
default_bookmarks_in_files = default-bookmarks.rdf.in
default_bookmarks_DATA = $(default_bookmarks_in_files:.rdf.in=.rdf)
$(default_bookmarks_DATA): $(default_bookmarks_in_files) Makefile $(INTLTOOL_MERGE)
	LC_ALL=C $(INTLTOOL_MERGE) -x -u /tmp $< $@.tmp
	sed -e "s|\@pkgdatadir\@|$(pkgdatadir)|g" $@.tmp > $@ || rm -f $@.tmp
	rm -f $@.tmp

iconsdir = $(pkgdatadir)/icons
dist_icons_DATA =         \
    missing-thumbnail.png \
    thumbnail-frame.png   \
    incognito.png

EXTRA_DIST = \
	$(aboutdialog_DATA)		\
	$(service_in_files)		\
	$(desktop_in_files)		\
	$(about_DATA)			\
	$(mimepermission_DATA)		\
	$(xsl_DATA)			\
	$(service_DATA)			\
	$(default_bookmarks_in_files)	\
	$(gsettings_SCHEMAS)		\
	$(convert_DATA)			\
	epiphany-service.xml		\
	check-mime.py

DISTCLEANFILES = \
	$(service_DATA)			\
	$(desktop_DATA)			\
	$(default_bookmarks_DATA)
