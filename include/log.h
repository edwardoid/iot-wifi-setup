#ifndef IWS_LOG_H
#define IWS_LOG_H

#include <iostream>
#define F_NAME __PRETTY_FUNCTION__
//#define LOG_FMT " In:\n" << __FILE__ << ":" << __LINE__ << " " << F_NAME << ":\n\t"
#define LOG_FMT " " << F_NAME << ":"

#ifndef LOG_DEBUG
#define LOG_DEBUG std::cout << "\nDBG" << LOG_FMT
#endif

#ifndef LOG_INFO
#define LOG_INFO  std::cout << "\nINF" << LOG_FMT
#endif

#ifndef LOG_ERROR
#define LOG_ERROR std::cerr << "\nERR" << LOG_FMT
#endif

#ifndef LOG_WARN
#define LOG_WARN  std::cout << "\nWRN" << LOG_FMT
#endif

#ifndef LOG_WARNING
#define LOG_WARNING LOG_WARN
#endif

#endif // IWS_LOG_H
