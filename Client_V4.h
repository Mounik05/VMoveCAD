/* ==========================================================================
 *       Filename:  Vct/License/Client.h
 *      Copyright:  Visual Collaboration Technologies Inc.
 * ======================================================================== */

/**
 * \file
 * @brief Defines the standard return codes used by VCollab applications
 *
 */

#ifndef Vct_License_Client_h
#define Vct_License_Client_h

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#   pragma once
#endif

#ifndef __cplusplus
#  error This is a C++ include file and cannot be used from plain C
#endif

#include <string>
#include "Vct/Api/Exports.h"

namespace Vct { namespace License {

    /**
     */
    class VCT_API Client {
    public:     // types
        ///
        enum LicenseType { 
            // Checks out generic license.
            // Ignores environment variable.
            GENERIC,

            // Checks out Scope license.
            // Throws error if environment variable is not defined.
            SCOPE,

            // Checks out Scope license matching with the environment.
            // Otherwise, checks out generic licnese
            SCOPE_IF_ENV_ELSE_GENERIC,    

            // Checks out Scope license matching with the environment.
            // If environment is not set, or if scope license check out fails, 
            // Checks out generic license
            GENERIC_IF_SCOPE_FAILS,
        };

        ///
        class Impl;

    private:    // structors
        /**
         */
        Client();

    public:     // structors
        /**
         */
        virtual ~Client();

    public:     // methods
        /**
         */
        static const Client& get();

        /**
         */
        bool acquireLicense(const char* const name, const char* const version, LicenseType type = SCOPE_IF_ENV_ELSE_GENERIC) const;

        /**
         */
        bool releaseLicense(const char* const name, LicenseType type = SCOPE_IF_ENV_ELSE_GENERIC) const;

        /**
         */
        void releaseAllLicenses() const;

		/**
         */
        std::string getErrorString() const;

        char* GetCEID() const;

        void SetIdleEnabled(bool bEnabled) const;

        bool IsIdleEnabled() const;

        int GetCurrentLicenseStatus(/*bool bWait=true*/) const;

        void ResetLicenseStatus() const;

    protected:      // attributes
        ///
        Impl * mpImpl;

        ///
        friend class Feature;
    };

} } // namespaces License, Vct

#endif // Vct_License_Client_h
