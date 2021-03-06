//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Fork a process and create a pipe to its standard input.
//
//----------------------------------------------------------------------------

#include "tsForkPipe.h"
#include "tsNullReport.h"
#include "tsMemoryUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor / destructor
//----------------------------------------------------------------------------

ts::ForkPipe::ForkPipe() :
    _in_mode(USE_PIPE),
    _is_open(false),
    _synchronous(false),
    _ignore_abort(false),
    _broken_pipe(false),
#if defined(TS_WINDOWS)
    _handle(INVALID_HANDLE_VALUE),
    _process(INVALID_HANDLE_VALUE)
#else
    _fpid(0),
    _fd(-1)
#endif
{
    // We will handle broken-pipe errors, don't kill us for that.
    IgnorePipeSignal();
}


ts::ForkPipe::~ForkPipe()
{
    close(NULLREP);
}


//----------------------------------------------------------------------------
// Create the process, open the pipe.
// If synchronous is true, wait for process termination in close.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::ForkPipe::open(const UString& command, bool synchronous, size_t buffer_size, Report& report, OutputMode out_mode, InputMode in_mode)
{
    if (_is_open) {
        report.error(u"pipe is already open");
        return false;
    }

    _in_mode = in_mode;
    _broken_pipe = false;
    _synchronous = synchronous;

    report.debug(u"creating process \"%s\"", {command});

#if defined (TS_WINDOWS)

    _handle = INVALID_HANDLE_VALUE;
    _process = INVALID_HANDLE_VALUE;
    ::HANDLE read_handle = INVALID_HANDLE_VALUE;
    ::HANDLE write_handle = INVALID_HANDLE_VALUE;

    // Create a pipe
    if (_in_mode == USE_PIPE) {
        ::DWORD bufsize = buffer_size == 0 ? 0 : ::DWORD(std::max<size_t>(32768, buffer_size));
        ::SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = 0;
        sa.bInheritHandle = TRUE;
        if (::CreatePipe(&read_handle, &write_handle, &sa, bufsize) == 0) {
            report.error(u"error creating pipe: %s", {ErrorCodeMessage()});
            return false;
        }

        // CreatePipe can only inherit none or both handles. Since we need the
        // read handle to be inherited by the child process, we said "inherit".
        // Now, make sure that the write handle of the pipe is not inherited.
        ::SetHandleInformation(write_handle, HANDLE_FLAG_INHERIT, 0);
    }

    // Our standard handles.
    const ::HANDLE in_handle  = ::GetStdHandle(STD_INPUT_HANDLE);
    const ::HANDLE out_handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    const ::HANDLE err_handle = ::GetStdHandle(STD_ERROR_HANDLE);

    // Process startup info specifies standard handles.
    // Make sure our handles can be inherited when necessary.
    ::STARTUPINFOW si;
    TS_ZERO(si);
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;

    switch (_in_mode) {
        case USE_PIPE:
            si.hStdInput = read_handle;
            break;
        case KEEP_STDIN:
            ::SetHandleInformation(in_handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
            si.hStdInput = in_handle;
            break;
        default:
            // Invalid enum value.
            if (_in_mode == USE_PIPE) {
                ::CloseHandle(read_handle);
                ::CloseHandle(write_handle);
            }
            return false;
    }

    switch (out_mode) {
        case KEEP_BOTH:
            ::SetHandleInformation(out_handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
            ::SetHandleInformation(err_handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
            si.hStdOutput = out_handle;
            si.hStdError = err_handle;
            break;
        case STDOUT_ONLY:
            ::SetHandleInformation(out_handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
            si.hStdOutput = si.hStdError = out_handle;
            break;
        case STDERR_ONLY:
            ::SetHandleInformation(err_handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
            si.hStdOutput = si.hStdError = err_handle;
            break;
        default:
            // Invalid enum value.
            if (_in_mode == USE_PIPE) {
                ::CloseHandle(read_handle);
                ::CloseHandle(write_handle);
            }
            return false;
    }

    // ::CreateProcess may modify the user-supplied command line (ugly!)
    UString cmd(command);
    ::WCHAR* cmdp = const_cast<::WCHAR*>(cmd.wc_str());

    // Create the process
    ::PROCESS_INFORMATION pi;
    if (::CreateProcessW(NULL, cmdp, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) == 0) {
        report.error(u"error creating process: %s", {ErrorCodeMessage()});
        if (_in_mode == USE_PIPE) {
            ::CloseHandle(read_handle);
            ::CloseHandle(write_handle);
        }
        return false;
    }

    // Close unused handles
    if (_synchronous) {
        _process = pi.hProcess;
    }
    else {
        _process = INVALID_HANDLE_VALUE;
        ::CloseHandle(pi.hProcess);
    }
    ::CloseHandle(pi.hThread);

    // Keep the writing end-point of pipe for data transmission.
    // Close the reading end-point of pipe.
    if (_in_mode == USE_PIPE) {
        _handle = write_handle;
        ::CloseHandle(read_handle);
    }

#else // UNIX

    // Create a pipe
    int filedes[2];
    if (_in_mode == USE_PIPE && ::pipe(filedes) < 0) {
        report.error(u"error creating pipe: %s", {ErrorCodeMessage()});
        return false;
    }

    // Create the forked process
    if ((_fpid = ::fork()) < 0) {
        report.error(u"fork error: %s", {ErrorCodeMessage()});
        if (_in_mode == USE_PIPE) {
            ::close(filedes[0]);
            ::close(filedes[1]);
        }
        return false;
    }
    else if (_fpid == 0) {
        // In the context of the created process.

        // Setup input pipe.
        if (_in_mode == USE_PIPE) {
            // Close standard input.
            ::close(STDIN_FILENO);

            // Close the writing end-point of the pipe.
            ::close(filedes[1]);

            // Redirect the reading end-point of the pipe to standard input
            if (::dup2(filedes[0], STDIN_FILENO) < 0) {
                ::perror("error redirecting stdin in forked process");
                ::exit(EXIT_FAILURE);
            }

            // Close the now extraneous file descriptor.
            ::close(filedes[0]);
        }

        // Merge stdout and stderr if requested.
        switch (out_mode) {
            case STDOUT_ONLY:
                // Use stdout as stderr as well.
                if (::dup2(STDOUT_FILENO, STDERR_FILENO) < 0) {
                    ::perror("error redirecting stdout to stderr");
                }
                break;
            case STDERR_ONLY:
                // Use stderr as stdout as well.
                if (::dup2(STDERR_FILENO, STDOUT_FILENO) < 0) {
                    ::perror("error redirecting stderr to stdout");
                }
                break;
            default:
                // Nothing to do.
                break;
        }

        // Execute the command. Should not return.
        ::execl("/bin/sh", "/bin/sh", "-c", command.toUTF8().c_str(), TS_NULL);
        ::perror("exec error");
        ::exit(EXIT_FAILURE);
        assert(false); // should never get there
    }
    else {
        // In the context of the parent process.

        // Keep the writing end-point of pipe for data transmission.
        // Close the reading end-point of pipe.
        if (_in_mode == USE_PIPE) {
            _fd = filedes[1];
            ::close(filedes[0]);
        }
    }

#endif

    _is_open = true;
    return true;
}


//----------------------------------------------------------------------------
// Close the pipe. Optionally wait for process termination.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::ForkPipe::close (Report& report)
{
    // Silent error is already closed
    if (!_is_open) {
        return false;
    }

    bool result = true;

#if defined (TS_WINDOWS)

    // Close the pipe handle
    if (_in_mode == USE_PIPE) {
        ::CloseHandle(_handle);
    }

    // Wait for termination of child process
    if (_synchronous && ::WaitForSingleObject(_process, INFINITE) != WAIT_OBJECT_0) {
        report.error(u"error waiting for process termination: %s", {ErrorCodeMessage()});
        result = false;
    }

    if (_process != INVALID_HANDLE_VALUE) {
        ::CloseHandle(_process);
    }

#else // UNIX

    // Close the pipe file descriptor
    if (_in_mode == USE_PIPE) {
        ::close(_fd);
    }

    // Wait for termination of forked process
    assert(_fpid != 0);
    if (_synchronous && ::waitpid(_fpid, NULL, 0) < 0) {
        report.error(u"error waiting for process termination: %s", {ErrorCodeMessage()});
        result = false;
    }

#endif

    _is_open = false;
    return result;
}


//----------------------------------------------------------------------------
// Write data to the pipe (received at process' standard input).
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::ForkPipe::write (const void* addr, size_t size, Report& report)
{
    if (!_is_open) {
        report.error(u"pipe is not open");
        return false;
    }
    if (_in_mode != USE_PIPE) {
        report.error(u"process was created without pipe");
        return false;
    }

    // If pipe already broken, return
    if (_broken_pipe) {
        return _ignore_abort;
    }

    bool error = false;
    ErrorCode error_code = SYS_SUCCESS;

#if defined (TS_WINDOWS)

    const char* data = reinterpret_cast <const char*> (addr);
    ::DWORD remain = ::DWORD (size);
    ::DWORD outsize;

    while (remain > 0 && !error) {
        if (::WriteFile(_handle, data, remain, &outsize, NULL) != 0) {
            // Normal case, some data were written
            assert(outsize <= remain);
            data += outsize;
            remain -= std::max(remain, outsize);
        }
        else {
            // Write error
            error_code = LastErrorCode();
            error = true;
            // MSDN documentation on WriteFile says ERROR_BROKEN_PIPE,
            // experience says ERROR_NO_DATA.
            _broken_pipe = error_code == ERROR_BROKEN_PIPE || error_code == ERROR_NO_DATA;
        }
    }

#else // UNIX

    const char *data = reinterpret_cast <const char*> (addr);
    size_t remain = size;

    while (remain > 0 && !error) {
        ssize_t outsize = ::write (_fd, data, remain);
        if (outsize > 0) {
            // Normal case, some data were written
            assert(size_t(outsize) <= remain);
            data += outsize;
            remain -= std::max(remain, size_t(outsize));
        }
        else if ((error_code = LastErrorCode()) != EINTR) {
            // Actual error (not an interrupt)
            error_code = LastErrorCode();
            error = true;
            _broken_pipe = error_code == EPIPE;
        }
    }
#endif

    if (!error) {
        return true;
    }
    else if (!_broken_pipe) {
        // Always report non-pipe error (message + error status).
        report.error(u"error writing to pipe: %s", {ErrorCodeMessage(error_code)});
        return false;
    }
    else if (_ignore_abort) {
        // Broken pipe but must be ignored. Report a verbose message
        // the first time to inform that data will continue to be
        // processed but will be ignored by the forked process.
        report.verbose(u"broken pipe, stopping transmission to forked process");
        // Not an error (ignored)
        return true;
    }
    else {
        // Broken pipe. Do not report a message, but report as error
        return false;
    }
}
