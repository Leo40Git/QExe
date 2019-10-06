#ifndef QEXE_GLOBAL_H
#define QEXE_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QEXE_LIBRARY)
#  define QEXE_EXPORT Q_DECL_EXPORT
#else
#  define QEXE_EXPORT Q_DECL_IMPORT
#endif

#endif // QEXE_GLOBAL_H
