// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; coding: utf-8-unix -*-
// ***** BEGIN LICENSE BLOCK *****
////////////////////////////////////////////////////////////////////
// Copyright (c) 2011-2013 RALOVICH, Kristóf                      //
//                                                                //
// This program is free software; you can redistribute it and/or  //
// modify it under the terms of the GNU General Public License    //
// version 2 as published by the Free Software Foundation.        //
//                                                                //
////////////////////////////////////////////////////////////////////
// ***** END LICENSE BLOCK *****


#include "common.hpp"
#include <iostream>
#include <boost/thread/thread_time.hpp>
#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
//#include <QDateTime>
#include <sstream>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include "Log.hpp"
#include "antdefs.hpp"


namespace antpm {

//std::ostream&
//logger()
//{
//  //antpm::Log::instance()->
//  std::cout.flush();
//  return std::cout;
//}

// FILE*
// loggerc()
// {
//   fflush(stdout);
//   return stdout;
// }


void
sleepms(const size_t timeout_ms)
{
  boost::system_time const timeout=boost::get_system_time() + boost::posix_time::milliseconds(timeout_ms);
  boost::this_thread::sleep(timeout);
}


#ifndef _MSC_VER
/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
char*
itoa(int value, char* result, int base)
{
  // check that the base if valid
  if (base < 2 || base > 36) { *result = '\0'; return result; }

  char* ptr = result, *ptr1 = result, tmp_char;
  int tmp_value;

  do {
    tmp_value = value;
    value /= base;
    *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
  } while ( value );

  // Apply negative sign
  if (tmp_value < 0) *ptr++ = '-';
  *ptr-- = '\0';
  while(ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr--= *ptr1;
    *ptr1++ = tmp_char;
  }
  return result;
}
#endif


template < class T >
const std::string
toString(const T& val, const int width, const char fill)
{
  std::stringstream clStr;
  if(width != -1) clStr << std::setw(width);
  if(fill != ' ') clStr << std::setfill(fill);
  clStr << std::hex;
  clStr << val;
  return clStr.str();
}


template < class T >
const std::string
toStringDec(const T& val, const int width, const char fill)
{
  std::stringstream clStr;
  if(width != -1) clStr << std::setw(width);
  if(fill != ' ') clStr << std::setfill(fill);
  clStr << val;
  return clStr.str();
}

const std::vector<std::string>
tokenize(const std::string& text, const char* delims)
{
  std::vector<std::string> rv;
  boost::char_separator<char> sep(delims);
  boost::tokenizer<boost::char_separator<char> > tokens(text, sep);
  BOOST_FOREACH(std::string t, tokens)
  {
    rv.push_back(t);
  }
  return rv;
}

const std::string
getDateString()
{
  //QString filename = QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss");
  //QByteArray ba=filename.toLatin1();
  //return ba.constData();
  std::ostringstream msg;
  const boost::posix_time::ptime now=boost::posix_time::second_clock::local_time();
  boost::posix_time::time_facet*const f=new boost::posix_time::time_facet("%Y_%m_%d_%H_%M_%S");
  msg.imbue(std::locale(msg.getloc(),f));
  msg << now;
  return msg.str();
}


const std::string
getConfigFileName()
{
  return getConfigFolder() + "/config.ini";
}


const std::string
getConfigFolder()
{
  const char* e0 = getenv("ANTPM_DIR");
  if(e0 && strlen(e0) > 0)
    return std::string(e0) + "/";

  // http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
  // $XDG_CONFIG_HOME defines the base directory relative to which user specific configuration files should be stored.
  // If $XDG_CONFIG_HOME is either not set or empty, a default equal to $HOME/.config should be used. 

#ifndef _WIN32
  const char* e1 = getenv("XDG_CONFIG_HOME");
  const char* e2 = getenv("HOME");
  if(e1 && strlen(e1) > 0)
    return std::string(e1) + "/" APP_NAME "/";
  else if(e2 && strlen(e2) > 0)
    return std::string(e2) + "/.config/" APP_NAME "/";
#else
  const char* e3 = getenv("USERPROFILE");
  if(e3 && strlen(e3) > 0)
    return std::string(e3) + "/.config/" APP_NAME "/";
#endif
  else
    return "~/.config/" APP_NAME "/";
}



void
readUInt64(const uint clientSN, uint64_t& pairedKey)
{
  std::stringstream ss;
  ss << getConfigFolder() << "libantpmauth_" << clientSN;
  FILE* f=fopen(ss.str().c_str(), "r");
  LOG_VAR2(ss.str(), f);
  if(f)
  {
    if(1 != fread(&pairedKey, sizeof(pairedKey), 1, f))
      pairedKey = 0;
    fclose(f);
  }
}

void
writeUInt64(const uint clientSN, const uint64_t& pairedKey)
{
  std::stringstream ss;
  ss << getConfigFolder() << "libantpmauth_" << clientSN;
  FILE* f=fopen(ss.str().c_str(), "w+");
  LOG_VAR2(ss.str(), f);
  if(f)
  {
    fwrite(&pairedKey, sizeof(pairedKey), 1, f);
    fclose(f);
  }
}

std::vector<unsigned char>
readFile(const char* fileName)
{
  std::vector<unsigned char> v;
  FILE* f=fopen(fileName, "rb");
  if(f)
  {
    unsigned char buf[256];
    size_t lastRead=0;
    do
    {
      lastRead=fread(buf, 1, 256, f);
      v.insert(v.end(), buf, buf+lastRead);
    } while(lastRead>0);
    fclose(f);
  }
  return v;
}


bool
mkDir(const char* dirName)
{
  try
  {
    boost::filesystem::path ddir(dirName);
    LOG(LOG_DBG) << "mkDir: \"" << ddir << "\"\n";
    return boost::filesystem::create_directories(ddir);
  }
  // Throws:  basic_filesystem_error<Path> if exists(p) && !is_directory(p)
  catch(boost::filesystem::filesystem_error& bfe)
  {
    LOG(LOG_WARN) << "mkDir: failed\n"
                  << "\twhat  " << bfe.what() << "\n"
                  << "\tpath1 =" << bfe.path1() << "\n"
                  << "\tpath2 =" << bfe.path2() << "\n";
    return false;
  }
}


bool
folderExists(const char* dirName)
{
  boost::filesystem::path ddir(dirName);
  return boost::filesystem::exists(ddir) && boost::filesystem::is_directory(ddir);
}

std::string
getVersionString()
{
  return std::string("") + APP_NAME
      + " v" + std::string(BOOST_STRINGIZE(ANTPM_VERSION))
      + " built " __DATE__ "" //+#ANTPM_SERIAL_IMPL
      + " under "
#ifdef __linux__
  "linux"
#elif defined(_WIN64)
  "win64"
#elif defined(_WIN32)
  "win32"
#else
  "unknown_os"
#endif
      + " with "
#ifdef __GNUC__
  "GCC " __VERSION__
#elif defined(_MSC_VER)
  "MSVC " _MSC_FULL_VER
#else
  "unknow_compiler"
#endif
      + "";
}















#define ANTP_PAIR(x) {x,#x}

MsgNames msgNames[]={
  ANTP_PAIR(MESG_EVENT_ID),
  ANTP_PAIR(MESG_ACKNOWLEDGED_DATA_ID),
  ANTP_PAIR(MESG_ASSIGN_CHANNEL_ID),
  ANTP_PAIR(MESG_BROADCAST_DATA_ID),
  ANTP_PAIR(MESG_BURST_DATA_ID),
  ANTP_PAIR(MESG_CAPABILITIES_ID),
  ANTP_PAIR(MESG_CHANNEL_ID_ID),
  ANTP_PAIR(MESG_CHANNEL_MESG_PERIOD_ID),
  ANTP_PAIR(MESG_CHANNEL_RADIO_FREQ_ID),
  ANTP_PAIR(MESG_CHANNEL_SEARCH_TIMEOUT_ID),
  ANTP_PAIR(MESG_CHANNEL_STATUS_ID),
  ANTP_PAIR(MESG_VERSION_ID),
  ANTP_PAIR(MESG_GET_SERIAL_NUM_ID),
  ANTP_PAIR(MESG_CLOSE_CHANNEL_ID),
  ANTP_PAIR(MESG_EXT_ACKNOWLEDGED_DATA_ID),
  ANTP_PAIR(MESG_EXT_BROADCAST_DATA_ID),
  ANTP_PAIR(MESG_EXT_BURST_DATA_ID),
  ANTP_PAIR(MESG_NETWORK_KEY_ID),
  ANTP_PAIR(MESG_OPEN_CHANNEL_ID),
  ANTP_PAIR(MESG_OPEN_RX_SCAN_ID),
  ANTP_PAIR(MESG_REQUEST_ID),
  ANTP_PAIR(MESG_RESPONSE_EVENT_ID),
  ANTP_PAIR(MESG_SEARCH_WAVEFORM_ID),
  ANTP_PAIR(MESG_SYSTEM_RESET_ID),
  ANTP_PAIR(MESG_UNASSIGN_CHANNEL_ID),
  {-1,"UNKNOWN"}
};


ResponseNames responseNames[]={
  ANTP_PAIR(RESPONSE_NO_ERROR),
  ANTP_PAIR(EVENT_RX_SEARCH_TIMEOUT),
  ANTP_PAIR(EVENT_RX_FAIL),
  ANTP_PAIR(EVENT_TX),
  ANTP_PAIR(EVENT_TRANSFER_RX_FAILED),
  ANTP_PAIR(EVENT_TRANSFER_TX_COMPLETED),
  ANTP_PAIR(EVENT_TRANSFER_TX_FAILED),
  ANTP_PAIR(EVENT_CHANNEL_CLOSED),
  ANTP_PAIR(EVENT_RX_FAIL_GO_TO_SEARCH),
  ANTP_PAIR(EVENT_CHANNEL_COLLISION),
  ANTP_PAIR(EVENT_TRANSFER_TX_START),
  ANTP_PAIR(CHANNEL_IN_WRONG_STATE),
  ANTP_PAIR(CHANNEL_NOT_OPENED),
  ANTP_PAIR(CHANNEL_ID_NOT_SET),
  ANTP_PAIR(CLOSE_ALL_CHANNELS),
  ANTP_PAIR(TRANSFER_IN_PROGRESS),
  ANTP_PAIR(TRANSFER_SEQUENCE_NUMBER_ERROR),
  ANTP_PAIR(TRANSFER_IN_ERROR),
  {-1,"UNKNOWN"}
};

AntFSCommandNames antFSCommandNames[]={
  ANTP_PAIR(ANTFS_CmdLink),
  ANTP_PAIR(ANTFS_CmdDisconnect),
  ANTP_PAIR(ANTFS_CmdAuthenticate),
  ANTP_PAIR(ANTFS_CmdPing),
  ANTP_PAIR(ANTFS_ReqDownload),
  ANTP_PAIR(ANTFS_ReqUpload),
  ANTP_PAIR(ANTFS_ReqErase),
  ANTP_PAIR(ANTFS_UploadData),
  {-1,"UNKNOWN"}
};

AntFSResponseNames  antFSResponseNames[]={
  ANTP_PAIR(ANTFS_RespAuthenticate),
  ANTP_PAIR(ANTFS_RespDownload),
  ANTP_PAIR(ANTFS_RespUpload),
  ANTP_PAIR(ANTFS_RespErase),
  ANTP_PAIR(ANTFS_RespUploadData),
  {-1,"UNKNOWN"}
};


#undef ANTP_PAIR








// explicit template instantiation

template const std::string toString(const int& val, const int width, const char fill);
template const std::string toString(const char& val, const int width, const char fill);
template const std::string toString(const unsigned char& val, const int width, const char fill);
template const std::string toString(const unsigned int& val, const int width, const char fill);
template const std::string toString(const unsigned short& val, const int width, const char fill);
template const std::string toString(const uint64_t& val, const int width, const char fill);


template const std::string toStringDec(const int& val, const int width, const char fill);
template const std::string toStringDec(const unsigned long& val, const int width, const char fill);
template const std::string toStringDec(const double& val, const int width, const char fill);
template const std::string toStringDec(const unsigned int& val, const int width, const char fill);



#ifdef _WIN64
template const std::string toStringDec(const uint64_t& val, const int width, const char fill);
#endif

}
