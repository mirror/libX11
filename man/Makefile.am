# Have to fake out automake by calling our man pages "DATA" since "MANS" won't
# accept .3x as a valid man page suffix

LIBMANSUFFIX=@LIBMAN_SUFFIX@
libmandir = $(mandir)/man$(LIBMANSUFFIX)

if MANPAGES
libman_SOURCES = \
	AllPlanes.man \
	BlackPixelOfScreen.man \
	DisplayOfCCC.man \
	ImageByteOrder.man \
	IsCursorKey.man \
	XAllocClassHint.man \
	XAllocIconSize.man \
	XAllocStandardColormap.man \
	XAllocSizeHints.man \
	XAllocWMHints.man \
	XAddHost.man \
	XAllocColor.man \
	XAllowEvents.man \
	XAnyEvent.man \
	XButtonEvent.man \
	XChangeKeyboardControl.man \
	XChangeKeyboardMapping.man \
	XClientMessageEvent.man \
	XChangePointerControl.man \
	XChangeSaveSet.man \
	XChangeWindowAttributes.man \
	XCreateWindowEvent.man \
	XCirculateEvent.man \
	XCirculateRequestEvent.man \
	XClearArea.man \
	XColormapEvent.man \
	XConfigureEvent.man \
	XConfigureRequestEvent.man \
	XConfigureWindow.man \
	XCopyArea.man \
	XCreateColormap.man \
	XCreateFontCursor.man \
	XCreateFontSet.man \
	XCreateGC.man \
	XCreateIC.man \
	XInitImage.man \
	XCreateOC.man \
	XCreatePixmap.man \
	XCreateRegion.man \
	XCreateWindow.man \
	XCrossingEvent.man \
	XDestroyWindowEvent.man \
	XDefineCursor.man \
	XDestroyWindow.man \
	XDrawArc.man \
	XDrawImageString.man \
	XDrawLine.man \
	XDrawPoint.man \
	XDrawRectangle.man \
	XDrawString.man \
	XDrawText.man \
	XEmptyRegion.man \
	XrmEnumerateDatabase.man \
	XExtentsOfFontSet.man \
	XErrorEvent.man \
	XExposeEvent.man \
	XFocusChangeEvent.man \
	XFilterEvent.man \
	XFontSetExtents.man \
	XFillRectangle.man \
	XFlush.man \
	XFontsOfFontSet.man \
	XFree.man \
	XGraphicsExposeEvent.man \
	XGravityEvent.man \
	XrmGetFileDatabase.man \
	XrmGetResource.man \
	XGetVisualInfo.man \
	XGetWindowAttributes.man \
	XGetWindowProperty.man \
	XGrabButton.man \
	XGrabKey.man \
	XGrabKeyboard.man \
	XGrabPointer.man \
	XGrabServer.man \
	XIconifyWindow.man \
	XIfEvent.man \
	XrmInitialize.man \
	XInstallColormap.man \
	XAddConnectionWatch.man \
	XIntersectRegion.man \
	XInternAtom.man \
	XKeymapEvent.man \
	XListFonts.man \
	XLoadFont.man \
	XLookupKeysym.man \
	XrmMergeDatabases.man \
	XMapEvent.man \
	XMapRequestEvent.man \
	XMapWindow.man \
	XNextEvent.man \
	XNoOp.man \
	XOpenDisplay.man \
	XOpenIM.man \
	XOpenOM.man \
	XPropertyEvent.man \
	XParseGeometry.man \
	XPolygonRegion.man \
	XPutBackEvent.man \
	XPutImage.man \
	XrmPutResource.man \
	XQueryBestSize.man \
	XQueryColor.man \
	XQueryExtension.man \
	XQueryPointer.man \
	XQueryTree.man \
	XReparentEvent.man \
	XResourceManagerString.man \
	XResizeRequestEvent.man \
	XRaiseWindow.man \
	XReadBitmapFile.man \
	XRecolorCursor.man \
	XReparentWindow.man \
	XSelectionClearEvent.man \
	XSaveContext.man \
	XSelectionEvent.man \
	XSetICFocus.man \
	XSetICValues.man \
	XSelectInput.man \
	XStringListToTextProperty.man \
	XSelectionRequestEvent.man \
	XSetArcMode.man \
	XSetClipOrigin.man \
	XSetCloseDownMode.man \
	XSetCommand.man \
	XSetErrorHandler.man \
	XSendEvent.man \
	XSetFillStyle.man \
	XSetFont.man \
	XSetFontPath.man \
	XSetInputFocus.man \
	XSetLineAttributes.man \
	XSetPointerMapping.man \
	XSetScreenSaver.man \
	XSetSelectionOwner.man \
	XSetState.man \
	XSetTransientForHint.man \
	XSetTextProperty.man \
	XSetTile.man \
	XSetWMClientMachine.man \
	XSetWMColormapWindows.man \
	XSetWMIconName.man \
	XSetWMName.man \
	XSetWMProperties.man \
	XSetWMProtocols.man \
	XStoreBytes.man \
	XStoreColors.man \
	XStringToKeysym.man \
	XSupportsLocale.man \
	XSynchronize.man \
	XmbTextListToTextProperty.man \
	XTextExtents.man \
	XTextWidth.man \
	XInitThreads.man \
	XTranslateCoordinates.man \
	XrmUniqueQuark.man \
	XUnmapEvent.man \
	XUnmapWindow.man \
	XVaCreateNestedList.man \
	XVisibilityEvent.man \
	XWarpPointer.man \
	XcmsCCCOfColormap.man \
	XcmsAllocColor.man \
	XcmsConvertColors.man \
	XcmsColor.man \
	XcmsCreateCCC.man \
	XcmsDefaultCCC.man \
	XcmsCIELabQueryMaxC.man \
	XcmsCIELuvQueryMaxC.man \
	XcmsQueryBlack.man \
	XcmsQueryColor.man \
	XcmsStoreColor.man \
	XcmsSetWhitePoint.man \
	XcmsTekHVCQueryMaxC.man \
	XmbDrawImageString.man \
	XmbDrawString.man \
	XmbDrawText.man \
	XmbLookupString.man \
	XmbResetIC.man \
	XmbTextEscapement.man \
	XmbTextExtents.man \
	XmbTextPerCharExtents.man
endif

libman_DATA=$(libman_SOURCES:man=@LIBMAN_SUFFIX@)

EXTRA_DIST = $(libman_SOURCES)

CLEANFILES = $(libman_DATA)

include $(top_srcdir)/cpprules.in

SUFFIXES += .$(LIBMANSUFFIX) .man

XORGRELSTRING = @PACKAGE_STRING@
  XORGMANNAME = X Version 11

MANDEFS = -D__libmansuffix__=$(LIBMANSUFFIX) \
	-D__xorgversion__="\"$(XORGRELSTRING)\" \"$(XORGMANNAME)\""

CPP_MAN_FLAGS = $(MANDEFS) $(EXTRAMANDEFS)

.man.$(LIBMANSUFFIX):
	$(RAWCPP) $(RAWCPPFLAGS) $(CPP_MAN_FLAGS) < $< | $(CPP_SED_MAGIC) > $@
