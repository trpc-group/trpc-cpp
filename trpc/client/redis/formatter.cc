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

#include <string.h>

#include "trpc/client/redis/formatter.h"

namespace trpc {

namespace redis {

// #lizard forgives
int Formatter::FormatCommand(Request* req, const char* format, va_list ap) {
  const char* c = format;
  int touched = 0;  // was the current argument touched?
  std::string cur_arg = "";

  while (*c != '\0') {
    if (*c != '%' || c[1] == '\0') {
      if (*c == ' ') {
        if (touched) {
          req->params_.push_back(cur_arg);
          cur_arg = "";
          touched = 0;
        }
      } else {
        cur_arg.push_back(*c);
        touched = 1;
      }
    } else {
      char* arg;
      size_t size;

      switch (c[1]) {
        case 's':
          arg = va_arg(ap, char*);
          size = strlen(arg);
          if (size > 0) {
            cur_arg += std::string(arg, size);
          }
          break;
        case 'b':
          arg = va_arg(ap, char*);
          size = va_arg(ap, size_t);
          if (size > 0) {
            cur_arg += std::string(arg, size);
          }
          break;
        case '%':
          cur_arg += "%";
          break;
        default:
          // Try to detect printf format
          {
            static const char intfmts[] = "diouxX";
            const char* p = c + 1;
            va_list cpy;

            do {
              // Flags
              if (*p != '\0' && *p == '#') p++;
              if (*p != '\0' && *p == '0') p++;
              if (*p != '\0' && *p == '-') p++;
              if (*p != '\0' && *p == ' ') p++;
              if (*p != '\0' && *p == '+') p++;

              // Field width
              while (*p != '\0' && isdigit(*p)) p++;

              // Precision
              if (*p == '.') {
                p++;
                while (*p != '\0' && isdigit(*p)) p++;
              }

              // Copy va_list before consuming with va_arg
              va_copy(cpy, ap);

              // Integer conversion (without modifiers)
              if (strchr(intfmts, *p) != nullptr) {
                va_arg(ap, int);
                break;
              }

              // Double conversion (without modifiers)
              if (strchr("eEfFgGaA", *p) != nullptr) {
                va_arg(ap, double);
                break;
              }

              // Size: char
              if (p[0] == 'h' && p[1] == 'h') {
                p += 2;
                if (*p != '\0' && strchr(intfmts, *p) != nullptr) {
                  va_arg(ap, int);  // char gets promoted to int
                  break;
                }
                va_end(cpy);
                return -1;
              }

              // Size: short
              if (p[0] == 'h') {
                p += 1;
                if (*p != '\0' && strchr(intfmts, *p) != nullptr) {
                  va_arg(ap, int);  // short gets promoted to int
                  break;
                }
                va_end(cpy);
                return -1;
              }

              // Size: int64_t
              if (p[0] == 'l' && p[1] == 'l') {
                p += 2;
                if (*p != '\0' && strchr(intfmts, *p) != nullptr) {
                  va_arg(ap, int64_t);
                  break;
                }
                va_end(cpy);
                return -1;
              }

              // Size: int64_t
              if (p[0] == 'l') {
                p += 1;
                if (*p != '\0' && strchr(intfmts, *p) != nullptr) {
                  va_arg(ap, int64_t);
                  break;
                }
                va_end(cpy);
                return -1;
              }
            } while (0);

            char format[16];
            size_t l = 0;

            l = (p + 1) - c;
            if (l < sizeof(format) - 2) {
              memcpy(format, c, l);
              format[l] = '\0';

              if (VPrintf(cur_arg, format, cpy)) {
                return -1;
              }

              // Update current position (note: outer blocks
              // increment c twice so compensate here)
              c = p - 1;
            }

            va_end(cpy);
            break;
          }
      }

      touched = 1;
      c++;
    }
    c++;
  }

  // Add the last argument if needed
  if (touched) {
    req->params_.push_back(cur_arg);
  }

  return 0;
}

int Formatter::VPrintf(std::string& current, const char* format, va_list ap) {
  va_list cpy;
  char staticbuf[1024], *buf = staticbuf;
  size_t buf_len = strlen(format) * 2;
  size_t len = 0;

  // We try to start using a static buffer for speed.
  // If not possible we revert to heap allocation.
  if (buf_len > sizeof(staticbuf)) {
    buf = new (std::nothrow) char[buf_len];
    if (buf == nullptr) {
      return -1;
    }
  } else {
    buf_len = sizeof(staticbuf);
  }

  // Try with buffers two times bigger every time we fail to
  // fit the string in the current buffer size.
  while (1) {
    buf[buf_len - 2] = '\0';
    va_copy(cpy, ap);
    len = vsnprintf(buf, buf_len, format, cpy);
    va_end(cpy);
    if (buf[buf_len - 2] != '\0') {
      if (buf != staticbuf) {
        delete[] buf;
      }
      buf_len *= 2;
      buf = new (std::nothrow) char[buf_len];
      if (buf == nullptr) {
        return -1;
      }
      continue;
    }
    break;
  }

  // Finally concat the obtained string to the SDS string and return it.
  current += std::string(buf, len);

  if (buf != staticbuf) {
    delete[] buf;
  }

  return 0;
}

int Formatter::TFormatCommand(Request* req, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  if (FormatCommand(req, format, ap)) {
    va_end(ap);
    return -1;
  }
  va_end(ap);
  return 0;
}

int Formatter::TVPrintf(std::string& current, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int r = VPrintf(current, format, ap);
  va_end(ap);
  return r;
}

}  // namespace redis

}  // namespace trpc
