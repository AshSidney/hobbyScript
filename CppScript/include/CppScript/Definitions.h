#pragma once

#ifndef CPPSCRIPT_API
	#ifndef _NOEXPORT
		#ifdef _WIN32
			#ifdef CPPSCRIPT_DLL
				#define CPPSCRIPT_API __declspec(dllexport)
			#else
				#define CPPSCRIPT_API __declspec(dllimport)
			#endif
		#else
			#define CPPSCRIPT_API __attribute__((visibility("default")))
		#endif
	#else
		#define CPPSCRIPT_API
	#endif
#endif
