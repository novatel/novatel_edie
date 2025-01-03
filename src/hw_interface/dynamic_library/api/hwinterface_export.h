
#ifndef HWINTERFACE_EXPORT_H
#define HWINTERFACE_EXPORT_H

#ifdef HWINTERFACE_STATIC_DEFINE
#  define HWINTERFACE_EXPORT
#  define HWINTERFACE_NO_EXPORT
#else
#  ifndef HWINTERFACE_EXPORT
#    ifdef HWINTERFACE_EXPORTS
        /* We are building this library */
#      define HWINTERFACE_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define HWINTERFACE_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef HWINTERFACE_NO_EXPORT
#    define HWINTERFACE_NO_EXPORT 
#  endif
#endif

#ifndef HWINTERFACE_DEPRECATED
#  define HWINTERFACE_DEPRECATED __declspec(deprecated)
#endif

#ifndef HWINTERFACE_DEPRECATED_EXPORT
#  define HWINTERFACE_DEPRECATED_EXPORT HWINTERFACE_EXPORT HWINTERFACE_DEPRECATED
#endif

#ifndef HWINTERFACE_DEPRECATED_NO_EXPORT
#  define HWINTERFACE_DEPRECATED_NO_EXPORT HWINTERFACE_NO_EXPORT HWINTERFACE_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef HWINTERFACE_NO_DEPRECATED
#    define HWINTERFACE_NO_DEPRECATED
#  endif
#endif

#endif /* HWINTERFACE_EXPORT_H */
