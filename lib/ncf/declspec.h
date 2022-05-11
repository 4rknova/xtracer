#ifndef LIBNCF_DECLSPEC_H_INCLUDED
#define LIBNCF_DECLSPEC_H_INCLUDED

#ifdef _MSC_VER
	#ifdef LIBNCF_EXPORT
		#define DECLSPEC __declspec(dllexport)
	#else
		#define DECLSPEC __declspec(dllimport)
	#endif

#else
	#define DECLSPEC

#endif

#endif /* LIBNCF_DECLSPEC_H_INCLUDED */
