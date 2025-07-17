//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include <errno.h>
#include <string>

namespace trpc::ssl {

// value of errno
using trpc_err_t = int;

const int kErrnoEPERM = EPERM;
const int kErrnoENOENT = ENOENT;
const int kErrnoENOPATH = ENOENT;
const int kErrnoESRCH = ESRCH;
const int kErrnoEINTR = EINTR;
const int kErrnoECHILD = ECHILD;
const int kErrnoENOMEM = ENOMEM;
const int kErrnoEACCES = EACCES;
const int kErrnoEBUSY = EBUSY;
const int kErrnoEEXIST = EEXIST;
const int kErrnoEEXIST_FILE = EEXIST;
const int kErrnoEXDEV = EXDEV;
const int kErrnoENOTDIR = ENOTDIR;
const int kErrnoEISDIR = EISDIR;
const int kErrnoEINVAL = EINVAL;
const int kErrnoENFILE = ENFILE;
const int kErrnoEMFILE = EMFILE;
const int kErrnoENOSPC = ENOSPC;
const int kErrnoEPIPE = EPIPE;
const int kErrnoEINPROGRESS = EINPROGRESS;
const int kErrnoENOPROTOOPT = ENOPROTOOPT;
const int kErrnoEOPNOTSUPP = EOPNOTSUPP;
const int kErrnoEADDRINUSE = EADDRINUSE;
const int kErrnoECONNABORTED = ECONNABORTED;
const int kErrnoECONNRESET = ECONNRESET;
const int kErrnoENOTCONN = ENOTCONN;
const int kErrnoETIMEDOUT = ETIMEDOUT;
const int kErrnoECONNREFUSED = ECONNREFUSED;
const int kErrnoENAMETOOLONG = ENAMETOOLONG;
const int kErrnoENETDOWN = ENETDOWN;
const int kErrnoENETUNREACH = ENETUNREACH;
const int kErrnoEHOSTDOWN = EHOSTDOWN;
const int kErrnoEHOSTUNREACH = EHOSTUNREACH;
const int kErrnoENOSYS = ENOSYS;
const int kErrnoECANCELED = ECANCELED;
const int kErrnoEILSEQ = EILSEQ;
const int kErrnoENOMOREFILES = 0;
const int kErrnoELOOP = ELOOP;
const int kErrnoEBADF = EBADF;
const int kErrnoEAGAIN = EAGAIN;

#define trpc_errno() errno
#define trpc_socket_errno() errno
#define trpc_set_errno(err) errno = err
#define trpc_set_socket_errno(err) errno = err
#define trpc_strerrno(err) strerror(err)

}  // namespace trpc::ssl
