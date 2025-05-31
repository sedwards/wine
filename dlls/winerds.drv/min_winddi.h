#ifndef MIN_WINDDI_H
#define MIN_WINDDI_H


#define INDEX_DrvEnablePDEV               __MSABI_LONG(0)
#define INDEX_DrvCompletePDEV             __MSABI_LONG(1)
#define INDEX_DrvDisablePDEV              __MSABI_LONG(2)
#define INDEX_DrvEnableSurface            __MSABI_LONG(3)
#define INDEX_DrvDisableSurface           __MSABI_LONG(4)
#define INDEX_DrvAssertMode               __MSABI_LONG(5)
#define INDEX_DrvOffset                   __MSABI_LONG(6)
#define INDEX_DrvResetPDEV                __MSABI_LONG(7)
#define INDEX_DrvDisableDriver            __MSABI_LONG(8)
#define INDEX_DrvUnknown1                 __MSABI_LONG(9)
#define INDEX_DrvCreateDeviceBitmap       __MSABI_LONG(10)
#define INDEX_DrvDeleteDeviceBitmap       __MSABI_LONG(11)
#define INDEX_DrvRealizeBrush             __MSABI_LONG(12)
#define INDEX_DrvDitherColor              __MSABI_LONG(13)
#define INDEX_DrvStrokePath               __MSABI_LONG(14)
#define INDEX_DrvFillPath                 __MSABI_LONG(15)
#define INDEX_DrvStrokeAndFillPath        __MSABI_LONG(16)
#define INDEX_DrvPaint                    __MSABI_LONG(17)
#define INDEX_DrvBitBlt                   __MSABI_LONG(18)
#define INDEX_DrvCopyBits                 __MSABI_LONG(19)
#define INDEX_DrvStretchBlt               __MSABI_LONG(20)
#define INDEX_DrvUnknown2                 __MSABI_LONG(21)
#define INDEX_DrvSetPalette               __MSABI_LONG(22)
#define INDEX_DrvTextOut                  __MSABI_LONG(23)
#define INDEX_DrvEscape                   __MSABI_LONG(24)
#define INDEX_DrvDrawEscape               __MSABI_LONG(25)
#define INDEX_DrvQueryFont                __MSABI_LONG(26)
#define INDEX_DrvQueryFontTree            __MSABI_LONG(27)
#define INDEX_DrvQueryFontData            __MSABI_LONG(28)
#define INDEX_DrvSetPointerShape          __MSABI_LONG(29)
#define INDEX_DrvMovePointer              __MSABI_LONG(30)
#define INDEX_DrvLineTo                   __MSABI_LONG(31)
#define INDEX_DrvSendPage                 __MSABI_LONG(32)
#define INDEX_DrvStartPage                __MSABI_LONG(33)
#define INDEX_DrvEndDoc                   __MSABI_LONG(34)
#define INDEX_DrvStartDoc                 __MSABI_LONG(35)
#define INDEX_DrvUnknown3                 __MSABI_LONG(36)
#define INDEX_DrvGetGlyphMode             __MSABI_LONG(37)
#define INDEX_DrvSynchronize              __MSABI_LONG(38)
#define INDEX_DrvUnknown4                 __MSABI_LONG(39)
#define INDEX_DrvSaveScreenBits           __MSABI_LONG(40)
#define INDEX_DrvGetModes                 __MSABI_LONG(41)
#define INDEX_DrvFree                     __MSABI_LONG(42)
#define INDEX_DrvDestroyFont              __MSABI_LONG(43)
#define INDEX_DrvQueryFontCaps            __MSABI_LONG(44)
#define INDEX_DrvLoadFontFile             __MSABI_LONG(45)
#define INDEX_DrvUnloadFontFile           __MSABI_LONG(46)
#define INDEX_DrvFontManagement           __MSABI_LONG(47)
#define INDEX_DrvQueryTrueTypeTable       __MSABI_LONG(48)
#define INDEX_DrvQueryTrueTypeOutline     __MSABI_LONG(49)
#define INDEX_DrvGetTrueTypeFile          __MSABI_LONG(50)
#define INDEX_DrvQueryFontFile            __MSABI_LONG(51)
#define INDEX_DrvMovePanning              __MSABI_LONG(52)
#define INDEX_DrvQueryAdvanceWidths       __MSABI_LONG(53)
#define INDEX_DrvSetPixelFormat           __MSABI_LONG(54)
#define INDEX_DrvDescribePixelFormat      __MSABI_LONG(55)
#define INDEX_DrvSwapBuffers              __MSABI_LONG(56)
#define INDEX_DrvStartBanding             __MSABI_LONG(57)
#define INDEX_DrvNextBand                 __MSABI_LONG(58)
#define INDEX_DrvGetDirectDrawInfo        __MSABI_LONG(59)
#define INDEX_DrvEnableDirectDraw         __MSABI_LONG(60)
#define INDEX_DrvDisableDirectDraw        __MSABI_LONG(61)
#define INDEX_DrvQuerySpoolType           __MSABI_LONG(62)
#define INDEX_DrvUnknown5                 __MSABI_LONG(63)
#define INDEX_DrvIcmCreateColorTransform  __MSABI_LONG(64)
#define INDEX_DrvIcmDeleteColorTransform  __MSABI_LONG(65)
#define INDEX_DrvIcmCheckBitmapBits       __MSABI_LONG(66)
#define INDEX_DrvIcmSetDeviceGammaRamp    __MSABI_LONG(67)
#define INDEX_DrvGradientFill             __MSABI_LONG(68)
#define INDEX_DrvStretchBltROP            __MSABI_LONG(69)
#define INDEX_DrvPlgBlt                   __MSABI_LONG(70)
#define INDEX_DrvAlphaBlend               __MSABI_LONG(71)
#define INDEX_DrvSynthesizeFont           __MSABI_LONG(72)
#define INDEX_DrvGetSynthesizedFontFiles  __MSABI_LONG(73)
#define INDEX_DrvTransparentBlt           __MSABI_LONG(74)
#define INDEX_DrvQueryPerBandInfo         __MSABI_LONG(75)
#define INDEX_DrvQueryDeviceSupport       __MSABI_LONG(76)

typedef PVOID PFN;
typedef HANDLE HSURF;

/* Replicated from wine gdi_driver.h */
typedef void *DHPDEV;
typedef HANDLE HDEV;


typedef struct _DRVFN {
    ULONG iFunc;
    PFN   pfn;
} DRVFN;

/* More privateness that needs to perhaps go either locally to the driver or some proper place in Wine */
typedef struct _DEVINFO DEVINFO;
typedef struct _SURFOBJ SURFOBJ;
typedef struct _STROBJ STROBJ;
typedef struct _FONTOBJ FONTOBJ;
typedef struct _CLIPOBJ CLIPOBJ;
typedef struct _BRUSHOBJ BRUSHOBJ;
typedef ULONG MIX;


DHPDEV DrvEnablePDEV(
    DEVMODEW*   pdevmode,
    LPWSTR      pwszLogAddress,
    ULONG       cPat,
    HSURF*      phsurfPatterns,
    ULONG       cjCaps,
    ULONG*      pdevcaps,
    ULONG       cjDevInfo,
    DEVINFO*    pdi,
    HDEV        hdev,
    PWSTR       pwszDeviceName,
    HANDLE      hDriver
);

VOID DrvCompletePDEV(
    DHPDEV dhpdev,
    HDEV   hdev
);

VOID DrvDisablePDEV(
    DHPDEV dhpdev
);

HSURF DrvEnableSurface(
    DHPDEV dhpdev
);

VOID DrvDisableSurface(
    DHPDEV dhpdev
);


ULONG DrvGetModes(
    HANDLE    hDriver,
    ULONG     cjSize,
    DEVMODEW* pdm
);

typedef struct _DRVENABLEDATA {
    ULONG iDriverVersion;
    ULONG c;
    DRVFN* pdrvfn;
} DRVENABLEDATA;

/* FIXME WinRDS Extension:
 * Not always explicitly defined; usually custom-declared in a driver using typedef.
   Best practice: Declare in a driver-specific header like driver.h or textout.h.
 */
typedef BOOL (*PFN_DrvTextOut)(
    SURFOBJ* pso,
    STROBJ*  pstro,
    FONTOBJ* pfo,
    CLIPOBJ* pco,
    RECTL*   prclExtra,
    RECTL*   prclOpaque,
    BRUSHOBJ* pboFore,
    BRUSHOBJ* pboOpaque,
    POINTL* pptlOrg,
    MIX      mix
);


#if 0

typedef struct _DRVFN {
  ULONG  iFunc;
  PFN  pfn;
} DRVFN, *PDRVFN;


HSURF
APIENTRY
DrvEnableSurface(
  DHPDEV  dhpdev);

typedef WINBOOL
(APIENTRY *PFN_DrvTextOut)(
  SURFOBJ  *pso,
  STROBJ  *pstro,
  FONTOBJ  *pfo,
  CLIPOBJ  *pco,
  RECTL  *prclExtra,
  RECTL  *prclOpaque,
  BRUSHOBJ  *pboFore,
  BRUSHOBJ  *pboOpaque,
  POINTL  *pptlOrg,
  MIX  mix);

DHPDEV
APIENTRY
DrvEnablePDEV(
  DEVMODEW  *pdm,
  LPWSTR  pwszLogAddress,
  ULONG  cPat,
  HSURF  *phsurfPatterns,
  ULONG  cjCaps,
  ULONG  *pdevcaps,
  ULONG  cjDevInfo,
  DEVINFO  *pdi,
  HDEV  hdev,
  LPWSTR  pwszDeviceName,
  HANDLE  hDriver);

#endif

#endif /* MIN_WINDDI_H */
