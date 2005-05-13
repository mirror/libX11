SUBDIRS=util

lib_LTLIBRARIES=libX11.la

BUILT_SOURCES=ks_tables.h
CLEANFILES=ks_tables.h ks_tables_h

AM_CFLAGS=		\
                 -I$(top_srcdir)/include \
		 -I$(top_srcdir)/include/X11 \
		 -I$(top_builddir)/include \
		 -I$(top_builddir)/include/X11 \
		 $(X11_CFLAGS) \
		 $(BIGREQS_CFLAGS) \
		 $(XDMCP_CFLAGS) \
		 -DX11_DATADIR='"@X11_DATADIR@"' \
                 -D_BSD_SOURCE

libX11_la_LDFLAGS = -version-number 6:2:1 -no-undefined

libX11_la_LIBADD = $(X11_LIBS) $(BIGREQS_LIBS) $(XDMCP_LIBS)

XCMS_SRCS= \
		  AddDIC.c \
                  AddSF.c \
                  CCC.c \
                  CvColW.c \
                  CvCols.c \
                  Cv.h \
                  HVC.c \
                  HVCGcC.c \
                  HVCGcV.c \
                  HVCGcVC.c \
                  HVCMnV.c \
                  HVCMxC.c \
                  HVCMxV.c \
                  HVCMxVC.c \
                  HVCMxVs.c \
                  HVCWpAj.c \
                  IdOfPr.c \
                  LRGB.c \
                  Lab.c \
                  LabGcC.c \
                  LabGcL.c \
                  LabGcLC.c \
                  LabMnL.c \
                  LabMxC.c \
                  LabMxL.c \
                  LabMxLC.c \
                  LabWpAj.c \
                  Luv.c \
                  LuvGcC.c \
                  LuvGcL.c \
                  LuvGcLC.c \
                  LuvMnL.c \
                  LuvMxC.c \
                  LuvMxL.c \
                  LuvMxLC.c \
                  LuvWpAj.c \
                  OfCCC.c \
                  PrOfId.c \
                  QBlack.c \
                  QBlue.c \
                  QGreen.c \
                  QRed.c \
                  QWhite.c \
                  QuCol.c \
                  QuCols.c \
                  SetCCC.c \
                  SetGetCols.c \
                  StCol.c \
                  StCols.c \
                  UNDEFINED.c \
                  XRGB.c \
                  XYZ.c \
                  cmsAllCol.c \
                  cmsAllNCol.c \
                  cmsCmap.c \
                  cmsColNm.c \
                  cmsGlobls.c \
                  cmsInt.c \
                  cmsLkCol.c \
                  cmsMath.c \
                  cmsProp.c \
                  cmsTrig.c \
                  uvY.c \
                  xyY.c \
                  Xcmsint.h

XI18N_SRCS=\
		  xim_trans.c \
		  xim_trans.h \
                  ICWrap.c \
                  IMWrap.c \
                  XDefaultIMIF.c \
                  XDefaultOMIF.c \
                  XlcDL.c \
                  XlcSL.c \
                  imCallbk.c \
                  imConv.c \
                  imDefFlt.c \
                  imDefIc.c \
                  imDefIm.c \
                  imDefLkup.c \
                  imDispch.c \
                  imEvToWire.c \
                  imExten.c \
                  imImSw.c \
                  imInsClbk.c \
                  imInt.c \
                  imKStoUCS.c \
                  imLcFlt.c \
                  imLcGIc.c \
                  imLcIc.c \
                  imLcIm.c \
                  imLcLkup.c \
                  imLcPrs.c \
                  imLcSIc.c \
                  imRm.c \
                  imRmAttr.c \
                  imThaiFlt.c \
                  imThaiIc.c \
                  imThaiIm.c \
                  imTrX.c \
                  imTrans.c \
                  imTransR.c \
                  lcCT.c \
                  lcCharSet.c \
                  lcConv.c \
                  lcDB.c \
                  lcDefConv.c \
                  lcDynamic.c \
                  lcEuc.c \
                  lcFile.c \
                  lcGenConv.c \
                  lcGeneric.c \
                  lcInit.c \
                  lcJis.c \
                  lcPrTxt.c \
                  lcPubWrap.c \
                  lcPublic.c \
                  lcRM.c \
                  lcSjis.c \
                  lcStd.c \
                  lcTxtPr.c \
                  lcUTF8.c \
                  lcUTF8Load.c \
                  lcUtil.c \
                  lcWrap.c \
                  mbWMProps.c \
                  mbWrap.c \
                  omDefault.c \
                  omGeneric.c \
                  omImText.c \
                  omText.c \
                  omTextEsc.c \
                  omTextExt.c \
                  omTextPer.c \
                  omXChar.c \
                  utf8WMProps.c \
                  utf8Wrap.c \
                  wcWrap.c \
                  Xaixlcint.h \
                  XimImSw.h \
                  XimProto.h \
                  XimThai.h \
                  XimTrInt.h \
                  XimTrX.h \
                  XimTrans.h \
                  Ximint.h \
                  XimintL.h \
                  XimintP.h \
                  XlcGeneric.h \
                  XlcPubI.h \
                  XlcPublic.h \
                  Xlcint.h \
                  lcUniConv/armscii_8.h \
                  lcUniConv/ascii.h \
                  lcUniConv/big5.h \
                  lcUniConv/big5_emacs.h \
                  lcUniConv/cp1133.h \
                  lcUniConv/cp1251.h \
                  lcUniConv/cp1255.h \
                  lcUniConv/cp1256.h \
                  lcUniConv/gb2312.h \
                  lcUniConv/georgian_academy.h \
                  lcUniConv/georgian_ps.h \
                  lcUniConv/iso8859_1.h \
                  lcUniConv/iso8859_10.h \
                  lcUniConv/iso8859_11.h \
                  lcUniConv/iso8859_13.h \
                  lcUniConv/iso8859_14.h \
                  lcUniConv/iso8859_15.h \
                  lcUniConv/iso8859_16.h \
                  lcUniConv/iso8859_2.h \
                  lcUniConv/iso8859_3.h \
                  lcUniConv/iso8859_4.h \
                  lcUniConv/iso8859_5.h \
                  lcUniConv/iso8859_6.h \
                  lcUniConv/iso8859_7.h \
                  lcUniConv/iso8859_8.h \
                  lcUniConv/iso8859_9.h \
                  lcUniConv/iso8859_9e.h \
                  lcUniConv/jisx0201.h \
                  lcUniConv/jisx0208.h \
                  lcUniConv/jisx0212.h \
                  lcUniConv/koi8_c.h \
                  lcUniConv/koi8_r.h \
                  lcUniConv/koi8_u.h \
                  lcUniConv/ksc5601.h \
                  lcUniConv/mulelao.h \
                  lcUniConv/tatar_cyr.h \
                  lcUniConv/tcvn.h \
                  lcUniConv/tis620.h \
                  lcUniConv/ucs2be.h \
                  lcUniConv/utf8.h \
                  lcUniConv/viscii.h

X11_SRCS= \
                  AllCells.c \
                  AllPlanes.c \
                  AutoRep.c \
                  BdrWidth.c \
                  Border.c \
                  ChAccCon.c \
                  ChCmap.c \
                  ChGC.c \
                  ChKeyCon.c \
                  ChProp.c \
                  ChWAttrs.c \
                  ChkIfEv.c \
                  ChkMaskEv.c \
                  ChkTypEv.c \
                  ChkTypWEv.c \
                  ChkWinEv.c \
                  CirWin.c \
                  CirWinDn.c \
                  CirWinUp.c \
                  ClDisplay.c \
                  Context.c \
                  CopyCmap.c \
                  CopyGC.c \
                  CrBFData.c \
                  CrCmap.c \
                  CrCursor.c \
                  CrGC.c \
                  CrGlCur.c \
                  CrPFBData.c \
                  ChSaveSet.c \
                  CrWindow.c \
                  Cursor.c \
                  Depths.c \
                  DisName.c \
                  DrArc.c \
                  DrArcs.c \
                  DrLines.c \
                  DrPoints.c \
                  DrRect.c \
                  DrRects.c \
                  DrSegs.c \
                  ErrDes.c \
                  ErrHndlr.c \
                  EvToWire.c \
                  FSSaver.c \
                  FSWrap.c \
                  FetchName.c \
                  FillArc.c \
                  FillArcs.c \
                  FillRct.c \
                  FillRcts.c \
                  FilterEv.c \
                  Flush.c \
                  Font.c \
                  FontInfo.c \
                  FontNames.c \
                  FreeEData.c \
                  FreeGC.c \
                  GCMisc.c \
                  Geom.c \
                  GetAtomNm.c \
                  GetColor.c \
                  GetDflt.c \
                  GetFPath.c \
                  GetFProp.c \
                  GetGCVals.c \
                  GetGeom.c \
                  GetHColor.c \
                  GetHints.c \
                  GetIFocus.c \
                  GetImage.c \
                  GetKCnt.c \
                  GetMoEv.c \
                  GetNrmHint.c \
                  GetPCnt.c \
                  GetPntMap.c \
                  GetProp.c \
                  GetRGBCMap.c \
                  GetSOwner.c \
                  GetSSaver.c \
                  GetStCmap.c \
                  GetTxtProp.c \
                  GetWAttrs.c \
                  GetWMCMapW.c \
                  GetWMProto.c \
                  GrKeybd.c \
                  GrPointer.c \
                  Host.c \
                  Iconify.c \
                  IfEvent.c \
                  ImText.c \
                  ImText16.c \
                  ImUtil.c \
                  ImUtil.h \
                  InitExt.c \
                  IntAtom.c \
                  KeyBind.c \
                  KeysymStr.c \
                  LiHosts.c \
                  LiICmaps.c \
                  LiProps.c \
                  ListExt.c \
                  LoadFont.c \
                  LookupCol.c \
                  LowerWin.c \
                  Macros.c \
                  MapRaised.c \
                  MaskEvent.c \
                  Misc.c \
                  ModMap.c \
                  NextEvent.c \
                  OCWrap.c \
                  OMWrap.c \
                  OpenDis.c \
                  ParseCmd.c \
                  ParseCol.c \
                  ParseGeom.c \
                  PeekEvent.c \
                  PeekIfEv.c \
                  Pending.c \
                  PixFormats.c \
                  PmapBord.c \
                  PolyReg.c \
                  PolyTxt.c \
                  PolyTxt16.c \
                  PropAlloc.c \
                  PutBEvent.c \
                  PutImage.c \
                  QuBest.c \
                  QuColor.c \
                  QuColors.c \
                  QuCurShp.c \
                  QuExt.c \
                  QuKeybd.c \
                  QuPntr.c \
                  QuStipShp.c \
                  QuTextE16.c \
                  QuTextExt.c \
                  QuTileShp.c \
                  QuTree.c \
                  Quarks.c \
                  RaiseWin.c \
                  RdBitF.c \
                  RecolorC.c \
                  ReconfWM.c \
                  ReconfWin.c \
                  Region.c \
                  RegstFlt.c \
                  RestackWs.c \
                  RotProp.c \
                  ScrResStr.c \
                  SendEvent.c \
                  SetBack.c \
                  SetCRects.c \
                  SetClMask.c \
                  SetClOrig.c \
                  SetDashes.c \
                  SetFPath.c \
                  SetFont.c \
                  SetFore.c \
                  SetFunc.c \
                  SetHints.c \
                  SetLStyle.c \
                  SetLocale.c \
                  SetNrmHint.c \
                  SetPMask.c \
                  SetPntMap.c \
                  SetRGBCMap.c \
                  SetStCmap.c \
                  SetState.c \
                  SetStip.c \
                  SetTSOrig.c \
                  SetTile.c \
                  SetTxtProp.c \
                  SetWMCMapW.c \
                  SetWMProto.c \
                  StBytes.c \
                  StColor.c \
                  StColors.c \
                  StNColor.c \
                  StName.c \
                  StrKeysym.c \
                  StrToText.c \
                  Sync.c \
                  Synchro.c \
                  Text.c \
                  Text16.c \
                  TextExt.c \
                  TextExt16.c \
                  TextToStr.c \
                  TrCoords.c \
                  UndefCurs.c \
                  VisUtil.c \
                  WMGeom.c \
                  WMProps.c \
                  WinEvent.c \
                  Window.c \
                  Withdraw.c \
                  WrBitF.c \
                  XlibAsync.c \
                  XlibInt.c \
                  Xrm.c \
                  evtomask.c \
                  globals.c \
                  locking.c \
                  Cr.h \
                  Key.h \
                  Xatomtype.h \
                  Xintatom.h \
                  Xintconn.h \
                  XomGeneric.h \
                  Xresinternal.h \
                  XrmI.h \
                  locking.h \
                  poly.h 

x11datadir = @X11_DATADIR@

x11data_DATA = XKeysymDB XErrorDB

EXTRA_DIST = \
	$(X11_SRCS) \
	$(XI18N_SRCS) \
	$(XCMS_SRCS) \
	X18NCMSstubs.c \
	$(x11data_DATA)

libX11_la_SOURCES = $(X11_SRCS)

libX11_la_SOURCES += \
                  ConnDis.c \
                  LockDis.c \
                  AllowEv.c \
                  Backgnd.c \
                  Bell.c \
                  ChActPGb.c \
                  ChClMode.c \
                  ChPntCon.c \
                  ChWindow.c \
                  Clear.c \
                  ClearArea.c \
                  ConfWind.c \
                  ConvSel.c \
                  CopyArea.c \
                  CopyPlane.c \
                  CrPixmap.c \
                  DefCursor.c \
                  DelProp.c \
                  DestSubs.c \
                  DestWind.c \
                  DrLine.c \
                  DrPoint.c \
                  FillPoly.c \
                  FreeCmap.c \
                  FreeCols.c \
                  FreeCurs.c \
                  FreePix.c \
                  GrButton.c \
                  GrKey.c \
                  GrServer.c \
                  InsCmap.c \
                  KillCl.c \
                  MapSubs.c \
                  MapWindow.c \
                  MoveWin.c \
                  PmapBgnd.c \
                  RepWindow.c \
                  SelInput.c \
                  SetIFocus.c \
                  SetSOwner.c \
                  SetSSaver.c \
                  UngrabBut.c \
                  UngrabKbd.c \
                  UngrabKey.c \
                  UngrabPtr.c \
                  UngrabSvr.c \
                  UninsCmap.c \
                  UnldFont.c \
                  UnmapSubs.c \
                  UnmapWin.c \
                  WarpPtr.c \
                  x11_trans.c \
                  x11_trans.h

if DISABLE_XLOCALE
libX11_la_SOURCES+= X18NCMSstubs.c 
else
libX11_la_SOURCES+= $(XI18N_SRCS)
endif

if DISABLE_XCMS
if !DISABLE_XLOCALE
libX11_la_SOURCES+= X18NCMSstubs.c 
endif
else
libX11_la_SOURCES+= $(XCMS_SRCS)
endif

if OS2
libX11_la_SOURCES+=os2Stubs.c
endif OS2

if UDC
libX11_la_SOURCES+=udcInf.c
endif

if XKB
libX11_la_SOURCES+=XKB.c XKBBind.c XKBCompat.c XKBCtrls.c XKBCvt.c XKBGetMap.c \
                  XKBGetByName.c XKBNames.c XKBRdBuf.c XKBSetMap.c XKBUse.c \
                  XKBleds.c XKBBell.c XKBGeom.c XKBSetGeom.c XKBExtDev.c \
                  XKBList.c XKBMisc.c \
                  XKBMAlloc.c XKBGAlloc.c XKBAlloc.c \
                  XKBlibint.h
endif XKB

preprocess: $(patsubst %.c,%.ii,$(libX11_la_SOURCES))
.c.ii:
	$(COMPILE) -E -o $@ `test -f '$<' || echo '$(srcdir)/'`$<

KEYSYMDEF=@KEYSYMDEF@

ks_tables.h: $(KEYSYMDEF) $(top_builddir)/src/util/makekeys$(EXEEXT)
	$(top_builddir)/src/util/makekeys < $(KEYSYMDEF) > ks_tables_h
	mv ks_tables_h $@

$(top_builddir)/src/util/makekeys$(EXEEXT): force
	cd util; $(MAKE)

force:
