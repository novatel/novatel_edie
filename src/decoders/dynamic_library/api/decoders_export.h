
#ifndef DECODERS_EXPORT_H
#define DECODERS_EXPORT_H

#ifdef DECODERS_STATIC_DEFINE
#  define DECODERS_EXPORT
#  define DECODERS_NO_EXPORT
#else
#  ifndef DECODERS_EXPORT
#    ifdef DECODERS_EXPORTS
        /* We are building this library */
#      define DECODERS_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define DECODERS_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef DECODERS_NO_EXPORT
#    define DECODERS_NO_EXPORT
#  endif
#endif

#ifndef DECODERS_DEPRECATED
#  define DECODERS_DEPRECATED __declspec(deprecated)
#endif

#ifndef DECODERS_DEPRECATED_EXPORT
#  define DECODERS_DEPRECATED_EXPORT DECODERS_EXPORT DECODERS_DEPRECATED
#endif

#ifndef DECODERS_DEPRECATED_NO_EXPORT
#  define DECODERS_DEPRECATED_NO_EXPORT DECODERS_NO_EXPORT DECODERS_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef DECODERS_NO_DEPRECATED
#    define DECODERS_NO_DEPRECATED
#  endif
#endif

#endif /* DECODERS_EXPORT_H */

