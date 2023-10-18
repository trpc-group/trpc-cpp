[中文](../zh/http_protocol_upload_download_client.md)

# Overview

tRPC-Cpp provides a set of interfaces for streaming reading and writing of data in HTTP, which can receive and send
large files in fragments. This article introduces how to access the HTTP file upload-download service based on
tRPC-Cpp (referred to as tRPC below), and developers can learn the following:

* How to use synchronous streaming interfaces to access file upload-download service.
  * Programming interfaces.
  * Accessing upload-download service.
* How to use asynchronous streaming interfaces to access file upload-download service.
  * Programming interfaces.
  * Code examples.
* FAQ.

# How to use synchronous streaming interfaces to access file upload-download service

Accessing HTTP file upload and download services is based on `HttpServiceProxy` and `HttpClientStreamReaderWriter`. When
transmitting data, we can specify the length `Content-Length: $length`, or use chunked
transfer `Transfer-Encoding: chunked`.

*Note: The synchronous streaming interface needs to run in the `fiber` coroutine environment.*

## Programming interfaces about synchronous streaming

### Client stream reader writer by synchronous

Obtain the stream reader-writer through the following interfaces of `HttpServiceProxy`, and the object type
is `HttpClientStreamReaderWriter`. The interface list is as follows:

| Object Type                  | Interface Signature                                                                        | Function                                                                                                                                                            | Parameters                                                                                                                                                                                             | Return Value                     |
|------------------------------|--------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------|
| HttpServiceProxy             | HttpClientStreamReaderWriter Get(const ClientContextPtr& context, const std::string& url)  | HTTP GET method, generally used to download data                                                                                                                    | context: request context; url: HTTP URL                                                                                                                                                                | HTTP client stream reader-writer |
| HttpServiceProxy             | HttpClientStreamReaderWriter Post(const ClientContextPtr& context, const std::string& url) | HTTP POST method, generally used to upload data                                                                                                                     | context: request context; url: HTTP URL                                                                                                                                                                | HTTP client stream reader-writer |
| HttpServiceProxy             | HttpClientStreamReaderWriter Put(const ClientContextPtr& context, const std::string& url)  | HTTP PUT method, generally used to upload data                                                                                                                      | context: request context; url: HTTP URL                                                                                                                                                                | HTTP client stream reader-writer |
| HttpClientStreamReaderWriter | Status GetStatus()                                                                         | Get the status after the reader-writer is initialized                                                                                                               | -                                                                                                                                                                                                      | Status                           |
| HttpClientStreamReaderWriter | Status ReadHeaders(int&amp; code, HttpHeader&amp; http_header)                             | Get the response headers, use the timeout+now configured by the service as the timeout point to block this operation                                                | -                                                                                                                                                                                                      | Status                           |
| HttpClientStreamReaderWriter | Status ReadHeaders(int&amp; code, HttpHeader&amp; http_header，const T&amp; expiry)         | Get the response headers, block this operation until the specified time                                                                                             | expiry, for example: trpc::ReadSteadyClock() + std::chrono::milliseconds(3), or std::chrono::milliseconds(3)                                                                                           | Status                           |
| HttpClientStreamReaderWriter | Status Read(NoncontiguousBuffer&amp; item, size_t max_bytes)                               | Read the specified length of content, use the timeout+now configured by the service as the timeout point for the entire read stream process to block this operation | max_bytes, if the remaining content after receiving the complete package is less than the specified length, it will immediately return the remaining content and indicate EOF through the return value | Status                           |
| HttpClientStreamReaderWriter | Status Read(NoncontiguousBuffer&amp; item, size_t max_bytes, const T&amp; expiry)          | Read the specified length of content, block this operation until the specified time                                                                                 | expiry, for example: trpc::ReadSteadyClock() + std::chrono::milliseconds(3), or std::chrono::milliseconds(3)                                                                                           | Status                           |
| HttpClientStreamReaderWriter | Status ReadAll(NoncontiguousBuffer&amp; item)                                              | Read the entire package content, use the timeout+now configured by the service as the timeout point for the entire read stream process to block this operation      | -                                                                                                                                                                                                      | Status                           |
| HttpClientStreamReaderWriter | Status ReadAll(NoncontiguousBuffer&amp; item, const T&amp; expiry)                         | Read the entire package content, block this operation until the specified time                                                                                      | expiry, for example: trpc::ReadSteadyClock() + std::chrono::milliseconds(3), or std::chrono::milliseconds(3)                                                                                           | Status                           |
| HttpClientStreamReaderWriter | Status Write(NoncontiguousBuffer&amp;&amp; item)                                           | Send content                                                                                                                                                        | -                                                                                                                                                                                                      | Status                           |
| HttpClientStreamReaderWriter | Status WriteDone()                                                                         | End sending                                                                                                                                                         | -                                                                                                                                                                                                      | Status                           |
| HttpClientStreamReaderWriter | void Close()                                                                               | Close the reader-writer interface                                                                                                                                   | -                                                                                                                                                                                                      | Status                           |

### Common return codes for client-side streaming interface

For timeout errors, we can try to retry, but for network errors, do not retry again, as it indicates that
there is an abnormality in the connection where the reader/writer is located.

| Return Code                                | Value | Meaning                                                               |
|--------------------------------------------|-------|-----------------------------------------------------------------------|
| kSuccStatus                                | 0     | Operation executed successfully                                       |
| kStreamStatusReadEof                       | -99   | All content has been read                                             |
| kStreamStatusClientReadTimeout             | 354   | Client read timeout                                                   |
| kStreamStatusClientWriteTimeout            | 334   | Client write timeout                                                  |
| kStreamStatusClientNetworkError            | 301   | Client network error                                                  |
| kStreamStatusClientWriteContentLengthError | 332   | The length of the written data does not match the Content-Length set. |

### Timeout explanation for read-related interfaces

Read-related interfaces: Read, ReadAll.
Taking the Read interface as an example, tRPC provides two types of specific interface forms:

* Read(item, max_bytes) without a custom timeout.

  The timeout for this type of interface is a fixed value that is determined when the reader/writer is created (
  calculated by the service configuration or specified by the SetDefaultDeadline method).

  > For example, if the timeout is configured as 60000ms and the user obtains the reader/writer at time "now", then no
  > matter how many times the user calls Read(item, max_bytes), the timeout point will always be "now + 1min", which can
  > be
  > understood as the timeout for the entire read stream process.

* Read(item, max_bytes, timeout) with a custom timeout.

  If the user's data is very large, such as a 10GB file, and the network conditions are uncertain, it is recommended to
  use the Read(item, max_bytes, timeout) interface in this scenario.

  > The "timeout" parameter here only applies to the Read operation. The "timeout" type can be a time interval, such as
  > 10s, which means that the Read operation will be blocked for 10s from the trigger start. It can also be a specific
  > time
  > point, which means that the Read operation will be blocked until the specified time point.
  
## Accessing file upload-download service

### File upload

Example: [upload_client.cc](../../examples/features/http_upload_download/client/upload_client.cc)

The basic data uploading process involves the following steps: setting the length form/chunked form, sending the request
header, writing data, completing the write, and reading the response header.

* **Setting the length form/chunked form**

  Before creating the stream (client reader/writer), set the HTTP header information through the client_context. Choose between length form and chunked form. If the complete data length is known, use length form with "Content-Length: 104857600". If the complete data length is unknown, use chunked form with "Transfer-Encoding: chunked".

* **Sending the request header**

  The client does not need to send the request header. tRPC does not provide this method. When the stream is obtained, tRPC-Cpp has already sent the request header.

* **Writing data**

  The user can continuously send data fragments to the server through the Write interface. If the user is using chunked form, there is no need to encode the transmitted data with chunked. tRPC will handle it automatically. If the user is using length form, the Write interface will report the kStreamStatusClientWriteContentLengthError error if the data sent by the user exceeds the set length.

* **Completing the write**

  The user informs the reader/writer that all data has been sent through the WriteDone interface. If the user is using chunked form, the framework will send the chunked end flag to the server. If the user is using length form, the framework will check whether the length of the data sent by the user is consistent with the set length. If they are inconsistent, the kStreamStatusClientWriteContentLengthError error will be reported. Once the WriteDone interface is called, the user should not try to use the Write interface again.
 
* **Reading the response header**

  If the ReadHeaders interface is executed successfully, it means that the response header from the server has been received. The HTTP status code (200, 404, etc.) can be obtained from the http_code parameter. These constants are also defined in tRPC, such as ResponseStatus::kOk. The response header can be obtained from the http_header parameter.

* Example code

  ```cpp
  // Upload a file through chunked.
  bool UploadWithChunked(const HttpServiceProxyPtr& proxy, const std::string& url, const std::string src_path) {
    // Open a file to be uploaded.
    auto fin = std::ifstream(src_path, std::ios::binary);
    if (!fin.is_open()) {
      return false;
    }
  
    auto ctx = ::trpc::MakeClientContext(proxy);
    ctx->SetTimeout(5000);
    // Set: "Transfer-Encoding: chunked"
    ctx->SetHttpHeader(::trpc::http::kHeaderTransferEncoding, ::trpc::http::kTransferEncodingChunked);
    // Or 
    // Set: "Content-Length: $length"
    // ctx->SetHttpHeader(::trpc::http::kHeaderContentLength, std::to_string(file_size));
    // Create a synchronous stream.
    auto stream = proxy->Post(ctx, url);
    if (!stream.GetStatus().OK()) {
      return false;
    }
    
    // Start uploading content of the file.
    std::size_t nwrite{0};
    ::trpc::BufferBuilder buffer_builder;
    for (;;) {
      trpc::Status status;
      fin.read(buffer_builder.data(), buffer_builder.SizeAvailable());
      std::size_t n = fin.gcount();
      if (n > 0) {
        ::trpc::NoncontiguousBuffer buffer;
        buffer.Append(buffer_builder.Seal(n));
        // Send content of the file.
        status = stream.Write(std::move(buffer));
        if (status.OK()) {
          nwrite += n;
          continue;
        }
        return false;
      } else if (fin.eof()) {
        // Read EOF of the file, finish writing.
        status = stream.WriteDone();
        if (status.OK()) break;
        return false;
      }
      return false;
    }
  
    int http_status = 0;
    ::trpc::http::HttpHeader http_header;
    // Read response header and check the status.
    ::trpc::Status status = stream.ReadHeaders(http_status, http_header);
    if (!status.OK()) {
      return false;
    } else if (http_status != ::trpc::http::ResponseStatus::kOk) {
      return false;
    }
    return true;
  }
  ```

### File download

Example: [download_client.cc](../../examples/features/http_upload_download/client/download_client.cc)

The basic data downloading process involves the following steps: sending the request header, reading the response
header, reading data, and completing the read.

* **Sending the request header**

  The client does not need to send the request header. tRPC does not provide this method. When the stream is obtained, tRPC-Cpp has already sent the request header.

* **Reading the response header**

  If the ReadHeaders interface is executed successfully, it means that the response header from the server has been received. The HTTP status code (200, 404, etc.) can be obtained from the http_code parameter. These constants are also defined in tRPC, such as ResponseStatus::kOk. The response header can be obtained from the http_header parameter.

* **Reading data**

  The Read(item, max_bytes) interface will block until it reads out the data of max_bytes length if the server's response data has not ended. If the server's response data has ended, the Read operation will immediately return after reading max_bytes length of data or reading the end of data.

* **Completing the read**

  When the end of data is read, the Read interface will return the kStreamStatusReadEof return code, informing the user that the server data has been completely read.

* Example code

  ```cpp
  // Download a file. 
  bool Download(const HttpServiceProxyPtr& proxy, const std::string& url, const std::string dst_path) {
    // Open a file to store data downloaded from the server.
    auto fout = std::ofstream(dst_path, std::ios::binary);
    if (!fout.is_open()) {
      return false;
    }
  
    auto ctx = ::trpc::MakeClientContext(proxy);
    ctx->SetTimeout(5000);
    // Create a synchronous stream.
    auto stream = proxy->Get(ctx, url);
    if (!stream.GetStatus().OK()) {
      return false;
    }
  
    // Read the response header.
    int http_status = 0;
    ::trpc::http::HttpHeader http_header;
    ::trpc::Status status = stream.ReadHeaders(http_status, http_header);
    if (!status.OK()) {
      return false;
    } else if (http_status != ::trpc::http::ResponseStatus::kOk) {
      return false;
    }
  
    // Read 1MB every time.
    constexpr std::size_t kBufferSize{1024 * 1024};
    size_t nread{0};
    // Start reading the content received from the server.
    for (;;) {
      ::trpc::NoncontiguousBuffer buffer;
      status = stream.Read(buffer, kBufferSize);
      if (status.OK()) {
        nread += buffer.ByteSize();
        // Write the blocks of noncontiguous buffer to the file.
        for (const auto& block : buffer) {
          fout.write(block.data(), block.size());
        }
        continue;
      } else if (status.StreamEof()) {
        // Get the EOF.
        break;
      }
      return false;
    }
    return true;
  }
  ```

# How to use asynchronous streaming interfaces to access file upload-download service

The HTTP client asynchronous streaming interface is implemented based on `HttpStreamProxy` and `HttpClientAsyncStreamReaderWriterPtr`.
When transmitting data, you can specify the length `Content-Length: $length`, or use chunked
transfer `Transfer-Encoding: chunked`.

Requirements:

* The asynchronous streaming interface needs to run in the `merge` thread model environment. The `merge` thread model is a runtime of tRPC , and the thread role does both `IO` and business logic `Handle`.
* Currently, only the asynchronous streaming interface called within tRPC 's internal thread is supported, and it is not supported to use it in user-defined external threads (the program will crash).
* Use `future/promise` programming model.
* HTTP message reading and writing comply with the HTTP protocol specification, that is, the writing order for requests and responses is as follows:
  * Write start_line/status_line + headers first.
  * Then write the body.

## Programming interfaces about asynchronous streaming

### Client stream reader writer by asynchronous

Call `GetAsyncStreamReaderWriter` of `HttpStreamProxy` to obtain the stream reader/writer object (`HttpClientAsyncStreamReaderWriterPtr`).

* The interface list is as follows:

  | Interface Signature                                                                                     | Function                                                                                                                                                       | Parameters                      |
  |---------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------|
  | Future&lt;HttpClientAsyncStreamReaderWriterPtr> GetAsyncStreamReaderWriter(const ClientContextPtr& ctx) | Obtains an asynchronous stream reader/writer                                                                                                                   | context: Client request context |
  | Future<> WriteRequestLine(HttpRequestLine&& req_line)                                                   | Writes the request start line                                                                                                                                  | Parameters                      |
  | Future<> WriteHeader(HttpHeader&& header)                                                               | Writes the header                                                                                                                                              | header: Header information      |
  | Future<> WriteData(NoncontiguousBuffer&& data)                                                          | Writes the data</br>Recognizes the information in the header. If "chunk" is set in the header, the data will be organized into chunk format before being sent. | data: Data to be written        |
  | Future<> WriteDone()                                                                                    | Completes writing the data</br>If using chunk mode, writes the chunk termination flag                                                                          | -                               |

* The interface for the client to read the response from the stream

  | Interface Signature                                                          | Function                                                                                                                               | Parameters                 |
  |-----------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------|----------------------------|
  | Future&lt;HttpStatusLine> ReadStatusLine(int timeout = max)                 | Reads the response status line from the stream                                                                                         | Timeout (ms)               |
  | Future&lt;HttpHeader> ReadHeader(int timeout = max)                         | Reads the header from the stream                                                                                                       | Timeout (ms)               |
  | Future&lt;NoncontiguousBuffer> ReadChunk(int timeout = max)                 | Reads the chunk from the stream. This interface can only be called after confirming that it is in chunk mode, otherwise it will fail (determined from the header)                                             | Timeout (ms)               |
  | Future&lt;NoncontiguousBuffer> ReadAtMost(uint64_t len, int timeout = max)  | Can be called in both length mode and chunk mode, and gets up to len length of data. </br>If the size of the data received from the network is smaller than len, return data of size. </br>If the size of the data received from the network is larger than len, return data of length len. </br>If the buffer is empty, it means EOF.</br>Scenario 1: Used in memory-limited situations, limiting the maximum read length each time.</br>Scenario 2: In relay mode, it can obtain data in a timely manner and send it downstream. | len in bytes, timeout (ms) |
  | Future&lt;NoncontiguousBuffer> ReadExactly(uint64_t len, int timeout = max) | Can be called in both length mode and chunk mode, and gets fixed length data of len. If EOF is read, it returns as much data as there is in the network. </br>If the size of the buffer read is smaller than len, it means EOF.</br>Special scenario 1: The requested data is compressed in fixed size and needs to be read in fixed size for decompression. | len in bytes, timeout (ms) |
  
* Client-side interfaces for writing complete requests and reading complete responses:
  
  | Interface Signature                                                                                     | Function                                    | Parameters                                         |
  |---------------------------------------------------------------------------------------------------------|---------------------------------------------|----------------------------------------------------|
  | Future<> WriteFullRequest(HttpClientAsyncStreamWriterPtr rw, HttpRequest&& req)                         | Writes the complete request to the stream   | rw: Client-side stream reader/writer, timeout (ms) |
  | Future&lt;HttpResponsePtr> ReadFullResponse(HttpClientAsyncStreamReaderWriterPtr rw, int timeout = max) | Reads the complete response from the stream | rw: Client-side stream reader/writer, timeout (ms) |
  