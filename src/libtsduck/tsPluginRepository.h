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
//!
//!  @file
//!  TSP plugin repository
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlugin.h"
#include "tsReport.h"
#include "tsSingletonManager.h"

namespace ts {
    //!
    //! A repository of TSP plugins, either statically or dynamically linked.
    //!
    //! This class is a singleton. Use static Instance() method to access the single instance.
    //!
    class TSDUCKDLL PluginRepository
    {
        TS_DECLARE_SINGLETON(PluginRepository);

    public:
        //!
        //! Allow or disallow the loading of plugins from shareable objects.
        //! When disabled, only statically registered plugins are allowed.
        //! Loading is initially enabled by default.
        //! @param [in] allowed When true, dynamically loading plugins is allowed.
        //!
        void setSharedLibraryAllowed(bool allowed) { _sharedLibraryAllowed = allowed; }

        //!
        //! Register an input plugin.
        //! @param [in] name Plugin name.
        //! @param [in] allocator New input plugin allocator function. Ignored when zero.
        //!
        void registerInput(const UString& name, NewInputProfile allocator);

        //!
        //! Register a packet processor plugin.
        //! @param [in] name Plugin name.
        //! @param [in] allocator New packet processor plugin allocator function. Ignored when zero.
        //!
        void registerProcessor(const UString& name, NewProcessorProfile allocator);

        //!
        //! Register an output plugin.
        //! @param [in] name Plugin name.
        //! @param [in] allocator New output plugin allocator function. Ignored when zero.
        //!
        void registerOutput(const UString& name, NewOutputProfile allocator);

        //!
        //! Get an input plugin by name.
        //! If not found and loading shareable library is allowed, try to load it.
        //! @param [in] name Plugin name.
        //! @param [in,out] report Where to report errors.
        //! @return Plugin allocator function. Zero when not found.
        //!
        NewInputProfile getInput(const UString& name, Report& report);

        //!
        //! Get a packet processor plugin by name.
        //! If not found and loading shareable library is allowed, try to load it.
        //! @param [in] name Plugin name.
        //! @param [in,out] report Where to report errors.
        //! @return Plugin allocator function. Zero when not found.
        //!
        NewProcessorProfile getProcessor(const UString& name, Report& report);

        //!
        //! Get an output plugin by name.
        //! If not found and loading shareable library is allowed, try to load it.
        //! @param [in] name Plugin name.
        //! @param [in,out] report Where to report errors.
        //! @return Plugin allocator function. Zero when not found.
        //!
        NewOutputProfile getOutput(const UString& name, Report& report);

        //!
        //! Get the number of registered input plugins.
        //! @return The number of registered input plugins.
        //!
        size_t inputCount() const { return _inputPlugins.size(); }

        //!
        //! Get the number of registered processor plugins.
        //! @return The number of registered processor plugins.
        //!
        size_t processorCount() const { return _processorPlugins.size(); }

        //!
        //! Get the number of registered output plugins.
        //! @return The number of registered output plugins.
        //!
        size_t outputCount() const { return _outputPlugins.size(); }

        //!
        //! Load all available tsp processors.
        //! Does nothing when dynamic loading of plugins is disabled.
        //! @param [in,out] report Where to report errors.
        //!
        void loadAllPlugins(Report& report);

        //!
        //! List all tsp processors.
        //! This function is typically used to implement the <code>tsp -\-list-processors</code> option.
        //! @param [in] loadAll When true, all available plugins are loaded first.
        //! Ignored when dynamic loading of plugins is disabled.
        //! @param [in,out] report Where to report errors.
        //! @return The text to display.
        //!
        UString listPlugins(bool loadAll, Report& report);

        //!
        //! A class to register plugins.
        //!
        //! The registration is performed using constructors.
        //! Thus, it is possible to perform a registration in the declaration of a static object.
        //!
        class Register
        {
        public:
            //!
            //! The constructor registers an input plugin.
            //! @param [in] name Plugin name.
            //! @param [in] allocator New input plugin allocator function.
            //!
            Register(const char* name, NewInputProfile allocator);

            //!
            //! The constructor registers a packet processor plugin.
            //! @param [in] name Plugin name.
            //! @param [in] allocator New packet processor plugin allocator function.
            //!
            Register(const char* name, NewProcessorProfile allocator);

            //!
            //! The constructor registers an output plugin.
            //! @param [in] name Plugin name.
            //! @param [in] allocator New output plugin allocator function.
            //!
            Register(const char* name, NewOutputProfile allocator);

        private:
            // Inaccessible operations.
            Register() = delete;
            Register(const Register&) = delete;
            Register& operator=(const Register&) = delete;
        };

    private:
        typedef std::map<UString, NewInputProfile>     InputMap;
        typedef std::map<UString, NewProcessorProfile> ProcessorMap;
        typedef std::map<UString, NewOutputProfile>    OutputMap;

        bool         _sharedLibraryAllowed;
        InputMap     _inputPlugins;
        ProcessorMap _processorPlugins;
        OutputMap    _outputPlugins;

        // List one plugin.
        static void ListOnePlugin(UString& out, const UString& name, Plugin* plugin, size_t name_width);
    };
}
