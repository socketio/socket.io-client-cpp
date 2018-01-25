#ifndef SIOCLIENT_EXPORT_H
#define SIOCLIENT_EXPORT_H

#ifdef _WIN32
#if defined(sioclient_EXPORTS) || defined(sioclient_tls_EXPORTS)
#define SIOCLIENT_EXPORT __declspec(dllexport)
#else
#define SIOCLIENT_EXPORT
#endif
#else
#define SIOCLIENT_EXPORT
#endif

#endif // SIOCLIENT_EXPORT_H
