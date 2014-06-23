
#if 0
// This file is intended to be passed to Microsoft's C/C++ compiler in a
// "preprocess-only" mode.
#endif

// This output is generated like so:
//
//    cl /EP /C [X] vs2010-m.cpp  >vs2010-m.lnt
//
// ... where [X] is the list of options passed to the compiler to build
// your project and "vs2010-m.lnt" is a suggested name.
//
// The output is a sequence of macro definition options that should be
// passed to Lint.  This will configure Lint's preprocessor to have the
// same set implicitly-defined macro definitions that the Microsoft
// compiler assumes for a given build configuration.
//
// If you want to lint with different build configurations, you may want
// to use a naming convention to identify the build configuration to which
// the output corresponds (e.g. "win32_debug-implicit.lnt").  Note that
// the filename should end in ".lnt" so that Lint sees it as a file
// containing command arguments.
//
// We recommend the use of "/EP /C" instead of "/E" because it suppresses
// #line directives and includes documentation for each active macro, so
// the output is ready to use as a well-formed Lint options file.
//
// For convenience, descriptions of macros have been copied from
// Microsoft's online documentation for Visual Studio 2010.
//
// Note: macros whose expansions depend on conditions at the point of use
// (like __FUNCSIG__, __COUNTER__, etc.) are deliberately omitted.

#ifdef _ATL_VER
    -d_ATL_VER{_ATL_VER}
    // 
    // Defines the ATL version.
    // 
#endif

#ifdef _CHAR_UNSIGNED
    -d_CHAR_UNSIGNED{_CHAR_UNSIGNED}
    // 
    // Default char type is unsigned. Defined when /J is specified.
    // 
#endif

#ifdef __CLR_VER
    -d__CLR_VER{__CLR_VER}
    // 
    // Defines the version of the common language runtime used when the
    // application was compiled. The value returned will be in the
    // following format:
    // 
    // Mmmbbbbb
    // 
    // where,
    // 
    // M is the major version of the runtime
    // 
    // mm is the minor version of the runtime
    // 
    // bbbbb is the build number.
    // 
#endif

#ifdef __cplusplus_cli
    -d__cplusplus_cli{__cplusplus_cli}
    // 
    // Defined when you compile with /clr, /clr:pure, or /clr:safe. Value
    // of __cplusplus_cli is 200406. __cplusplus_cli is in effect
    // throughout the translation unit.
    // 
#endif

#ifdef _CPPRTTI
    -d_CPPRTTI{_CPPRTTI}
    // 
    // Defined for code compiled with /GR (Enable Run-Time Type
    // Information).
    // 
#endif

#ifdef _CPPUNWIND
    -d_CPPUNWIND{_CPPUNWIND}
    // 
    // Defined for code compiled with /GX (Enable Exception Handling).
    // 
#endif

#ifdef _DEBUG
    -d_DEBUG{_DEBUG}
    // 
    // Defined when you compile with /LDd, /MDd, and /MTd.
    // 
#endif

#ifdef _DLL
    -d_DLL{_DLL}
    // 
    // Defined when /MD or /MDd (Multithreaded DLL) is specified.
    // 
#endif

#ifdef _INTEGRAL_MAX_BITS
    -d_INTEGRAL_MAX_BITS{_INTEGRAL_MAX_BITS}
    // 
    // Reports the maximum size (in bits) for an integral type.
    // 
#endif

#ifdef _M_ALPHA
    -d_M_ALPHA{_M_ALPHA}
    // 
    // Defined for DEC ALPHA platforms (no longer supported).
    // 
#endif

#ifdef _M_CEE
    -d_M_CEE{_M_CEE}
    // 
    // Defined for a compilation that uses any form of /clr
    // (/clr:oldSyntax, /clr:safe, for example).
    // 
#endif

#ifdef _M_CEE_PURE
    -d_M_CEE_PURE{_M_CEE_PURE}
    // 
    // Defined for a compilation that uses /clr:pure.
    // 
#endif

#ifdef _M_CEE_SAFE
    -d_M_CEE_SAFE{_M_CEE_SAFE}
    // 
    // Defined for a compilation that uses /clr:safe.
    // 
#endif

#ifdef _M_IX86
    -d_M_IX86{_M_IX86}
    // 
    // Defined for x86 processors. See the Values for _M_IX86 table below
    // for more information. This is not defined for x64 processors.
    // 
#endif

#ifdef _M_IA64
    -d_M_IA64{_M_IA64}
    // 
    // Defined for Itanium Processor Family 64-bit processors.
    // 
#endif

#ifdef _M_IX86_FP
    -d_M_IX86_FP{_M_IX86_FP}
    // 
    // Expands to a value indicating which /arch compiler option was used:
    // 
    // 0 if /arch was not used.
    // 
    // 1 if /arch:SSE was used.
    // 
    // 2 if /arch:SSE2 was used.
    // 
    // See /arch (Minimum CPU Architecture) for more information.
    // 
#endif

#ifdef _M_MPPC
    -d_M_MPPC{_M_MPPC}
    // 
    // Defined for Power Macintosh platforms (no longer supported).
    // 
#endif

#ifdef _M_MRX000
    -d_M_MRX000{_M_MRX000}
    // 
    // Defined for MIPS platforms (no longer supported).
    // 
#endif

#ifdef _M_PPC
    -d_M_PPC{_M_PPC}
    // 
    // Defined for PowerPC platforms (no longer supported).
    // 
#endif

#ifdef _M_X64
    -d_M_X64{_M_X64}
    // 
    // Defined for x64 processors.
    // 
#endif

#ifdef _MANAGED
    -d_MANAGED{_MANAGED}
    // 
    // Defined to be 1 when /clr is specified.
    // 
#endif

#ifdef _MFC_VER
    -d_MFC_VER{_MFC_VER}
    // 
    // Defines the MFC version. In Visual Studio 2010, _MFC_VER is defined
    // as 0x1000.
    // 
#endif

#ifdef _MSC_BUILD
    -d_MSC_BUILD{_MSC_BUILD}
    // 
    // Evaluates to the revision number component of the compiler's
    // version number. The revision number is the fourth component of the
    // period-delimited version number. For example, if the version number
    // of the Visual C++ compiler is 15.00.20706.01, the _MSC_BUILD macro
    // evaluates to 1.
    // 
#endif

#ifdef _MSC_EXTENSIONS
    -d_MSC_EXTENSIONS{_MSC_EXTENSIONS}
    // 
    // This macro is defined when you compile with the /Ze compiler option
    // (the default). Its value, when defined, is 1.
    // 
#endif

#ifdef _MSC_FULL_VER
    -d_MSC_FULL_VER{_MSC_FULL_VER}
    // 
    // Evaluates to the major, minor, and build number components of the
    // compiler's version number. The major number is the first component
    // of the period-delimited version number, the minor number is the
    // second component, and the build number is the third component. For
    // example, if the version number of the Visual C++ compiler is
    // 15.00.20706.01, the _MSC_FULL_VER macro evaluates to 150020706.
    // Type cl /? at the command line to view the compiler's version
    // number.
    // 
#endif

#ifdef _MSC_VER
    -d_MSC_VER{_MSC_VER}
    // 
    // Evaluates to the major and minor number components of the
    // compiler's version number. The major number is the first component
    // of the period-delimited version number and the minor number is the
    // second component.
    // 
    // For example, if the version number of the Visual C++ compiler is
    // 15.00.20706.01, the _MSC_VER macro evaluates to 1500.
    // 
    // In Visual Studio 2010, _MSC_VER is defined as 1600.
    // 
#endif

#ifdef __MSVC_RUNTIME_CHECKS
    -d__MSVC_RUNTIME_CHECKS{__MSVC_RUNTIME_CHECKS}
    // 
    // Defined when one of the /RTC compiler options is specified.
    // 
#endif

#ifdef _MT
    -d_MT{_MT}
    // 
    // Defined when /MD or /MDd (Multithreaded DLL) or /MT or /MTd
    // (Multithreaded) is specified.
    // 
#endif

#ifdef _NATIVE_WCHAR_T_DEFINED
    -d_NATIVE_WCHAR_T_DEFINED{_NATIVE_WCHAR_T_DEFINED}
    // 
    // Defined when /Zc:wchar_t is used.
    // 
#endif

#ifdef _OPENMP
    -d_OPENMP{_OPENMP}
    // 
    // Defined when compiling with /openmp, returns an integer
    // representing the date of the OpenMP specification implemented by
    // Visual C++.
    // 
#endif

#ifdef _VC_NODEFAULTLIB
    -d_VC_NODEFAULTLIB{_VC_NODEFAULTLIB}
    // 
    // Defined when /Zl is used; see /Zl (Omit Default Library Name) for
    // more information.
    // 
#endif

#ifdef _WCHAR_T_DEFINED
    -d_WCHAR_T_DEFINED{_WCHAR_T_DEFINED}
    // 
    // Defined when /Zc:wchar_t is used or if wchar_t is defined in a
    // system header file included in your project.
    // 
#endif

#ifdef _WIN32
    -d_WIN32{_WIN32}
    // 
    // Defined for applications for Win32 and Win64. Always defined.
    // 
#endif

#ifdef _WIN64
    -d_WIN64{_WIN64}
    // 
    // Defined for applications for Win64.
    // 
#endif

#ifdef _Wp64
    -d_Wp64{_Wp64}
    // 
    // Defined when specifying /Wp64.
    //
#endif

  /**** Undocumented implicitly-defined macros follow ****/

#ifdef _NATIVE_NULLPTR_SUPPORTED
    -d_NATIVE_NULLPTR_SUPPORTED{_NATIVE_NULLPTR_SUPPORTED}
#endif

#ifdef _M_AMD64
    -d_M_AMD64{_M_AMD64}
#endif

#ifdef _AFXDLL
    -d_AFXDLL{_AFXDLL}
#endif

#ifdef __STDC__
    -d__STDC__{__STDC__}
#endif
